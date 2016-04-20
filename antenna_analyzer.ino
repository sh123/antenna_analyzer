/**
 *  Antenna analyzer based on si5341 and pcd8544 display
 *
 **/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <si5351.h>
#include <Rotary.h>
#include <SimpleTimer.h>
#include "Wire.h"

#define SWR_MAX            99
#define SWR_LIST_SIZE      84
#define SWR_SCREEN_HEIGHT  48
#define SWR_SCREEN_CHAR    8
#define SWR_GRAPH_HEIGHT   (SWR_SCREEN_HEIGHT - SWR_SCREEN_CHAR)
#define SWR_GRAPH_CROP     5

#define FREQ_STEP_INC      2500000ULL
#define FREQ_STEP_MAX      100000000ULL
#define FREQ_MAX           16000000000ULL
#define BANDS_CNT          12

#define TO_KHZ(freq)       (freq / (1000ULL * SI5351_FREQ_MULT))
#define VALID_RANGE(freq)  (freq < FREQ_MAX && !(freq > 14810000000ULL && freq < 15000000000ULL))

SimpleTimer g_timer;
Si5351 g_generator;
Adafruit_PCD8544 g_display = Adafruit_PCD8544(7, 6, 5, 4, 3);
Rotary g_rotary = Rotary(11, 12, 13);

enum SCREEN_STATE {
  S_MAIN_SCREEN = 0,
  S_GRAPH_MANUAL,
  S_GRAPH_AUTOMATIC,
  S_CHANGE_STEP
};

// band selection
struct band_t {
  uint64_t freq;
  uint64_t freq_step;
  char *band_name;
} const g_bands[BANDS_CNT] PROGMEM = {
  { 350000000ULL,   1000000ULL,  "80m"  },
  { 700000000ULL,   1000000ULL,  "40m"  },
  { 1010000000ULL,  1000000ULL,  "30m"  },
  { 1400000000ULL,  1000000ULL,  "20m"  },
  { 1800000000ULL,  1000000ULL,  "17m"  },
  { 2100000000ULL,  1000000ULL,  "15m"  },
  { 2480000000ULL,  1000000ULL,  "12m"  },
  { 2700000000ULL,  2500000ULL,  "CB "  },
  { 2800000000ULL,  2500000ULL,  "10m"  },
  { 5000000000ULL,  5000000ULL,  "6m "  },
  { 7000000000ULL,  5000000ULL,  "4m "  },
  { 14400000000ULL, 5000000ULL,  "2m "  }
};

int g_active_band_index = 0;
struct band_t g_active_band;

long g_freq_min;
double g_swr_min;

// swr graph
unsigned char g_swr_list[SWR_LIST_SIZE];

// program state
SCREEN_STATE g_screen_state;
bool g_do_update = true;

/* --------------------------------------------------------------------------*/

void setup()
{
  Serial.begin(9600);

  g_screen_state = S_MAIN_SCREEN;
  swr_list_clear(); 
  band_select(g_active_band_index);

  g_generator.init(SI5351_CRYSTAL_LOAD_6PF, 0);
  g_generator.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
  g_generator.set_freq(g_active_band.freq, 0UL, SI5351_CLK2);

  g_timer.setInterval(1000, process_display_swr);
  g_timer.setInterval(100, process_rotary_button);
  g_timer.setInterval(10, process_rotary);

  g_display.begin();
  g_display.setContrast(60);
  g_display.display();
  delay(1000);
  g_display.clearDisplay();
  g_display.display();
}

/* --------------------------------------------------------------------------*/

void swr_list_clear() {
  for (int i = 0; i < SWR_LIST_SIZE; i++) {
    g_swr_list[i] = 0;
  }
}

void swr_list_shift_right() {
  g_swr_list[0] = 0;
  for (int i = SWR_LIST_SIZE - 1; i != 0; i--) {
    g_swr_list[i + 1] = g_swr_list[i];
  }
}

void swr_list_shift_left() {
  g_swr_list[SWR_LIST_SIZE - 1] = 0;
  for (int i = 0; i < SWR_LIST_SIZE - 1; i++) {
    g_swr_list[i] = g_swr_list[i + 1];
  }
}

void swr_list_store_center(double swr) {
  int swr_graph = swr * (double)SWR_GRAPH_HEIGHT / (double)SWR_GRAPH_CROP;
  if (swr_graph > SWR_GRAPH_HEIGHT) {
    swr_graph = SWR_GRAPH_HEIGHT;
  }
  g_swr_list[SWR_LIST_SIZE / 2] = (unsigned char)swr_graph;
}

void swr_list_draw() {
  for (int i = 0; i < SWR_LIST_SIZE; i++) {
    if (g_swr_list[i] != 0) {
      g_display.drawFastVLine(i, SWR_SCREEN_HEIGHT - g_swr_list[i] + SWR_GRAPH_CROP, g_swr_list[i] - SWR_GRAPH_CROP, BLACK);
    }
  }
}

void swr_list_sweep_and_fill() {

  uint64_t freq_hz = g_active_band.freq - g_active_band.freq_step * SWR_LIST_SIZE / 2;

  double swr = SWR_MAX;
    
  for (int i = 0; i < SWR_LIST_SIZE; i++) {

    if (VALID_RANGE(freq_hz)) {
      g_generator.set_freq(freq_hz, 0ULL, SI5351_CLK2);
      int val_fwd = analogRead(0);
      int val_rfl = analogRead(1);
      swr = swr_calculate(val_fwd, val_rfl);
    }

    if (swr < g_swr_min) {
      g_swr_min = swr;
      g_freq_min = TO_KHZ(freq_hz);
    }

    int swr_graph = swr * (double)SWR_GRAPH_HEIGHT / (double)SWR_GRAPH_CROP;
    if (swr_graph > SWR_GRAPH_HEIGHT) {
      swr_graph = SWR_GRAPH_HEIGHT;
    }
    g_swr_list[i] = (unsigned char)swr_graph;

    freq_hz += g_active_band.freq_step;
  }

  g_generator.set_freq(g_active_band.freq, 0ULL, SI5351_CLK2);
}

/* --------------------------------------------------------------------------*/

void grid_draw() {

  g_display.drawFastVLine(SWR_LIST_SIZE / 2, SWR_SCREEN_CHAR, SWR_SCREEN_CHAR / 2, BLACK);

  for (unsigned char x = 0; x <= SWR_LIST_SIZE; x += SWR_LIST_SIZE / 12) {

    for (unsigned char y = SWR_SCREEN_CHAR; y <= SWR_GRAPH_HEIGHT + SWR_GRAPH_CROP; y += SWR_GRAPH_HEIGHT / SWR_GRAPH_CROP) {

      g_display.drawPixel(x + 6, y + SWR_SCREEN_CHAR - 1, BLACK);

    } // y

  } // x
}

void screen_select_next() {
  switch (g_screen_state) {

    case S_MAIN_SCREEN:
      g_screen_state = S_GRAPH_MANUAL;
      break;

    case S_GRAPH_MANUAL:
      g_screen_state = S_GRAPH_AUTOMATIC;
      swr_list_sweep_and_fill();
      break;

    case S_GRAPH_AUTOMATIC:
      g_screen_state = S_CHANGE_STEP;
      break;

    case S_CHANGE_STEP:
      g_screen_state = S_MAIN_SCREEN;
      break;

     default:
       break;
  }
}

void band_select_next() {
  g_active_band_index += 1;
  if (g_active_band_index >= BANDS_CNT) {
    g_active_band_index = 0;
  }
  band_select(g_active_band_index);
  g_generator.set_freq(g_active_band.freq, 0ULL, SI5351_CLK2);
}

void band_select(int index) {
  if (index < BANDS_CNT) {
    memcpy_PF((void*)&g_active_band, (uint_farptr_t)&g_bands[index], sizeof(g_bands[index]));
    swr_list_clear();
    g_swr_min = SWR_MAX;
    g_freq_min = g_active_band.freq / 100000ULL;
  }
}

double swr_calculate(int fwd, int rfl) {
  int val_fwd = fwd;
  int val_rfl = rfl;
  if (val_rfl > val_fwd) {
    val_rfl = val_fwd;
  }
  double gamma = (double)val_rfl / (double)val_fwd;
  double swr = (1 + gamma) / (1 - gamma);
  if (swr > SWR_MAX || isnan(swr)) {
    swr = SWR_MAX;
  }
  return swr;
}

/* --------------------------------------------------------------------------*/

void process_rotary() {
  unsigned char rotary_state = g_rotary.process();
  if (rotary_state) {

    int dir = (rotary_state == DIR_CW) ? -1 : 1;

    switch (g_screen_state) {

      case S_GRAPH_AUTOMATIC:

        g_active_band.freq += dir * g_active_band.freq_step;

        if (g_active_band.freq > FREQ_MAX) {
            g_active_band.freq = FREQ_MAX;
        }
        break;

      case S_MAIN_SCREEN:
      case S_GRAPH_MANUAL:

        g_active_band.freq += dir * g_active_band.freq_step;

        if (g_active_band.freq > FREQ_MAX) {
            g_active_band.freq = FREQ_MAX;
        }
        g_generator.set_freq(g_active_band.freq, 0ULL, SI5351_CLK2);

        if (rotary_state == DIR_CW) {
          swr_list_shift_right();
        } else {
          swr_list_shift_left();
        }
        break;

      case S_CHANGE_STEP:
        // change step
        g_active_band.freq_step += dir * FREQ_STEP_INC;
        if (g_active_band.freq_step > FREQ_STEP_MAX) { 
          g_active_band.freq_step = FREQ_STEP_INC;
        }
        break;
    }

    g_do_update = true;
  }
}

void process_rotary_button() {

  unsigned char rotary_btn_state = g_rotary.process_button();

  switch (rotary_btn_state) {

    case BTN_NONE:
      break;

    case BTN_PRESSED:
      Serial.println("button pressed");
      break;

    case BTN_RELEASED:
      Serial.println("button released");
      band_select_next();
      g_do_update = true;
      break;

    case BTN_PRESSED_LONG:
      Serial.println("button long pressed");
      screen_select_next();
      g_do_update = true;
      break;

    case BTN_RELEASED_LONG:
      Serial.println("button long released");
      break;

    default:
      break;
  }
}

void process_display_swr() {

  int val_fwd = analogRead(0);
  int val_rfl = analogRead(1);
  long freq_khz = TO_KHZ(g_active_band.freq);

  double swr = swr_calculate(val_fwd, val_rfl);
  if (swr < g_swr_min) {
    g_swr_min = swr;
    g_freq_min = freq_khz;
  }
  swr_list_store_center(swr);

  g_display.clearDisplay();
  g_display.setTextSize(1);
  g_display.setTextColor(BLACK);
  g_display.setCursor(0,0);

  switch (g_screen_state) {

    case S_MAIN_SCREEN:

      Serial.print(freq_khz); Serial.print(F(" "));
      Serial.print(val_fwd); Serial.print(F(" "));
      Serial.print(val_rfl); Serial.print(F(" "));
      Serial.print(swr); Serial.println(F(""));

      g_display.print(g_active_band.band_name); g_display.print(F(": ")); 
      g_display.print(freq_khz); g_display.println(F(" k"));
      g_display.print(F("SWR: ")); g_display.println(swr);
      g_display.print(F("FWD: ")); g_display.println(val_fwd);
      g_display.print(F("RFL: ")); g_display.println(val_rfl);
      g_display.println(F("MIN:"));
      g_display.print(g_swr_min); g_display.print(F(" ")); g_display.println(g_freq_min);

      break;

    case S_GRAPH_AUTOMATIC:
      g_display.print(F("A "));
      swr_list_sweep_and_fill();

    case S_GRAPH_MANUAL:

      g_display.print(freq_khz);
      g_display.print(F(" "));
      g_display.println(swr);

      grid_draw();
      swr_list_draw();

      break;

    case S_CHANGE_STEP:

      g_display.print(F("STEP: ")); 
      g_display.print((long)(g_active_band.freq_step/100000UL));
      g_display.print(F(" kHz"));

      break;

  } // screen state

  g_display.display();
}

void loop()
{
  g_timer.run();
  if (g_do_update) {
    process_display_swr();
    g_do_update = false;
  }
}

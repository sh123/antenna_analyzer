/**
 *  antenna analyzer project
 *
 **/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <si5351.h>
#include <Rotary.h>
#include <SimpleTimer.h>
#include "Wire.h"

SimpleTimer timer;
Si5351 generator;
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);
Rotary rotary = Rotary(11, 12, 13);

#define SWR_MAX            99
#define SWR_LIST_SIZE      84
#define SWR_SCREEN_HEIGHT  48
#define SWR_SCREEN_CHAR    8
#define SWR_GRAPH_HEIGHT   (SWR_SCREEN_HEIGHT - SWR_SCREEN_CHAR)
#define SWR_GRAPH_CROP     5

#define FREQ_STEP_INC      2500000ULL
#define FREQ_MAX           16000000000ULL
#define BANDS_CNT          12
#define IDX_70CM           11

enum SCREEN_STATE {
  S_FULL = 0,
  S_GRAPH,
  S_STEP
};

// band selection
struct band_t {
  uint64_t freq;
  uint64_t freq_step;
  char *band_name;
} const bands[BANDS_CNT] PROGMEM = {
  { 350000000ULL,   1000000ULL,  "80m"  },
  { 700000000ULL,   1000000ULL,  "40m"  },
  { 1010000000ULL,  1000000ULL,  "30m"  },
  { 1400000000ULL,  1000000ULL,  "20m"  },
  { 1800000000ULL,  1000000ULL,  "17m"  },
  { 2100000000ULL,  1000000ULL,  "15m"  },
  { 2480000000ULL,  1000000ULL,  "12m"  },
  { 2700000000ULL,  1000000ULL,  "CB "  },
  { 2800000000ULL,  1000000ULL,  "10m"  },
  { 5000000000ULL,  2500000ULL,  "6m "  },
  { 14400000000ULL, 2500000ULL,  "2m "  },
  { 14400000000ULL, 800000ULL,   "70c"  }
};

int band_index = 0;
struct band_t active_band;

long freq_min;
double swr_min;

// swr graph
unsigned char swr_list[SWR_LIST_SIZE];

// program state
SCREEN_STATE screen_state;
bool do_update = true;

void setup()
{
  Serial.begin(9600);

  screen_state = S_FULL;
  swr_list_clear(); 
  select_active_band_by_index(band_index);
  
  generator.init(SI5351_CRYSTAL_LOAD_6PF, 0);
  generator.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
  generator.set_freq(active_band.freq, 0UL, SI5351_CLK2);
  
  timer.setInterval(1000, process_display_swr);
  timer.setInterval(100, process_rotary_button);
  timer.setInterval(10, process_rotary);
  
  display.begin();
  display.setContrast(60);
  display.display();
  delay(1000);
  display.clearDisplay();
  display.display();
}

void swr_list_clear() {
  for (int i = 0; i < SWR_LIST_SIZE; i++) {
    swr_list[i] = 0;
  }
}

void swr_list_shift_right() {
  swr_list[0] = 0;
  for (int i = SWR_LIST_SIZE - 1; i != 0; i--) {
    swr_list[i + 1] = swr_list[i];
  }
}

void swr_list_shift_left() {
  swr_list[SWR_LIST_SIZE - 1] = 0;
  for (int i = 0; i < SWR_LIST_SIZE - 1; i++) {
    swr_list[i] = swr_list[i + 1];
  }
}

void swr_list_store(double swr) {
  int swr_graph = swr * (double)SWR_GRAPH_HEIGHT / (double)SWR_GRAPH_CROP;
  if (swr_graph > SWR_GRAPH_HEIGHT) {
    swr_graph = SWR_GRAPH_HEIGHT;
  }
  swr_list[SWR_LIST_SIZE / 2] = (unsigned char)swr_graph;
}

void swr_list_draw() {
  for (int i = 0; i < SWR_LIST_SIZE; i++) {
    if (swr_list[i] != 0) {
      display.drawFastVLine(i, SWR_SCREEN_HEIGHT - swr_list[i] + SWR_GRAPH_CROP, swr_list[i] - SWR_GRAPH_CROP, BLACK);
    }
  }
}

void grid_draw() {
  display.drawFastVLine(SWR_LIST_SIZE / 2, SWR_SCREEN_CHAR, SWR_SCREEN_CHAR / 2, BLACK);
  for (unsigned char x = 0; x <= SWR_LIST_SIZE; x += SWR_LIST_SIZE / 12) {
    for (unsigned char y = SWR_SCREEN_CHAR; y <= SWR_GRAPH_HEIGHT + SWR_GRAPH_CROP; y += SWR_GRAPH_HEIGHT / SWR_GRAPH_CROP) {
      display.drawPixel(x + 6, y + SWR_SCREEN_CHAR - 1, BLACK);
    }
  }  
}

void next_screen() {
  switch (screen_state) {
    
    case S_FULL:
      screen_state = S_GRAPH;
      break;
      
    case S_GRAPH:
      screen_state = S_STEP;
      break;
      
    case S_STEP:
      screen_state = S_FULL;
      break;
      
     default:
       break;
  }
}

void select_active_band_by_index(int index) {
  if (index < BANDS_CNT) {
    memcpy_PF((void*)&active_band, (uint_farptr_t)&bands[index], sizeof(bands[index]));
    swr_list_clear();
    swr_min = SWR_MAX;
    freq_min = active_band.freq / 100000ULL;
  }
}

double get_swr(int fwd, int rfl) {
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

void process_rotary() {
  unsigned char rotary_state = rotary.process();
  if (rotary_state) {
    if (rotary_state == DIR_CW) {
      if (screen_state == S_FULL || screen_state == S_GRAPH) {
        active_band.freq -= active_band.freq_step;
        swr_list_shift_right();
      }
      else if (screen_state == S_STEP) {
        if (active_band.freq_step - FREQ_STEP_INC > 0) {
          active_band.freq_step -= FREQ_STEP_INC;
        }
      }
    } 
    else {
      if (screen_state == S_FULL || screen_state == S_GRAPH) {
        active_band.freq += active_band.freq_step;
        swr_list_shift_left();
      }
      else if (screen_state == S_STEP) {
        active_band.freq_step += FREQ_STEP_INC;
      }
    }
    if (active_band.freq > FREQ_MAX) {
      active_band.freq = FREQ_MAX;
    }
    generator.set_freq(active_band.freq, 0ULL, SI5351_CLK2);
    do_update = true;
  }
}

void process_rotary_button() {
  
  unsigned char rotary_btn_state = rotary.process_button();
      
  switch (rotary_btn_state) {
    
    case BTN_NONE:
      break;
      
    case BTN_PRESSED:
      Serial.println("button pressed");
      break;
    
    case BTN_RELEASED:
      Serial.println("button released");
      band_index += 1;
      if (band_index > BANDS_CNT - 1) {
        band_index = 0;
      }
      select_active_band_by_index(band_index);
      generator.set_freq(active_band.freq, 0ULL, SI5351_CLK2);
      do_update = true;
      break;

    case BTN_PRESSED_LONG:
      Serial.println("button long pressed");
      next_screen();
      do_update = true;
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
  long freqKhz = active_band.freq / 100000ULL;
  
  // second harmonics on 70cm band
  if (band_index == IDX_70CM) {
    freqKhz *= 3;
  }
  double swr = get_swr(val_fwd, val_rfl);
  if (swr < swr_min) {
    swr_min = swr;
    freq_min = freqKhz;
  }
  swr_list_store(swr);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
  
  if (screen_state == S_FULL) {
    
    Serial.print(freqKhz); Serial.print(F(" "));
    Serial.print(val_fwd); Serial.print(F(" "));
    Serial.print(val_rfl); Serial.print(F(" "));
    Serial.print(swr); Serial.println(F(""));

    display.print(active_band.band_name); display.print(F(": ")); 
    display.print(freqKhz); display.println(F(" k"));
    display.print(F("SWR: ")); display.println(swr);
    display.print(F("FWD: ")); display.println(val_fwd);
    display.print(F("RFL: ")); display.println(val_rfl);
    display.println(F("MIN:"));
    display.print(swr_min); display.print(F(" ")); display.println(freq_min);
    
  } else if (screen_state == S_GRAPH) {
    
    display.print(freqKhz);
    display.print(F(" "));
    display.println(swr);
    
    grid_draw();
    swr_list_draw();
    
  } else if (screen_state == S_STEP) {
    display.print(F("STEP: ")); 
    display.print((long)(active_band.freq_step/100000UL));
    display.print(F(" kHz"));
  }
  display.display();
}

void loop()
{
  timer.run();
  if (do_update) {
    process_display_swr();
    do_update = false;
  }
} 

# antenna_analyzer
Arduino graphical/panoramic SWR 160 to 2m band antenna analyzer based on Si5351 module. 

Influenced by http://www.hamstack.com/hs_projects/k6bez_antenna_analyzer.pdf

Analyzer demonstrated quite valid results compared to factory built and calibrated devices.

Arudino Antenna Analyzer
========================

Introduction
------------
Arudino based antenna swr analyzer / plotter can be used to measure antenna
SWR from 160m band up to 2m. The limit is around 160MHz. Next peripherals are
in use:

 * Nokia screen, PCD8544
 * Rotary encoder
 * Si5351 clock generator - https://www.adafruit.com/datasheets/Si5351.pdf
 * Reflectometer/VSWR Bridge as per http://www.hamstack.com/hs_projects/k6bez_antenna_analyzer.pdf
 * You will also need a set of low pass filters to get sine wave at desired bands as SI5351 is a clock generator

Requirements:
-------------
 * Rotary encoder library modified fork - https://github.com/sh123/Rotary/tree/rotary_button
 * Adafruit PCD8544 - https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
 * Adafruit GFX - https://github.com/adafruit/Adafruit-GFX-Library
 * Si5351 library (< 2.0.0 version, just pick from Arduino library manager) - https://github.com/etherkit/Si5351Arduino
 * Simple Timer library - https://github.com/jfturcot/SimpleTimer

Supported operations:
---------------------
 * Details screen - shows various numeric parameters, such as forward/reflected signals, current frequency/band. By rotating rotary encoder user can change the frequency. By short press - change the band, by long press go to next screen.
 * Real time graph screen - shows partially updated SWR plot, plot is updated while user is changing the frequency using encoder, plot is shifted left or right depending on frequency change direction.
 * Frequency sweep screen - shows complete SWR plot, which is updated every second.
 * Frequency step change screen - enables user to change frequency step, which affects both rotary encoder changes and sweep screen.

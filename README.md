# antenna_analyzer
Arduino graphical/panoramic SWR 160 to 1.25m band antenna analyzer based on Si5351 module. 

Influenced by http://www.hamstack.com/hs_projects/k6bez_antenna_analyzer.pdf

Output power from si5351 is around 10dBm (10mW).

Analyzer demonstrated quite valid results compared to factory built and calibrated devices.

Note, that it is not a complete VNA, it is based on resistive reflectometer and does not measure
complex part of impedance.

Arudino Antenna Analyzer
========================

Introduction
------------
Arudino based antenna swr analyzer / plotter can be used to measure antenna
SWR from 160m band up to 1.25m. The limit from Etherkit Si5351 library is 225MHz.

Next peripherals are in use:

 * Nokia 5110 screen, PCD8544 - https://www.sparkfun.com/datasheets/LCD/Monochrome/Nokia5110.pdf
 * Si5351 clock generator - https://www.adafruit.com/datasheets/Si5351.pdf
 * Reflectometer/VSWR Bridge as per http://www.hamstack.com/hs_projects/k6bez_antenna_analyzer.pdf
 * Rotary encoder
 * You will also need a set of low pass filters to get sine wave at desired bands as si5351 is a clock generator

Requirements:
-------------
 * Rotary encoder library modified fork - https://github.com/sh123/Rotary/tree/rotary_button
 * Simple Timer library - https://github.com/jfturcot/SimpleTimer
 * Adafruit PCD8544 (from Arduino library)- https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
 * Adafruit GFX (from Arduino library) - https://github.com/adafruit/Adafruit-GFX-Library
 * Etherkit Si5351 (version 2+ from Arduino library) - https://github.com/etherkit/Si5351Arduino

Supported operations:
---------------------
 * Details screen - shows various numeric parameters, such as forward/reflected signals, current frequency/band. By rotating rotary encoder user can change the frequency. By short press - change the band, by long press go to next screen.
 * Real time graph screen - shows partially updated SWR plot, plot is updated while user is changing the frequency using encoder, plot is shifted left or right depending on frequency change direction.
 * Frequency sweep screen - shows complete SWR plot, which is updated approximately every second.
 * Frequency step change screen - enables user to change frequency step, which affects both rotary encoder changes and sweep screen.

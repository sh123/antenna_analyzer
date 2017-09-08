# antenna_analyzer
Arduino graphical SWR 160-2m bands antenna analyzer based on Si5351 module.

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
 * Reflectometer as per http://www.hamstack.com/hs_projects/k6bez_antenna_analyzer.pdf
 * You will also need a set of low pass filters to get sine wave at desired bands as SI5351 is a clock generator

Requirements:
-------------
 * Rotary encoder library modified fork - https://github.com/sh123/Rotary/tree/rotary_button
 * Adafruit PCD8544 - https://github.com/adafruit/Adafruit-PCD8544-Nokia-5110-LCD-library
 * Adafruit GFX - https://github.com/adafruit/Adafruit-GFX-Library
 * Si5351 library (< 2.0.0 version, just pick from Arduino library manager) - https://github.com/etherkit/Si5351Arduino
 * Simple Timer library - https://github.com/jfturcot/SimpleTimer

# SC22 demo

## Introduction

This demo application reads out the Sensirion SCD40 and SHT40 sensors, optionally displays the values on the local device' display, and optionally uploads the data to [Adafruit IO](https://io.adafruit.com).

## Prerequisites

### Base setup
- Arduino
- SCD40 breakout board
- SHT40 breakout board

### For cloud upload 
- Adafruit IO account
- Arduino with Wifi functionality
- Arduino needs to be compatible with Adafruit IO Wifi setup code

### For local display
- Adafruit_ST7789 compatible display


## Local Preparation
- Check out this repository
- Open `sc22.ino`
- Adapt `config.h` to your network settings, Adafruit IO user name/key
- Compile and run

## Prepare Adafruit IO

### Setup Feeds
Visit [Adafruit IO](https://io.adafruit.com) and log in to your account. Then, select "Feeds" > "View all" from the navigation, which will give you a list of existing feeds. On that webpage, create three feeds as such:

|Feed name|Feed key|
|----------|-------|
|CO2|sc22-co2-1|
|Relative Humidity|sc22-rh-1|
|Temperature|sc22-t-1|

While the feed name doesn't need to be exactly as shown, please note that if you change the feed key, you will need to update the code (see below, "Demo Configuration")

### Setup dashboard
Visit [Adafruit IO](https://io.adafruit.com) and log in to your account. Then, select "Dashboards" > "View all" from the navigation, which will give you a list of existing dashboards. On that webpage, create a new dashboard for this demo (name of the dashboard does not matter, so choose something that makes sense to you).

You can setup your dashboard in any way you like. We would recommend the following as a starting point:
- Add three "gauge" blocks; connect them to the three feeds created in the section above
- Create two "line chart" blocks; connect the first to CO2, and the second to RH and T
- Recommended: for the initial setup, it is recommended to add a "stream" type block too, which will help you see incoming events

## Demo Configuration
TBD


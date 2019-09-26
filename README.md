# VL53L0X-MQTT-Tripwire
Arduino sketches used to create a single or dual tripwire for use with IoT software

##  MQTT Broker

You must have an MQTT broker and a working arduino environment capable of compiling and programming an ESP8266.
There is a guide here for setting up Mosquitto which seems to work well: https://www.home-assistant.io/addons/mosquitto/
There is no special reason for using Mosquitto, it's just the one I am running and it's easy to set up.

## Versions

There are two versions of the sketch, for circuits that connect either one or two VL53L0x units.  The version with two
lasers has the additional benefit of being able to report a direction of travel when both are triggered.

## To do

- Add circuit diagram for dual version (it's literally another unit on the i2c bus and direct connections to the reset
pins on the laser module that allow them to be programmed with i2c assresses.  Id's on these chips are volatile and
lost with every reboot.

## Future versions

- The next version of the sketch / circuit(s) will support calibration hardware that will allow users to interactively
set the distance to monitor to allow for specific distance ranges (such as the 100 millimeters covering a letterbox).

- Calibration to be saved to flash to allow reboot.  Calibration to be set by MQTT message if enabled at compile time.

## Issues

### Compile issues

When you compile you may get a #pragma compilation error message.  Just ignore it, as long as the sketch compiles
it's fine.  It looks like it might be caused by several of the libraries trying to use stdin when it only needs to be 
included once.  If anyone knows how to get rid of this, please let me know.

### VL53L0X errors on upload

If you find you upload and the board starts cycling every few seconds just power cycle it.  Sometimes when you upload
the sketch to the ESP8266 the VL53L0X gets into an error state and it needs to be cycled.  The reset button won't help
here, you need to remove the power for a second.  It sometimes happens after uploading the sketch.

### Further information

There's a lot of information about these out there, but these two video's are excellent introductions.
https://www.youtube.com/watch?v=gGtD93wb7xI - Shows the beam spread filmed with an infrared camera.
https://www.youtube.com/watch?v=v8BYS-LokaE - Anreas Speiss (The guy with the Swiss Accent!)

You can buy these everywhere for a few dollars if you can wait for the post from China.  Adafruit board is nice
and probably will arrive quicker.  I'm generally broke so these all developed with the cheapo boards.

### Changelog

0.4 Update to secrets.h to move port from main sketch
0.3 Moved secrets out of sketch (mqtt user / password etc)
0.2 Removed legacy led on/off from original example sketch 
0.1 First upload

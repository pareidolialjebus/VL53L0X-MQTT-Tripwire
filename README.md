# VL53L0X-MQTT-Tripwire V0.5
Arduino sketches used to create a single or dual tripwire for use with IoT software

## Dependancies

### Arduino IDE
For this project I've used the arduino IDE and installed ESP8266 boards.

### Libraries
Adafruit L53L0x Library found here: https://github.com/adafruit/Adafruit_VL53L0X and the
PubSubClient library by Nick O'Leary found here: https://github.com/knolleary/pubsubclient.

They handle the heavy lifting with the laser chip and MQTT respectively.

### MQTT Broker

You must have an MQTT broker and a working arduino environment capable of compiling and programming an ESP8266.
There is a guide here for setting up Mosquitto which seems to work well: https://www.home-assistant.io/addons/mosquitto/
There is no special reason for using Mosquitto, it's just the one I am running and it's easy to set up.

## Versions

There are two versions of the sketch, for circuits that connect either one or two VL53L0x units.  The version with two
lasers has the additional benefit of being able to report a direction of travel when both are triggered.

## Dual Version

Currently this is the version being actively developed.  When more D1 Mini's and more V53L0x's arrive in the post, I'll get back to work on the single beam version.  MQTT messages send to the unit have the effect of putting it into range calibration.

- Message "0" - Tells the unit to exit range calibration.  Any changes are set.
- Message "1" - Sets calibration to set laser 1 max.  This is the longest range it will consider as a trigger.  By default 2000 (or 2 metres).
- Message "2" - Sets calibration to set laser 2 max.  This is the longest range it will consider as a trigger.  By default 2000 (or 2 metres).
- Message "3" - Sets calibration to set laser 1 minimum range.  This is the shortes range it will consider as a trigger.  By default 0.
- Message "4" - Sets calibration to set laser 2 minimum range.  This is the shortes range it will consider as a trigger.  By default 0.

Use these modes to set up your tripwire so it's only detecting motion at the exact points you want.  Useful for using to monitor small spaces like a mail slot while avoiding the dog that likes to dance around under it and bark at the postman.

Ignoring this ability means it functions the same as version 0.4 (over the full range of a few millimetres to 2 metres).

## To do

- Range calibration to be saved to flash to allow reboot without losing settings.

## Circuit
A copy of the original Adafruit fritzing sketch with an embarrassing yet functional circuit diagram of the dual version are available here: https://i.imgur.com/GOsAUgk.jpg

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

- 0.5 Added MQTT control to set range.
- 0.4 Update to secrets.h to move port from main sketch
- 0.3 Moved secrets out of sketch (mqtt user / password etc)
- 0.2 Removed legacy led on/off from original example sketch 
- 0.1 First upload

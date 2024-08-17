# LockBox
A software for creating a remotely controlled LockBox using ESP32, a solenoid lock and other sensors.

Now with Chaster.app integration

![LockBox Picture](./renders/photo.jpeg)

Fot this project I used:
 * Adafruit Huzzah32
 * Adafruit monochrome epaper display feather wing
 * 12v solenoid lock (you may use the 5v version available on Adafuit shop but will need modifications in the code or on the lock)
 * 5v to 12v converter (if using 12v lock)
 * DS3231 RTC module from AliExpress
 * Simple Relay
 * End of travel sensor or something similar or button (I used a button with integrates spring salvaged from an old device)
 * Push button
 * A LED (integrated in the button in my case)

## Features
 * Web page to add/remove time directly one the box remotely
 * PIN for preventing unauthorized people (ie the wearer) from sending command to the box
 * 4 States: Locked, Locked with timer, Free, Once (free to open only once and will relock after)
 * Button on the box to open the lock
 * Screen to dispay status
 * Can be battery powered (using Huzzah32 internal battery controller) to continue updating display and checking state for some time when unplugged (cannot open the box on battery, it needs to be plugged)
 * Integration with chaster.app :
   * The box can be linked to a Chaster.app account
   * In this mode, the state box reflects the state of the first unarchived lock of the account
   * By default, the screen displays the state, the remaining time (or stars if timer is hidden) and the lock name
   * When a Chaster.app lock exists, manual lock interraction are disabled
   * If the Chaster.app lock is in state unlocked or temporary unlocked, the button can open the box
   * If the Chaster.app lock is not unlocked or temporary unlocked but a temporary unlock is possible, a long press (more than 10 seconds) will request a temporary unlock to Chaster.app and open the box
   * When temporary unlocked, the screen will display the remaining opening time allowed before getting a penalty
   * Automatically relock on chaster when closing the box during a temporary opening
   * When a verification code is generated on Chaster.app for a new verification picture, the code is displayed in big on the screen so the wearer can take the picture with the box and doesn't need to write codes anymore 

## Build
 * Copy the file lockbox.h.dist to lockbox.h and set your wifi credentials
 * Compile and upload the code using Arduino IDE
 * Upload the web file to the ESP32 using the method described here (Arduino IDE 1.8 needed): https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/
 * Have fun

## Schema
Here you can find an example of a schema for this. 

The e-paper display is not represented, and the component may not be accurate.

![LockBox Schema](./renders/schema.png)

NOTE: Using the 5v to 12v converter requires a 3A power supply, you may want to power it using 12v and have a 12v to 5v converter for the Microcontroller

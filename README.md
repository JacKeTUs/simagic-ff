# Force feedback support for Simagic steering wheels

Linux module driver for Simagic driving wheels.

Simagic wheels is basically DirectInput wheels without 0xa7 descriptor (effect delay). Current in-tree driver does not take into account the lack of descriptor and fully rejects FFB support.
In that repository - copy of pidff driver from 6.3 kernel with some patches (removed 0xa7 support), and some changes around infinite length effects (like that https://github.com/berarma/ffbtools/issues/26)

And that's basically it

## What devices are supported?
### Bases:
1. Simagic Alpha Mini
2. Simagic Alpha
3. Simagic Alpha U

Just because they have identical VendorID/ProductID

### Wheel rims:
Tested on GT1, and all buttons works perfectly.


## What works?
1. FFB (all effects from DirectInput FFB specifications)
2. All inputs (wheel axis, buttons)


## What does not work?
1. Wheel base setup through proprietary Simagic soft (yet)
  1. Setting center
  2. Setting wheel range from soft
  3. Setting wheel base settings from soft
  4. Firmware updates
  5. Screen in FX-Pro wheel
  6. All telemetry functions (Shift lights, etc)
  7. Setting gain
  8. etc...
I'm planning to write another soft, which will copy functionality from AlphaManager and SimPro Manager. Or set interfaces for settings for Oversteer, idk yet


## How to use that driver?
1. Install linux-headers-$(uname -r)
2. Clone repository
3. `make`
4. `sudo insmod hid-simagic-ff.ko`


## How to get rid of deadzone?
You need to calibrate wheel using standart soft
```
evdev-joystick --evdev /dev/input/by-id/usb-STMicroelectronics_alpha_feedback_wheel_simulator_*-event-joystick --axis 0 --deadzone 0
```

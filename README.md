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

Just because they have identical VendorID/ProductID (`0x0483` `0x0522`)

### Wheel rims:
Tested on GT1, and all buttons works perfectly. I suspect that all rims will work, except of FX-Pro display.

## What works?
1. FFB (all effects from device descriptor, with [caveats](#known-issues-with-the-firmware))
2. All inputs (wheel axis, buttons)
3. Setup through proprietary Simagic soft (with [some tweaking](#how-to-set-up-a-base-parameters))

## What does not work?
1. Telemetry functions (Shift lights, FX-Pro display), mostly because telemetry works only with proprietary soft, which can't get access to shared memory chunks from games.
2. `Firmware Update` function. Use Windows PC or Windows VM at the moment.
3. Driver only detects wheels in "Normal mode". Compatibility mode changes VID/PID of the wheelbase to vJoy defaults (`0x1234` `0xbead`)

## How to use that driver?
You can install it through DKMS or manually.
### DKMS
1. Install `dkms`
2. Clone repository to `/usr/src/simagic-ff`
3. Install the module: 
`sudo dkms install /usr/src/simagic-ff`
4. Update initramfs:
`sudo update-initramfs -u`
5. Reboot

To remove module:
`sudo dkms remove simagic-ff/<version> --all`
### Manually 

1. Install `linux-headers-$(uname -r)`
2. Clone repository
3. `make`
4. `sudo insmod hid-simagic-ff.ko`

To unload module:
`sudo rmmod hid_simagic_ff`


## How to set up a base parameters?

You can do it through AlphaManager or SimPro Manager with Wine. You need to tweak Wine prefix for them.

AlphaManager works pretty good, but it recognizes only "old" bases (made before ~June 2022) with old firmware (max v108). SimPro 1.x launches, but graphical interface is pretty janky and really slow, i don't recommend using it. SimPro 2.x before 0.116 works with new firmware (max v159) and pretty useful.


That soft uses hidraw to set up a base. You need to create `udev` rule for allow access to hidraw device:
```
echo 'KERNEL=="hidraw*", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="0522", MODE="0660", TAG+="uaccess"' | sudo tee /etc/udev/rules.d/11-simagic.rules

udevadm control --reload-rules && udevadm trigger
```

Then you need to force Simagic soft to use hidraw, not SDL, to find devices:
1. Create new Wine prefix for them:

      `mkdir ~/simagic-wine`
2. Launch regedit in prefix:

      `WINEPREFIX=$HOME/simagic-wine wine regedit`
3. Create two keys in
  `HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\winebus`:

    * `DisableInput` (DWORD) - set to `1`;
    * `Enable SDL` (DWORD) - set to `0`; (yes, variable name contains  space)
4. Now you can launch soft through that WINEPREFIX:

    `WINEPREFIX=$HOME/simagic-wine wine AlphaManager.exe ` - launch your SimPro/AlphaManager from installation directory.


I'm planning to write another soft, which will copy functionality from AlphaManager and/or SimPro Manager. Or set interfaces for settings for [Oversteer](https://github.com/berarma/oversteer), idk yet.

## Known issues with the driver

1. ~~If you use `WheelCheck.exe` (iRacing .exe to check FFB on the wheel), it sends some bizare parameters (like 2147483647 in saturation coefficient) to driver - and hid-core rejects it with kernel warning. It does not happening in games, and it's unclear if it is a bug in firmware, or we need to fix pidff driver for it.~~ Fixed - it was unclamped PID_LOOP_COUNT, bug in pidff. 
2. Force Feedback clipping. Output queue could become full, and your kernel log will fill up with `output queue full` messages. I could reproduce it with spamming `WheelCheck.exe` parameters, i did not see it in games at all (maybe time will tell).
3. Firmware update does not work. Please use Windows machine or Windows VM for any firmware updates
4. Wheel firmware update does not work. Please use Windows machine or Windows VM for any firmware updates

## Known issues with the firmware 
Here is some issues, which is also a case for Windows
1. Base firmware (tested v108 and v159, old revision) does not use parameter "Center Point Offset (0x60)" in Conditional Effect. For example - every "Spring" effect will center the will, there is no way now to "spring" the wheel to the left/right. It could be checked through `WheelCheck.exe`. Report to Simagic developers in their official Discord: [link](https://canary.discord.com/channels/1111167845435453482/1111178030031831042/1122902054747250698)
2. If you try to change range of the wheel when it is outside requested range - feedback will dissapear completely. Reboot and reconnect base to fix it.
3. With base firmware greater than v108 some wheel rim functions like GT1 - "Set Rotation Angle" does not work.
4. GT1 - with wheel firmware 3242 (latest) setting LED mode (slow/fast flashing / off / on) does not work
5. Firmware v169 and SimPro2.0.116 introduced proprietary protocol for communicating to the wheelbase and does not use standart directinput ffb reports. Please, avoid firmware v169 and SimPro 2.0.116 for the time being.

## DISCLAIMER
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# Force feedback support for Simagic steering wheelbases

Linux module driver for Simagic driving wheelbases.

This driver adds support for FFB on Simagic new firmware (post fw v159 / Simpro v2).

## What devices are supported?
### Bases:
Wheelbase must be on new-enough firmware, post fw v159. If you purchased new wheelbase after ~2024, you already should have got new one.
1. Simagic Alpha Mini
1. Simagic Alpha
1. Simagic Alpha U
1. Simagic Evo series

### Old firmware
Wheelbases using old firmware (pre fw v159), with PID descriptor exposed, should work out of the box in recent enough kernels (>6.12). No need to install this driver for old firmware.
1. Simagic M10

### Wheel rims, passthrough:
There is no special handling for devices which are connecting to the wheelbase and reports as a one whole device, this works out of the box.
Was tested on GT1, all buttons worked perfectly. I suspect that all rims/pedals will work, except of FX-Pro display.
Using new Alpha QR Passthrough, which is just USB connection, is out of scope of the driver and should work out of the box too.

## What works?
1. FFB (all effects which are present on Windows, but with [caveats](#known-issues-with-the-firmware))
1. All inputs (wheel axis, buttons)
1. Setup through proprietary Simagic soft (with [some tweaking](#how-to-set-up-a-base-parameters))
1. There are sysfs entries, which are exposed in this driver. You can use them to setup your wheelbase. Look into `/sys/bus/hid/drivers/simagic-ff/XXXX:XXXX:XXXX.XXXX/` folder.

## What does not work?
1. Telemetry functions (Shift lights, FX-Pro display). They are out of scope of this driver (which focuses on bringing FFB native support). Telemetry works only with proprietary soft, which should get access to shared memory chunks from games. They will work if you setup Simpro manager in same prefix as the game, and they will work independently of driver installed.
1. `Firmware Update` function. Use Windows PC or Windows VM at the moment.
1. Driver only detects wheels in "Normal mode". Compatibility mode changes VID/PID of the wheelbase to vJoy defaults (`0x1234` `0xbead`)

## How to use that driver?
You can install it through DKMS or manually.
### DKMS
1. Install `dkms`
1. Clone repository to `/usr/src/simagic-ff`
1. Install the module: 
  `sudo dkms install /usr/src/simagic-ff`
1. Update initramfs:
  `sudo update-initramfs -u`
1. Reboot

To remove module:
`sudo dkms remove simagic-ff/<version> --all`
### Manually 

1. Install `linux-headers-$(uname -r)`
1. Clone repository
1. `make`
1. `sudo insmod hid-simagic-ff.ko`

To unload module:
`sudo rmmod hid_simagic_ff`


## How to set up a base parameters?

You can do it through AlphaManager or SimPro Manager with Wine. You can use Steam or custom Wine prefix for them.

#### Prerequisites:
You need to install udev rules, which will open hidraw descriptors to the wheelbase, wheels, pedals.

```
# Simagic devices with 0x3670 VID (GT Neo, Evo wheelbases)
echo 'KERNEL=="hidraw*", ATTRS{idVendor}=="3670", MODE="0666", TAG+="uaccess"' | sudo tee -a /etc/udev/rules.d/11-simagic.rules
# Simagic devices with 0x0483 VID (Older wheelbases)
echo 'KERNEL=="hidraw*", ATTRS{idVendor}=="0483", MODE="0666", TAG+="uaccess"' | sudo tee -a /etc/udev/rules.d/11-simagic.rules

udevadm control --reload-rules && udevadm trigger
```

#### Steam method
1. Download Simpro Setup.exe
1. Add Setup.exe to the Steam as Non-Steam Game
1. Force Proton Experimental
1. Set launch option as `PROTON_ENABLE_HIDRAW=1 %command%`
1. Launch the tool through steam, let it install itself into prefix and install every dependency it may need
1. After successfull setup, in the steam "game" settings change .exe to the path of the installed simpro (you should find it yourself, it should be somewhere in `~/.local/share/Steam/steamapps/compatdata/*someid*/pfx/drive_c/Program Files (x86)/simpro/bin/`, search by recently created folders). Alternatively, use `protontricks -l` to find id of Non-Steam shortcut.
1. Change exe and game dir accordingly
   exe:
     `~/.local/share/Steam/steamapps/compatdata/*someid*/pfx/drive_c/Program Files (x86)/simpro/bin/simpro.exe`
   dir:
     `~/.local/share/Steam/steamapps/compatdata/*someid*/pfx/drive_c/Program Files (x86)/simpro/bin`
1. Launch the tool again, and check if everything detected.

#### Custom Wine prefix
You need to force Simagic soft to use hidraw, not SDL, to find devices:
1. Create new Wine prefix for them:
      `mkdir ~/simagic-wine`
1. Launch regedit in prefix:
      `WINEPREFIX=$HOME/simagic-wine wine regedit`
1. Create two keys in
  `HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\winebus`:
    * `DisableInput` (DWORD) - set to `1`;
    * `Enable SDL` (DWORD) - set to `0`; (yes, variable name contains  space)
1. Now you can launch soft through that WINEPREFIX:

    `WINEPREFIX=$HOME/simagic-wine wine AlphaManager.exe ` - launch your SimPro/AlphaManager from installation directory.

#### SYSFS entries

There are multiple sysfs entries, which could be used to setup the wheelbase parameters. Look into them here:

```
/sys/bus/hid/drivers/simagic-ff/XXXX:XXXX:XXXX.XXXX/
```

## Known issues with the driver

1. Force Feedback clipping. Output queue could become full, and your kernel log will fill up with `output queue full` messages. I could reproduce it with spamming `WheelCheck.exe` parameters, i did not see it in games at all (maybe time will tell).
1. Firmware update does not work. Please use Windows machine or Windows VM for any firmware updates
1. "Feel" of the FFB could be slightly different
1. With firmware updates this driver could stop working. But in our observation, old ids for effects and protocol itself didn't changed at all from first attempts at this, so...


## Known issues with the firmware 
Here is some issues, which is also a case for Windows
1. Base firmware does not use parameter "Center Point Offset (0x60)" in Conditional Effect. For example - every "Spring" effect will center the will, there is no way now to "spring" the wheel to the left/right. It could be checked through `WheelCheck.exe`. Report to Simagic developers in their official Discord: [link](https://canary.discord.com/channels/1111167845435453482/1111178030031831042/1122902054747250698).
1. If you try to change range of the wheel when it is outside requested range - feedback will dissapear completely. Reboot and reconnect base to fix it. Now it could be replicated only with using Sysfs entries, so be careful. Simpro manager just blocks frontend button if you try to change range when wheel is not centered.
1. With base firmware greater than v108 some wheel rim functions like GT1 - "Set Rotation Angle" does not work.
1. GT1 - with wheel firmware 3242 (latest) setting LED mode (slow/fast flashing / off / on) does not work

## DISCLAIMER
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
1. FFB (all effects from DirectInput FFB specifications)
2. All inputs (wheel axis, buttons)
3. Setup through proprietary Simagic soft (with [some tweaking](#how-to-set-up-a-base-parameters))

## What does not work?
1. Telemetry functions (Shift lights, FX-Pro display)
Mostly because telemetry works only with proprietary soft, which can't get access to shared memory chunks from games.
2. I did not test `Firmware Update` function. Use it on your own risk.

## How to use that driver?
You can install it through DKMS or manually.
### DKMS
1. Install `dkms`
2. Clone repository to `/usr/src/simagic-ff`
3. Install the module: 
`sudo dkms install /usr/src/new-lg4ff`
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

AlphaManager works with only "old" bases (made before ~June 2022). SimPro 1.x launches, but graphical interface is pretty janky. SimPro 2.x works, but does not see current angle of steering wheel.

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

    `WINEPREFIX=$HOME/simagic-wine wine AlphaManager.exe`


I'm planning to write another soft, which will copy functionality from AlphaManager and/or SimPro Manager. Or set interfaces for settings for Oversteer, idk yet.

## Known issues

1. When using `AlphaManager` - if you try to quickly change steering angle and/or another parameter, steering wheel becomes 'empty' and without any feedback - reconnecting the base to PC fixes it. It's unclear if it is a bug in AlphaManager or in the firmware (which we, sadly, can't fix), or in the driver
2. If you use `WheelCheck.exe` (iRacing .exe to check FFB on the wheel), it sends some bizare parameters (like 2147483647 in saturation coefficient) to driver - and hid-core rejects it with kernel warning. It does not happening in games, and it's also unclear if it is a bug in firmware, or we need to fix pidff driver for it.
3. Wheel firmware does not use Spring Center parameter. That is also a case on Windows
4. Force Feedback clipping. Output queue could become full, and your kernel log will fill up with `output queue full` messages.
5. I did not test `Firmware update` function. Use it on your own risk.

## DISCLAIMER
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
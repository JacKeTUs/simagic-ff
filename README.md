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
You can install it through DKMS or manually.
### DKMS
1. Install `dkms`
2. Clone repository to /usr/src/simagic-ff
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


## DISCLAIMER
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
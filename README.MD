# Magic Automatic Fake User Input
MAFU-I is a plugin that lets you record the input you provide to your PSVita and play it back whenever you need to.
The capture will be saved in `ur0:/data/MAFU-I/capture.dat`

## Usage
- Press `R + CIRCLE + LEFT` to capture, press it again to save
- Press `R + CIRCLE + RIGHT` to playback, press it again to stop early

## Installation
1. Copy `MAFUinput.skprx` to your PSVita
2. Add `MAFUinput.skprx` to taiHEN's config (`ur0:/tai/config.txt` or `ux0:/tai/config.txt`):
```
*KERNEL
ur0:tai/MAFUinput.skprx
```
3. Reboot your PSVita.

## Build
- Install [DolceSDK](https://github.com/DolceSDK/ddpm)
- Run `cmake . && make`

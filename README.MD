# Magic Automatic Fake User Input
MAFU-I is a plugin that lets you record the input you provide to your PSVita and play it back whenever you need to.\
The capture will be saved in `ur0:/data/MAFU-I/capture.dat`

## Usage
- Press `R + CIRCLE + LEFT` to capture, press it again to save
- Press `R + CIRCLE + RIGHT` to playback, press it again to stop early

MAFU-I provides an integration with [déjàvu](https://github.com/TheOfficialFloW/dejavu)
If `dejavu.skprx` is present, two more shortcuts will be available
- Press `R + TRIANGLE + LEFT` to save a snapshot
- Press `R + TRIANGLE + RIGHT` to load a snapshot

Using these two shortcuts, playback and capture will resume from the correct after loading a snapshot.\
This makes a very crude speedrunning tool.

### What's supported?
The plugin can currently capture data from digital buttons, analog sticks and both the touch panels (including touch force).\
Analog button data should also be supported (via ds3/4) but it hasn't been tested.

Sensor data is still unsupported.
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

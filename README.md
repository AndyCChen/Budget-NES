# NES emulator  

BudgetNES is a NES emulator I learned to write after finishing the [Chip8](https://github.com/AndyCChen/Chip8) interpreter. This emulator features all the official 6502
cpu opcodes with a few of the illegal opcodes. PPU emulation for graphics and APU for decent sounding audio generation. This is definitely
not the most accurate emulation of the NES but it should be able to run most popular games just fine. Implementing the PPU and APU almost made me want to quit,
but I continued on and I am pretty happy with how it turned out.

- [X] 6502 cpu implementation
- [X] Mapper 0,1,2,4,7,9 implementation
- [X] basic ppu implementation that can draw background tiles
- [X] ppu sprite rendering/evaluation
- [X] input handling
- [X] audio processing unit implementation to have sound
- [X] more mappers
- [X] debug gui  

 <br>

https://github.com/user-attachments/assets/b228ad42-d2d4-4d51-8d53-09aba6c1b9e9

![Alt text](/res/dragonWarrior3.png "Dragon Warrior 3")

![Alt text](/res/megaMan2.png "mega mag 2")

## Build/Install

Build with CMake.

### Dependencies
All dependencies are included either as submodules or in the libs folder except for sdl2 which can be easily installed via a 
package such as [Homebrew](https://brew.sh/) for mac or [vcpkg](https://vcpkg.io/en/) on windows.
* [cglm](https://github.com/recp/cglm)
* [cimgui](https://github.com/cimgui/cimgui)
* [SDL2](https://www.libsdl.org/)
* [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended)
* [Blip_Buffer](https://www.slack.net/~ant/libs/audio.html#Blip_Buffer)

```
git clone --recurse-submodules https://github.com/AndyCChen/Budget-NES.git
```

### Windows
Using vcpkg for visual studio.

```bash
vcpkg install sdl2
```

Remember to set path to vcpkg toolchain file.
```bash
cmake -G "Visual Studio 17 2022" -S ./ -B build -DCMAKE_TOOLCHAIN_FILE="path_to_vcpkg\scripts\buildsystems\vcpkg.cmake"
```

Use visual studio to build the .sln file located in the build directory.  

Alternatively you can fully use CMake + ninja (included with visual studio) without having to use MSbuild
and the .sln files. This is my prefered method. Just make sure to set the path to the vcpkg toolchain file under configurations.

![Alt text](/res/toochain_path.png "toolchain_path")

Or integrate vcpkg with visual studio and it will use the path to the vcpkg toolchain file automatically.
```
vcpkg integrate install
```

### Mac
Using homebrew on mac.

```bash
brew install sdl2
```

```bash
cmake -G "Unix Makefiles" -S ./ -B build
cd build
make
```

## Initial attempts at PPU graphics rendering
Here were my initial tries at trying to get the ppu to at least render
the background tiles of the menu screens of the nestest and donkey kong rom.
As you can see not great... The individual graphics tiles look fine. 
We can see the correct pixel tiles being rendered with the right colors but they are all
over the place. Why?!?

![Alt text](/res/nestest-bug.png "corrupted nestest title")
![Alt text](/res/donkey-kong-bug.png? "corrupted donkey kong title")

The issue was nametable corruption. When the program for both donkey kong and nestest begins initial
execution it writes data into the nametables (ppu vram) to determine what tiles get displayed. This process
can take many CPU clock cycles and in general any PPU memory accesses from the CPU should be done during VBLANK. This is the
period of time in between visible frames where the NES stops rendering (no internal memory accesses are made by the PPU).
Writing to the nametables when the PPU is busy rendering graphics will cause corruption as the rendering process depends on 
many internal registers, accessing the PPU can change the state of these registers which will lead to visible corruption of the display
as seen in the screenshots. On initial power up the PPU's rendering is actually turned off which is something I completely overlooked. This resulted in a situation where
the program expected rendering to be disabled while in reality my emulator was still busy rendering. What should happen is rendering should be off
initially, this allows the CPU to spend as many clock cycles as needed to write data into the nametables without corrupting the display as the PPU
is not rendering. Once the cpu is finished sending this data into vram, it can then toggle rendering back on via a write to the PPU port located at address $2002.
Here is what those title screen should look like. As you can see the nametables are no longer corrupted!

![Alt text](/res/nestest-title.png "nestest title")
![Alt text](/res/donkey-kong-title.png "donkey kong title")

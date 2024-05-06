# NES emulator  

Learning how to write a emulator for the NES console. 

- [X] 6502 cpu implementation
- [X] Mapper 0 implementation
- [X] basic ppu implementation that can draw background tiles
- [ ] ppu sprite rendering/evaluation
- [ ] input handling
- [ ] audio processing unit implementation to have sound
- [ ] more mappers
- [ ] debug gui

## Initial attempts at PPU graphics rendering
Here were my initial tries at trying to get the ppu to at least render
the background tiles of the menu screens of the nestest and donkey kong rom.
As you can see not great... The individual graphics tiles look fine. 
We can see the correct pixel tiles being rendered with the right colors but they are all
over the place. Why?!?

![Alt text](/res/nestest-bug.png?raw=true "corrupted nestest title")
![Alt text](/res/donkey-kong-bug.png?raw=true "corrupted donkey kong title")

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

![Alt text](/res/nestest-title.png?raw=true "nestest title")
![Alt text](/res/donkey-kong-title.png?raw=true "donkey kong title")

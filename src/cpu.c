#include <stdint.h>
#include <stdio.h>

#include "../includes/cpu.h"
#include "../includes/bus.h"

CPU_6502 cpu;

void cpu_reset(void)
{
   cpu.ac = 0;
   cpu.X = 0;
   cpu.Y = 0;
   cpu.sp = 0xFF;
   cpu.status_flags = 0;
   cpu.pc = 0xC000;
}

void cpu_fetch_decode_execute(void)
{
   /*
   most instructions have the form aaabbbcc where
   aaa and cc bits form the opcode and
   bbb form the addressing mode
   */
   uint8_t instruction = bus_read(cpu.pc);

   uint8_t aaa = ( instruction & 0xE0 ) >> 0x3;
   uint8_t cc = instruction & 0x3;
   uint8_t opcode = aaa | cc;
   
   switch (opcode)
   {
   case 0xFF:
      /* code */
      break;
   
   default:
      break;
   }
}
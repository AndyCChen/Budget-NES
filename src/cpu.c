#include <stdint.h>
#include <stdio.h>

#include "../includes/cpu.h"
#include "../includes/bus.h"
#include "../includes/log.h"
#include "../includes/instructions_6502.h"

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

/**
 * Fetches, decodes, and executes a instruction.
 * Will also cycle count the number of cycle it takes to execute the instruction
*/
void cpu_fetch_decode_execute(void)
{
   // fetch

   uint8_t opcode = bus_read(cpu.pc);

   // decode 

   instruction_decode(opcode);

   // execute
   uint8_t cycles_to_count = instruction_execute(opcode);
   //printf("cycles: %d\n", cycles_to_count);

   // todo: count cycles
}
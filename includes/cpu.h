#ifndef CPU_H
#define CPU_H

#include <stdint.h>

typedef struct {
   uint8_t ac;  // accumulator
   uint8_t X;   // x index register
   uint8_t Y;   // y index register
   uint8_t sp;  // top down stack pointer at locations 0x0100 - 0x01FF
   uint16_t pc; // program counter

   // cpu status flags
   // 7th bit - negative
   // 6th bit - overflow
   // 5th bit - reserved flag 
   // 4th bit - break
   // 3rd bit - decimal
   // 2nd bit - interrupt
   // 1st bit - zero
   // 0th bit - carry
   uint8_t status_flags;
} CPU_6502;

extern CPU_6502 cpu;


void cpu_reset(void);
void cpu_fetch_decode_execute(void);

#endif
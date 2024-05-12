#ifndef INSTRUCTIONS_6502_H
#define INSTRUCTIONS_6502_H

typedef struct cpu_6502_t
{
   size_t cycle_count;

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
} cpu_6502_t;

void cpu_emulate_instruction(void);
void cpu_reset(void);
void cpu_init(void);
void cpu_IRQ(void);
void cpu_NMI(void);
void cpu_tick(void);
cpu_6502_t* get_cpu(void);

#endif

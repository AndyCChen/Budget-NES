#ifndef INSTRUCTIONS_6502_H
#define INSTRUCTIONS_6502_H

#include <stdbool.h>

typedef struct cpu_6502_t
{
   bool is_processing_nmi;
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

typedef enum address_modes_t
{
   IMP, // implied
   ACC, // accumulator
   IMM, // immediate
   ABS, // absolute
   XAB, // X-indexed absolute
   YAB, // Y-indexed absolute
   ABI, // absolute indirect
   ZPG, // zero page
   XZP, // X-indexed zero page
   YZP, // Y-indexed zero page
   XZI, // X-indexed zero page indirect
   YZI, // Zero Page indirect Y indexed 
   REL  // relative
} address_modes_t;

typedef struct instruction_t
{
   char* mnemonic;                    // 3 character mnemonic of the a instruction
   uint8_t (*opcode_function) (void); // pointer to a function that contain the execution code of a instruction, may return extra cycle if branching occurs
   address_modes_t mode;              // enum that represents the addressing mode of the instruction
   uint8_t cycles;                    // number of base clock cycles this instruction takes
} instruction_t;

void cpu_emulate_instruction(void);
void cpu_reset(void);
void cpu_init(void);
void cpu_IRQ(void);
void cpu_NMI(void);
void cpu_tick(void);
cpu_6502_t* get_cpu(void);
const instruction_t* get_instruction_lookup_entry(uint8_t position);

#endif

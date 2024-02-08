#ifndef INSTRUCTIONS_6502_H
#define INSTRUCTIONS_6502_H

#include <stdint.h>

typedef enum address_modes
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

typedef struct opcode
{
   char* mnemonic;                   // 3 character mnemonic of the a instruction
   uint8_t (*opcode_function) (void);   // pointer to a function that contain the execution code of a instruction
   address_modes_t mode;             // enum that represents the addressing mode of the instruction
   uint8_t cycles;                   // number of cycles this instruction takes
} opcode_t;

void instruction_decode(uint8_t opcode);
uint8_t instruction_execute(uint8_t opcode);

#endif
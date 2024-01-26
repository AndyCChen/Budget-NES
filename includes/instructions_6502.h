#ifndef INSTRUCTIONS_6502_H
#define INSTRUCTIONS_6502_H

#include <stdint.h>

typedef enum addressing_mode
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
   YZI, // Y-indexed zero page indirect
   REL  // relative
} addressing_mode_t;

typedef struct opcode
{
   char* mnemonic;          // 3 character mnemonic of the a instruction
   void (*opcode) (void);   // pointer to a function that contain the execution code of a instruction
   addressing_mode_t mode;  // enum that represents the addressing mode of the instruction
   uint8_t cycles;          // number of cycles this instruction takes
} opcode_t;



#endif
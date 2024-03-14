#ifndef INSTRUCTIONS_6502_H
#define INSTRUCTIONS_6502_H

#include <stdint.h>

#define NMI_VECTOR       0xFFFA // address of non-maskable interrupt vector
#define RESET_VECTOR     0xFFFC // address of reset vector
#define INTERRUPT_VECTOR 0xFFFE // address of the interrupt vector

#define CPU_RAM_SIZE 1024 * 2
#define CPU_RAM_END  0x1FFF     // ending address space of cpu ram

// memory mapped addresses used by cpu to access cartridge space

#define CPU_CARTRIDGE_START         0x4020
#define CPU_CARTRIDGE_PRG_RAM_START 0x6000
#define CPU_CARTRIDGE_PRG_RAM_END   0x7FFF
#define CPU_CARTRIDGE_PRG_ROM_START 0x8000

// address range used by cpu to access ppu registers

#define CPU_PPU_REG_START 0x2000
#define CPU_PPU_REG_END   0x3FFF

/**
 * Bottom address of the stack.
 * Stack pointer value is added to this address as a offset.
 * When a value is pushed, the stack pointer is decremented and when
 * a value is popped, the stack pointer is incremented
*/
#define CPU_STACK_ADDRESS 0x0100

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

typedef struct cpu_6502_t
{
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

typedef struct instruction_t
{
   char* mnemonic;                    // 3 character mnemonic of the a instruction
   uint8_t (*opcode_function) (void); // pointer to a function that contain the execution code of a instruction, may return extra cycle if branching occurs
   address_modes_t mode;              // enum that represents the addressing mode of the instruction
   uint8_t cycles;                    // number of base clock cycles this instruction takes
} instruction_t;

void cpu_emulate_instruction(void);
void cpu_reset(void);

#endif
#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include "../includes/cpu.h"

/**
 * disassembles a opcode without disrupting emulator execution.
 * Will internally increment a pointer to the next instruction to decode.
 * For example if a instruction is 2 bytes, the pointer will be advanced by 2 in order to 
 * point to the next instruction.
 * @returns size of the instruction in bytes (i.e opcode byte + operand bytes)
 */ 
void disassemble(void);

/**
 * Sets the pointer to the address of the instruction to disassemble.
 * Used to set the starting point of the disassembler or to disassemble
 * starting at another location in memory.
*/
void disassemble_set_position(uint16_t pos);

/**
 * Disassemble the next x number of instructions.
 * @param x the number of instructions to disassemble
*/
void disassemble_next_x(uint8_t x);

#endif

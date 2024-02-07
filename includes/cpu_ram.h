#ifndef CPU_RAM_H
#define CPU_RAM_H

#include <stdint.h>

#define CPU_RAM_SIZE 0x7ff

/**
 * Bottom address of the stack.
 * Stack pointer value is added to this address as a offset.
 * When a value is pushed, the stack pointer is decremented and when
 * a value is popped, the stack pointer is incremented
*/
#define CPU_STACK_ADDRESS 0x0100

uint8_t cpu_ram_read(uint16_t position);
void cpu_ram_write(uint16_t position, uint8_t data);

#endif
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stdbool.h>

#define CPU_RAM_SIZE 1024 * 2
#define CPU_RAM_END 0x1FFF // ending address space of cpu ram

/**
 * Bottom address of the stack.
 * Stack pointer value is added to this address as a offset.
 * When a value is pushed, the stack pointer is decremented and when
 * a value is popped, the stack pointer is incremented
*/
#define CPU_STACK_ADDRESS 0x0100

uint8_t cpu_bus_read(uint16_t position);
void cpu_bus_write(uint16_t position, uint8_t data);

uint16_t cpu_bus_read_u16(uint16_t position);
void cpu_bus_write_u16(uint16_t position, uint16_t data);

#endif
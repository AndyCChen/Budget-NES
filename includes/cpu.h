#ifndef INSTRUCTIONS_6502_H
#define INSTRUCTIONS_6502_H

#include <stdint.h>

void cpu_emulate_instruction(void);
void cpu_reset(void);
void cpu_IRQ(void);

#endif
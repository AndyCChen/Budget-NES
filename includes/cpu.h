#ifndef INSTRUCTIONS_6502_H
#define INSTRUCTIONS_6502_H

void cpu_emulate_instruction(void);
void cpu_reset(void);
void cpu_init(void);
void cpu_IRQ(void);
void cpu_NMI(void);
void cpu_tick(void);

#endif

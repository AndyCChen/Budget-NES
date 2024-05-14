#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

#define MAX_NEXT 5 // max number of future instructions to disassemble

extern FILE *log_file; // pointer to nes test output log file

   

bool log_open(void);
void log_close(void);

/**
 * Start writing to the ring buffer with a max
 * characters allowed to be written defined by DISASSEM_LENGTH.
*/
void log_write(const char* const format, ...);

/**
 * Moves the head of the buffer back.
 * @param r move the buffer head back r times
*/
void log_rewind(uint8_t r);

/**
 * Get the disassembled instruction that is currently being executed
*/
const char* log_get_current_instruction(void);

/**
 * Get the x-th disassembled instruction starting from the instruction currently being executed.
 * @param x the next x-th instruction to retrive. Passing in 2 for example means retrieve the 2nd instruction relative
 * to the instruction being executed currently.
*/
const char* log_get_next_instruction(uint8_t x);

/**
 * Get the previous x-th disassembled instruction starting from the instruction currently being executed
 * @param x the x-th previous instruction to retrieve
*/
const char* log_get_prev_instruction(uint8_t x);

/**
 * Updates the index that points to the disassembled instruction that is being executed by the emulator
*/
void log_update_current(void);

#endif

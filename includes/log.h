#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

#define MAX_NEXT 5     // max number of future instructions to disassemble

extern const char* log_size_options[];
extern const size_t log_size_options_count;

void dump_log_to_file();

/**
 * Start writing to the ring buffer with a max
 * characters allowed to be written defined by DISASSEM_LENGTH.
*/
void log_write_instruction(const char* const format, ...);

/**
 * Log state of cpu registers
*/
void log_cpu_state(const char* const format, ...);

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
const char* log_get_next_instruction(uint32_t x);

/**
 * Get the previous x-th disassembled instruction starting from the instruction currently being executed
 * @param x the x-th previous instruction to retrieve
*/
const char* log_get_prev_instruction(uint32_t x);

const char* log_get_prev_cpu_state(uint32_t x);

/**
 * Updates the index that points to the disassembled instruction that is being executed by the emulator
*/
void log_update_current(void);

/**
 * Set the number of instructions to log.
 * @param select index representing a log_max_instructions_t enum
 */
void log_set_size(uint32_t select);

/**
 * Allocates memory for log buffers, will internally free the log buffers
 * if they are not NULL before allocating new memory.
 */
bool log_allocate_buffers(void);

#endif

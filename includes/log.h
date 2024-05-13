#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

#define MAX_NEXT 5 // max number of future instructions to disassemble

extern FILE *log_file; // pointer to nes test output log file
extern bool nestest_log_flag;  // true: ouput logs into nestest log file, false: do not log anything

// writes to a log file if nestest_log_flag is true
// use this macro instead of the function to avoid having to write if statements to check the flag everytime 
#define nestest_log(format, ...)                                    \
   do                                                               \
   {                                                                \
      log_write(format, __VA_ARGS__);         \
   } while (0);                                                     \
   

bool log_open(void);
void log_close(void);

/**
 * Start writing to the ring buffer with a max
 * characters allowed to be written defined by DISASSEM_LENGTH.
*/
void log_write(const char* const format, ...);

/**
 * Advances the head of the ring buffer by 1
 * to point to the next location that will store
 * the disassembled instruction c_string.
 * Also moves the sub head pointer to the beginning of the c_string.
*/
void log_new_line(void);

/**
 * Moves the head of the buffer back.
 * @param r move the buffer head back r times
*/
void log_rewind(uint8_t r);

#endif

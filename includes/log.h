#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

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
void log_write(const char* const format, ...);
void log_clear(void);
char* log_get(void);
void log_close(void);
void log_file_output(void);

#endif

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

extern FILE *nestest_log_file; // pointer to nes test output log file
extern bool nestest_log_flag;  // true: ouput logs into nestest log file, false: do not log anything

// writes to a log file if nestest_log_flag is true
// use this macro instead of the function to avoid having to write if statements to check the flag everytime 
#define nestest_log(format, ...)                                    \
   do                                                               \
   {                                                                \
      if (nestest_log_flag) nestest_log_write(format, __VA_ARGS__); \
   } while (0);                                                     \
   

bool nestest_log_open();
void nestest_log_write(const char* const format, ...);
void nestest_log_close();

#endif
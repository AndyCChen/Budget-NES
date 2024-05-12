#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#include "../includes/log.h"

FILE *log_file = NULL;
static char log_buffer[256]; // 256 byte long buffer for storing log of a single instruction
static uint8_t head = 0;     // pointer to current position in log buffer

// opens/create file for nestest logs
// does nothing if nestest_log_flag is set to false
bool log_open(void)
{
   // do not open/create log file if log flag is false
   if ( !nestest_log_flag )
   {
      return false;
   }

   log_file = fopen("nes.log", "w");
   
   if (log_file == NULL)
   {
      printf("Failed to open/create log file!\n");
   }

   return log_file != NULL;
}

void log_write(const char* const format, ...)
{
   va_list args;
   va_start(args, format);

   uint16_t bytes_written = vsnprintf(log_buffer + head, 256 - head, format, args);
   head += bytes_written;

   va_end(args);
}

// output log buffer into log file
void log_file_output(void)
{
   fprintf(log_file, log_buffer);
}

// moves pointer back to beginning of log buffer
void log_clear(void)
{
   head = 0;
}

// retrieves pointer to log_buffer
char* log_get(void)
{
   return log_buffer;
}


// closes nestest log file
void log_close(void)
{
   // only close file if log flag was true
   if ( nestest_log_flag )
   {
      fclose(log_file);
   }
}

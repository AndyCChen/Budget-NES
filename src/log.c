#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "../includes/log.h"

FILE *nestest_log_file = NULL;

// opens/create file for nestest logs
// does nothing if nestest_log_flag is set to false
bool nestest_log_open()
{
   // do not open/create log file if log flag is false
   if ( !nestest_log_flag )
   {
      return false;
   }

   nestest_log_file = fopen("nes.log", "w");
   
   if (nestest_log_file == NULL)
   {
      printf("Failed to open/create nestest log file!\n");
   }

   return nestest_log_file != NULL;
}

void nestest_log_write(const char* const format, ...)
{
   assert((nestest_log_file != NULL) && "Writing to log file that does not exist!");

   va_list args;
   va_start(args, format);

   vfprintf(nestest_log_file, format, args);

   va_end(args);
}

// closes nestest log file
void nestest_log_close()
{
   // only close file if log flag was true
   if ( nestest_log_flag )
   {
      fclose(nestest_log_file);
   }
}
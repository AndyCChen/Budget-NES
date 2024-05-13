#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>

#include "../includes/log.h"

#define MAX_INSTR 11        // max number of disassembled instructions to store in ring buffer
#define DISASSEM_LENGTH 64  // max size of the buffer used to store disassembled instruction c_strings

FILE *log_file = NULL;
static char ring_buffer[MAX_INSTR][DISASSEM_LENGTH]; // a ring buffer disassembled instruction c_strings
static uint8_t buffer_head = 0;                      // position of head in ring buffer, this will generally be pointing to the most recently disassembled instruction a couple instructions ahead of the currently executed instruction
static uint8_t sub_buffer_head = 0;                  // position of the write cursor of a c_string inside the ring buffer

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

// closes nestest log file
void log_close(void)
{
   // only close file if log flag was true
   if ( nestest_log_flag )
   {
      fclose(log_file);
   }
}

void log_write(const char* const format, ...)
{
   va_list args;
   va_start(args, format);
   
   assert( (sub_buffer_head) < DISASSEM_LENGTH && "Buffer overflow in log buffer!\n" );
   uint32_t bytes_written = vsnprintf(&ring_buffer[buffer_head][sub_buffer_head], DISASSEM_LENGTH - sub_buffer_head, format, args);
   sub_buffer_head += bytes_written;
   
   va_end(args);
}

void log_new_line(void)
{
   buffer_head += 1;         // increment head pointer
   buffer_head %= MAX_INSTR; // wrap back to zero if head exceeds max buffer size
   
   sub_buffer_head = 0;      // move the sub buffer head back to the beginner of the c_string
}

void log_rewind(uint8_t r)
{
   buffer_head -= r;
   buffer_head %= MAX_INSTR; 

   sub_buffer_head = 0;      // move the sub buffer head back to the beginner of the c_string
}

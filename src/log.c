// This file contains logging functions specifically for logging the disassemble 6502 instructions to the display while the emulator is running.

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
static uint8_t current = 0;                          // index pointing to the disassembled instruction currently being executed

static uint32_t modulo(int x, int m)
{
   return (x % m + m) % m;
}

// opens/create file for nestest logs
// does nothing if nestest_log_flag is set to false
/* bool log_open(void)
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
} */

// closes nestest log file
/* void log_close(void)
{
   // only close file if log flag was true
   if ( nestest_log_flag )
   {
      fclose(log_file);
   }
} */

void log_write(const char* const format, ...)
{
   va_list args;
   va_start(args, format);
   
   uint32_t bytes_written = vsnprintf( ring_buffer[buffer_head], DISASSEM_LENGTH, format, args );
   assert( (bytes_written) < DISASSEM_LENGTH && "Buffer overflow in log buffer!\n" );
   
   va_end(args);
}

void log_new_line(void)
{
   buffer_head += 1;         // increment head pointer
   buffer_head %= MAX_INSTR; // wrap back to zero if head exceeds max buffer size
}

void log_rewind(uint8_t r)
{
   buffer_head -= r;
   buffer_head = modulo(buffer_head, MAX_INSTR); 
}

void log_update_current(void)
{
   current = modulo(buffer_head - MAX_NEXT - 1, MAX_INSTR);
}

const char* log_get_current_instruction(void)
{
   return ring_buffer[current];
}

const char* log_get_next_instruction(uint8_t x)
{
   return ring_buffer[ (current + x) % MAX_INSTR ];
}

const char* log_get_prev_instruction(uint8_t x)
{
   uint8_t prev = modulo(current - x, MAX_INSTR);
   return ring_buffer[prev];
}

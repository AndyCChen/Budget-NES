// This file contains logging functions specifically for logging the disassemble 6502 instructions to the display while the emulator is running.

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>

#include "../includes/log.h"

#define DISASSEM_LENGTH 64  // max size of the buffer used to store disassembled instruction c_strings

FILE *log_file = NULL;
static char ring_buffer[MAX_INSTR][DISASSEM_LENGTH]; // a ring buffer disassembled instruction c_strings
static uint32_t buffer_head = 0;                      // position of head in ring buffer, this will generally be pointing to the most recently disassembled instruction a couple instructions ahead of the currently executed instruction
static uint32_t current = 0;                          // index pointing to the disassembled instruction that will be executed

static uint32_t modulo(int x, int m)
{
   return (x % m + m) % m;
}

// opens/create file for nestest logs
bool log_file_open(void)
{
   log_file = fopen("nes.log", "w");

   if (log_file == NULL)
   {
      printf("Failed to open/create log file!\n");
   }

   return log_file != NULL;
}

// closes nestest log file
void log_file_close(void)
{
   fclose(log_file);
   log_file = NULL;
}

void log_write(const char* const format, ...)
{
   va_list args;
   va_start(args, format);
   
   uint32_t bytes_written = vsnprintf( ring_buffer[buffer_head], DISASSEM_LENGTH, format, args );
   assert( (bytes_written) < DISASSEM_LENGTH && "Buffer overflow in log buffer!\n" );
   
   va_end(args);

   buffer_head += 1;         // increment head pointer
   buffer_head %= MAX_INSTR; // wrap back to zero if head exceeds max buffer size
}

void log_rewind(uint8_t r)
{
   buffer_head = modulo(buffer_head - r, MAX_INSTR); 
}

void log_update_current(void)
{
   current = modulo(buffer_head - MAX_NEXT - 1, MAX_INSTR);
}

void log_to_file(void)
{  
   printf("log to file\n");
   log_update_current();
   log_file = freopen("nes.log", "w", log_file);

   if (log_file == NULL)
   {
      printf("Error reopening log file!\n");
      return;
   }

   for (int i = MAX_INSTR - MAX_NEXT - 1; i >= 0; --i)
   {
      fprintf(log_file, "%s", log_get_prev_instruction(i));
   }
}

const char* log_get_current_instruction(void)
{
   return ring_buffer[current];
}

const char* log_get_next_instruction(uint32_t x)
{
   return ring_buffer[ (current + x) % MAX_INSTR ];
}

const char* log_get_prev_instruction(uint32_t x)
{
   size_t prev = modulo(current - x, MAX_INSTR);
   return ring_buffer[prev];
}

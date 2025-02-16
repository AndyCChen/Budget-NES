// This file contains logging functions specifically for logging the disassemble 6502 instructions to the display while the emulator is running.

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../includes/log.h"

#define DEFAULT_MAX_INTR      100  // default number of lines to log, must be equivalent to DEFAULT_MAX_INTR_STR
#define DEFAULT_MAX_INTR_STR "100" // string option of the default number of lines to log
#define INSTRUCTION_BUFFER_LENGTH 64  // max size of the buffer used to store disassembled instruction c_strings
#define REGISTER_BUFFER_LENGTH    32

static uint32_t max_instructions_input       = DEFAULT_MAX_INTR;
static uint32_t max_instructions             = DEFAULT_MAX_INTR;
static uint32_t instruction_ring_buffer_size = DEFAULT_MAX_INTR + MAX_NEXT + 1;

const char* log_size_options[] = {DEFAULT_MAX_INTR_STR, "500", "1000", "5000", "10000", "30000"};
const size_t log_size_options_count = sizeof(log_size_options) / sizeof(log_size_options[0]);

FILE *log_file = NULL;

// cpu register logs

static char (*register_ring_buffer)[REGISTER_BUFFER_LENGTH];
static uint32_t register_buffer_head = 0;

// cpu instruction logs

static char (*instruction_ring_buffer)[INSTRUCTION_BUFFER_LENGTH];  // a ring buffer disassembled instruction c_strings
static uint32_t buffer_head = 0; // position of head in ring buffer, this will generally be pointing to the most recently disassembled instruction a couple instructions ahead of the currently executed instruction

static uint32_t current = 0; // index pointing to the disassembled instruction that will be executed

static uint32_t modulo(int x, int m);

void dump_log_to_file(void)
{  
   log_update_current();

   log_file = fopen("BudgetNES.log", "w");

   if (log_file == NULL)
   {
      printf("Failed to open/create log file!\n");
      return;
   }

   for (int i = max_instructions; i > 0; --i)
   {
      const char* cpu_state_str = log_get_prev_cpu_state(i);
      const char* prev_instruction_str = log_get_prev_instruction(i);
      if ( !(strcmp(" ", cpu_state_str) == 0 || strcmp(" ", prev_instruction_str) == 0) )
      {
         fprintf(log_file, "%s \t $%s", log_get_prev_cpu_state(i), log_get_prev_instruction(i));
      }
   }

   fclose(log_file);
   log_file = NULL;
}

void log_write_instruction(const char* const format, ...)
{
   va_list args;
   va_start(args, format);
   
   uint32_t bytes_written = vsnprintf( instruction_ring_buffer[buffer_head], INSTRUCTION_BUFFER_LENGTH, format, args );
   assert( (bytes_written) < INSTRUCTION_BUFFER_LENGTH && "Buffer overflow in log buffer!\n" );
   
   va_end(args);

   buffer_head += 1;         // increment head pointer
   buffer_head %= instruction_ring_buffer_size; // wrap back to zero if head exceeds max buffer size
}

void log_cpu_state(const char* const format, ...)
{
   va_list args;
   va_start(args, format);
   
   uint32_t bytes_written = vsnprintf( register_ring_buffer[register_buffer_head], REGISTER_BUFFER_LENGTH, format, args );
   assert( (bytes_written) < REGISTER_BUFFER_LENGTH && "Buffer overflow in cpu state log buffer!\n" );

   va_end(args);

   register_buffer_head += 1;
   register_buffer_head %= max_instructions;
}

void log_rewind(uint8_t r)
{
   buffer_head = modulo(buffer_head - r, instruction_ring_buffer_size); 
}

void log_update_current(void)
{
   current = modulo(buffer_head - MAX_NEXT - 1, instruction_ring_buffer_size);
}

const char* log_get_current_instruction(void)
{
   if (instruction_ring_buffer == NULL) return "";
   return instruction_ring_buffer[current];
}

const char* log_get_next_instruction(uint32_t x)
{
   if (instruction_ring_buffer == NULL) return "";
   return instruction_ring_buffer[ (current + x) % (instruction_ring_buffer_size) ];
}

const char* log_get_prev_instruction(uint32_t x)
{
   if (register_ring_buffer == NULL) return "";

   uint32_t prev = modulo(current - x, instruction_ring_buffer_size);
   return instruction_ring_buffer[prev];
}

const char* log_get_prev_cpu_state(uint32_t x)
{
   if (register_ring_buffer == NULL) return "";

   uint32_t prev = modulo(register_buffer_head - x, max_instructions);
   return register_ring_buffer[prev];
}

void log_set_size(uint32_t select)
{
   if ( !(select < log_size_options_count) )
   {
      return;
   }
   
   char *end;
   max_instructions_input = strtol(log_size_options[select], &end, 10);

   if (max_instructions_input == 0) max_instructions_input = DEFAULT_MAX_INTR;
}

bool log_allocate_buffers(void)
{
   // free buffers before allocating new memory
   log_free();
   
   max_instructions             = max_instructions_input;
   instruction_ring_buffer_size = max_instructions_input + MAX_NEXT + 1;

   register_ring_buffer    = malloc( sizeof(*register_ring_buffer) * max_instructions );
   instruction_ring_buffer = malloc( sizeof(*instruction_ring_buffer) * instruction_ring_buffer_size );

   if (register_ring_buffer == NULL || instruction_ring_buffer == NULL)
   {
      printf("Failed to allocate memory for log buffers\n");
      return false;
   }

   printf("\n%zu bytes allocated.\n", 
	   (sizeof(char) * max_instructions * REGISTER_BUFFER_LENGTH) + 
	   (sizeof(char) * instruction_ring_buffer_size * INSTRUCTION_BUFFER_LENGTH)
   );

   // initialize buffers to empty string
   for (uint32_t i = 0; i < max_instructions; ++i)
   {
      snprintf(register_ring_buffer[i], REGISTER_BUFFER_LENGTH, " ");
      snprintf(instruction_ring_buffer[i], INSTRUCTION_BUFFER_LENGTH, " ");
   }

   // initialize remaining space since the instruction ring buffer is slightly larger to hold
   // the next MAX_NEXT future disassembled instructions
   for (uint32_t i = max_instructions; i < instruction_ring_buffer_size; ++i)
   {
      snprintf(instruction_ring_buffer[i], INSTRUCTION_BUFFER_LENGTH, " ");
   }

   return true;
}

void log_free(void)
{
   free(register_ring_buffer);
   free(instruction_ring_buffer);

   register_ring_buffer = NULL;
   instruction_ring_buffer = NULL;

   current              = 0;
   buffer_head          = 0;
   register_buffer_head = 0;
}

static uint32_t modulo(int x, int m)
{
   return (x % m + m) % m;
}

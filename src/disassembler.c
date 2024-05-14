#include <string.h>

#include "../includes/disassembler.h"
#include "../includes/log.h"
#include "../includes/cpu.h"
#include "../includes/bus.h"

static uint16_t position = 0;

void disassemble_set_position(uint16_t pos)
{
   position = pos;
}

void disassemble(void)
{
   const instruction_t* instruction = get_instruction_lookup_entry( DEBUG_cpu_bus_read(position) );

   uint8_t instruction_size = 0;

   switch (instruction->mode)
   {
      case IMP:
      {
         if (strcmp("BRK", instruction->mnemonic) == 0) instruction_size = 2; // handle edge case where instruction is BRK, this instruction is 2 bytes not 1!!!
         else                                           instruction_size = 1;
         log_write("%04X %s\n", position, instruction->mnemonic);
         break;
      }
      case ACC:
      { 
         instruction_size = 1;
         log_write("%04X %s A\n", position, instruction->mnemonic);
         break;
      }
      case IMM:
      {
         instruction_size = 2;
         log_write("%04X %s #$%02X\n", position, instruction->mnemonic, DEBUG_cpu_bus_read(position + 1));
         break;
      }
      case ABS:
      {
         instruction_size = 3;
         uint8_t lo = DEBUG_cpu_bus_read(position + 1);
         uint8_t hi = DEBUG_cpu_bus_read(position + 2);

         uint16_t instruction_operand = ( hi << 8 ) | lo;

         log_write("%04X %s $%04X\n", position, instruction->mnemonic, instruction_operand);
         
         break;
      }
      case XAB:
      {
         instruction_size = 3;
         uint8_t lo = DEBUG_cpu_bus_read(position + 1);
         uint8_t hi = DEBUG_cpu_bus_read(position + 2);

         uint16_t abs_address = ( hi << 8 ) | lo;

         log_write("%04X %s $%04X,X\n", position, instruction->mnemonic, abs_address);
         break;
      }
      case YAB:
      {
         instruction_size = 3;
         uint8_t lo = DEBUG_cpu_bus_read(position + 1);
         uint8_t hi = DEBUG_cpu_bus_read(position + 2);

         uint16_t abs_address = ( hi << 8 ) | lo;

         log_write("%04X %s $%04X,Y\n", position, instruction->mnemonic, abs_address);
         break;
      }
      case ABI:
      {
         instruction_size = 3;
         uint8_t lo = DEBUG_cpu_bus_read(position + 1);
         uint8_t hi = DEBUG_cpu_bus_read(position + 2);

         uint16_t abs_address = ( hi << 8 ) | lo;

         log_write("%04X %s ($%04X)\n", position, instruction->mnemonic, abs_address);
         break;
      }
      case ZPG:
      {
         instruction_size = 2;
         log_write("%04X %s $%02X\n", position, instruction->mnemonic, DEBUG_cpu_bus_read(position + 1));
         break;
      }
      case XZP:
      {
         instruction_size = 2;
         log_write("%04X %s $%02X,X\n", position, instruction->mnemonic, DEBUG_cpu_bus_read(position + 1));
         break;
      }
      case YZP:
      {
         instruction_size = 2;
         log_write("%04X %s $%02X,Y\n", position, instruction->mnemonic, DEBUG_cpu_bus_read(position + 1));
         break;
      }
      case XZI:
      {
         instruction_size = 2;
         log_write("%04X %s ($%02X,X)\n", position, instruction->mnemonic, DEBUG_cpu_bus_read(position + 1));
         break;
      }
      case YZI:
      {
         instruction_size = 2;
         log_write("%04X %s ($%02X),Y\n", position, instruction->mnemonic, DEBUG_cpu_bus_read(position + 1));
         break;
      }
      case REL:
      {
         instruction_size = 2;
         uint8_t offset_byte = DEBUG_cpu_bus_read(position + 1);
         uint16_t instruction_operand = 0;
         
         /**
          * Offset_byte is in 2's complement so we check bit 7 to see if it is negative.
         */
         if (offset_byte & 0x80)
         {
            /**
             * Since the signed offset_byte is 8 bits and we are adding into a 16 bit value,
             * we need to treat the offset as a 16 bit value. The offset byte is negative value
             * in 2's complement, so the added extra 8 bits are all set to 1, hence the 0xFF00 bitmask.
            */
            instruction_operand = (position + 2) + ( (uint16_t) offset_byte | 0xFF00 );
         }
         else
         {
            instruction_operand = (position + 2) + offset_byte;
         }

         log_write("%04X %s $%04X\n", position, instruction->mnemonic, instruction_operand);
         break;
      }
   }

   position += instruction_size;
}

void disassemble_next_x(uint8_t x)
{
   for (int i = 0; i < x; ++i)
   {
      disassemble();
   }
}

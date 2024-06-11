#include "../includes/mappers/mapper_001.h"
#include "../includes/mappers/mirror_config.h"



cartridge_access_mode_t mapper001_cpu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers)
{

}

cartridge_access_mode_t mapper001_cpu_write(nes_header_t *header, uint16_t position, uint8_t data, uint16_t *mapped_addr, void* internal_registers)
{
   Registers_001* mapper = (Registers_001*) internal_registers;

   // writes into program rom is intercepted and used to configure bank switching
   if (position >= 0x8000 && position <= 0xFFFF)
   {
      // bit 7 set means reset shift register and set prg bank mode to 3
      if (mapper->shift_register & 0x80)
      {
         mapper->shift_register = 0x10;
         mapper->PRG_bank |= 0xC;
      }
      // bit 0 being set means shift register is full
      else if (mapper->shift_register & 0x1)
      {
         uint8_t dest = ( (data & 0x1) << 4 ) | ( (mapper->shift_register & 0x1E) >> 1 );
         mapper->shift_register = 0x10;

         // chr bank 0
         if (position >= 0xA000 && position <= 0xBFFF)
         {
            mapper->CHR_bank_0 = dest;
         }
         // chr bank 1
         else if (position >= 0xC000 && position <= 0xDFFF)
         {
            mapper->CHR_bank_1 = dest;
         }
         // prg bank
         else if (position >= 0xE000 && position <= 0xFFFF)
         {
            mapper->PRG_bank = dest;
         }
         // control register
         else
         {
            mapper->control = dest;
         }
      }
      // else just shift bit 0 of data being written into the msb of shift register
      // and shift the register right 1 bit
      else
      {
         mapper->shift_register = mapper->shift_register >> 1;
         mapper->shift_register |= (data & 0x1) << 4;
      }
   }
}

cartridge_access_mode_t mapper001_ppu_read(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers)
{

}

cartridge_access_mode_t mapper001_ppu_write(nes_header_t *header, uint16_t position, uint16_t *mapped_addr, void* internal_registers)
{

}

void mapper001_init(nes_header_t* header, void* internal_registers)
{
   Registers_001* data = (Registers_001*) internal_registers;

   data->control = header->nametable_arrangement ? 0x2 : 0x3; // initalize mirroring configuration
   data->control |= 0xC;

   data->shift_register = 0x10;
}

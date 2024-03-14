#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../includes/cartridge.h"
#include "../includes/cpu.h"
#include "../includes/ppu.h"
#include "../includes/mapper.h"

#define iNES_HEADER_SIZE 16 // iNES headers are all 16 bytes long
#define TRAINER_SIZE 512

static mapper_t mapper;
static nes_header_t header;

static uint8_t *prg_rom = NULL;
static uint8_t *prg_ram = NULL;
static uint8_t *chr_memory = NULL; // array for chr-ram or chr-rom

/**
 * Loads data in iNES header into a struct
 * @param iNES_header array container 16 header
 * @param header struct to contain header information
*/
static void load_iNES(uint8_t *iNES_header, nes_header_t *header );

bool cartridge_load(const char* const filepath)
{
   FILE *file = fopen(filepath, "rb");
   if (!file)
   {
      printf("Cannot open file: %s\n", filepath);
      return false;
   }

   // read iNes header into array

   uint8_t iNES_header[iNES_HEADER_SIZE];
   uint8_t bytes_read = fread(iNES_header, 1, iNES_HEADER_SIZE, file);

   if (bytes_read != iNES_HEADER_SIZE)
   {
      printf("Failed to read iNES header!\n");
      return false;
   }

   if ( !(iNES_header[0]=='N' && iNES_header[1]=='E' && iNES_header[2]=='S' && iNES_header[3]==0x1A) )
   {
      printf("Invalid nes file!\n");
      return false;
   }

   if ( iNES_header[7] & 0x0C == 0x08 )
   {
      printf("iNES 2.0 not supported!\n");
      return false;
   }

   load_iNES(iNES_header, &header);
   if ( !load_mapper(header.mapper_id, &mapper) )
   {
      printf("Mapper does not exist or is not supported!\n");
      return false;
   }

   size_t program_size = header.program_rom_size * 1024 * 16;
   size_t chr_size;
   if (header.chr_rom_size == 0) // if rom size is zero we use chr_ram which will just be fixed to 8kb of memory
   {
      chr_size = 1024 * 8;
   }
   else
   {
      chr_size = header.chr_rom_size * 1024 * 8;
   }

   // allocate memory for program and chr rom

   prg_rom = calloc( program_size, sizeof(uint8_t) );
   if (prg_rom == NULL)
   {
      printf("Failed to allocate memory for PRG-rom!\n");
      return false;
   }

   chr_memory = calloc( chr_size, sizeof(uint8_t) );
   if (chr_memory == NULL)
   {
      printf("Failed to allocated memory for CHR-rom!\n");
      return false;
   }

   // read trainer data and do nothing with it if it exists
   if ( header.trainer != 0 )
   {
      fread(NULL, 1, TRAINER_SIZE, file);
   }

   bytes_read = fread(prg_rom, 1, program_size, file);
   if (bytes_read != program_size)
   {
      printf("Program rom reading error!\n");
      return false;
   }

   bytes_read = fread(chr_memory, 1, chr_size, file);
   if (bytes_read != chr_size)
   {
      printf("CHR-ram/rom reading error!\n");
      return false;
   }

   fclose(file);
   return true;
}

uint8_t cartridge_cpu_read(uint16_t position)
{
   uint16_t mapped_addr = 0;

   if ( position >= CPU_CARTRIDGE_PRG_ROM_START )
   {
      mapper.cpu_read(&header, position, &mapped_addr, PROGRAM_ROM);
      return prg_rom[mapped_addr];
   }
   else if ( position >= CPU_CARTRIDGE_PRG_RAM_START && position <= CPU_CARTRIDGE_PRG_RAM_END )
   {
      mapper.cpu_read(&header, position, &mapped_addr, PROGRAM_RAM);
      return
   }

   return 0;
}

uint8_t cartridge_ppu_read(uint16_t position)
{

}

void cartridge_free_memory()
{
   free(prg_rom);
   free(chr_memory);
}

static void load_iNES(uint8_t *iNES_header, nes_header_t *header )
{
   header->trainer = iNES_header[6] & 0x04;
   header->program_rom_size = iNES_header[4];
   header->chr_rom_size = iNES_header[5];

   uint8_t mapper_id_lo = (iNES_header[6] & 0xF0) >> 4;
   uint8_t mapper_id_hi = (iNES_header[7] & 0xF0);
   header->mapper_id = mapper_id_hi | mapper_id_lo;
}
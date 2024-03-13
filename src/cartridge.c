#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../includes/cartridge.h"

static mapper_t *mapper = NULL;

// this is where the cartridge program (game code) is loaded into
// fixed to 32kb for now
static uint8_t *program_rom = NULL;
static uint8_t *chr_rom = NULL;

bool cartridge_load(const char* const filepath)
{
   FILE *file = fopen(filepath, "rb");

   if (!file)
   {
      printf("Cannot open file: %s\n", filepath);
      return false;
   }

   fseek(file, 0, SEEK_END);
   size_t size = ftell(file); // total bytes to read from file
   rewind(file);

   if (size == 0)
   {
      printf("Empty file!\n");
      return false;
   }

   // read iNes header into array

   uint8_t iNES_header[iNES_HEADER_SIZE];

   uint32_t bytes_read = fread(iNES_header, 1, iNES_HEADER_SIZE, file);
   size -= bytes_read; // decrement total bytes to read byte the amount of bytes previously read

   if (bytes_read != iNES_HEADER_SIZE)
   {
      printf("Failed to read iNES header!\n");
      return false;
   }

   // allocate memory for program and chr rom

   program_rom = calloc( iNES_header[4] * 1024 * 16, sizeof(uint8_t) );  // alocate with 16kb units

   // program_rom must always have alocated memory
   if (program_rom == NULL)
   {
      printf("Failed to allocate memory for PRG-rom!\n");
      return false;
   }

   chr_rom = calloc( iNES_header[5] * 1024 * 8, sizeof(uint8_t) );       // allocate with 8 kb units

   // if size of chr_rom is zero according to iNES header, then we use chr_ram instead
   // otherwise we need to check if request memory for chr_rom is successful
   if (iNES_header[5] != 0 && chr_rom == NULL)
   {
      printf("Failed to allocated memory for CHR-rom!\n");
      return false;
   }
   

   printf("iNES header\n");
   for(int i = 0; i < 16; ++i)
   {
      printf("%02x ", iNES_header[i]);
   }
   printf("\n");

   // read in the rest of the program rom

   bytes_read = fread(program_rom, 1, size, file);

   if (bytes_read != size)
   {
      printf("File reading error!\n");
      return false;
   }

   fclose(file);
   return true;
}

void cartridge_free_memory()
{
   free(program_rom);
   free(chr_rom);
}

uint8_t cartridge_cpu_read(uint16_t position)
{
   return program_rom[position & 0x3FFF];
}
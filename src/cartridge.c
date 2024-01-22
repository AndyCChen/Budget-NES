#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "../includes/cartridge.h"

// this is where the cartridge program (game code) is loaded into
// fixed to 32kb for now
static uint8_t program_rom[0x8000];

// start loading program rom into memory
// todo: handle ines header
bool cartridge_load(const char* const filepath)
{
   FILE *file = fopen(filepath, "rb");

   if (!file)
   {
      printf("Cannot open file: %s\n", filepath);
      return false;
   }

   fseek(file, 0, SEEK_END);
   uint32_t size = ftell(file); // total bytes to read from file
   rewind(file);

   if (size == 0)
   {
      printf("Empty file!\n");
      return false;
   }

   // read in the 16 byte long iNES header

   uint8_t iNES_header[iNES_HEADER_SIZE];

   uint32_t bytes_read = fread(iNES_header, 1, iNES_HEADER_SIZE, file);
   size -= bytes_read; // decrement total bytes to read byte the amount of bytes previously read

   if (bytes_read != iNES_HEADER_SIZE)
   {
      printf("File reading error!\n");
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

// read data from cartridge from at specified position
uint8_t cartridge_read(uint16_t position)
{
   position = position - 0x8000;
   position = position & 0x3FFF;

   return program_rom[position];
}
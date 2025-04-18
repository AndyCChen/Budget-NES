#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if _WIN32
#include <windows.h>
#elif __APPLE__
#include <sys/stat.h>
#include <errno.h>
#endif

#include "cartridge.h"
#include "mapper.h"

#define iNES_HEADER_SIZE 16 // iNES headers are all 16 bytes long
#define TRAINER_SIZE 512

static mapper_t mapper;
static void* mapper_registers = NULL; // void pointer to struct containing a mapper's registers
static nes_header_t rom_header;

static uint8_t ppu_vram[1024 * 2];
static uint8_t *prg_rom = NULL;
static uint8_t *prg_ram = NULL;
static uint8_t *chr_memory = NULL; // memory for either chr-ram or chr-rom

static bool load_iNES10(uint8_t *iNES_header, nes_header_t *header);
static bool load_iNES20(uint8_t *iNES_header, nes_header_t *header);

static char rom_name[256];

uint8_t cartridge_cpu_read(uint16_t position)
{
   size_t mapped_addr = 0;
   cartridge_access_mode_t mode = mapper.cpu_read(&rom_header, position, &mapped_addr, mapper_registers);

   static uint8_t data = 0;
   switch ( mode )
   {
      case ACCESS_PRG_ROM:
         data = prg_rom[mapped_addr];
         break;
      case ACCESS_PRG_RAM:
         data = prg_ram[mapped_addr];
         break;
      case NO_CARTRIDGE_DEVICE: // when addressed location has no attached device, return value from previous read in static data
      default:
         break;
   }

   return data;
}

void cartridge_cpu_write(uint16_t position, uint8_t data)
{
   size_t mapped_addr = 0;
   cartridge_access_mode_t mode = mapper.cpu_write(&rom_header, position, data, &mapped_addr, mapper_registers);

   switch ( mode )
   {
      case ACCESS_PRG_RAM:
         prg_ram[mapped_addr] = data;
         break;
      default:
         break;
   }
}

uint8_t cartridge_ppu_read(uint16_t position)
{
   size_t mapped_addr = 0;

   // ppu address space is only 14 bits, hence the 0x3FFF bitmask

   cartridge_access_mode_t mode = mapper.ppu_read(&rom_header, position & 0x3FFF, &mapped_addr, mapper_registers);

   uint8_t data = 0;
   switch ( mode )
   {
      case ACCESS_CHR_MEM:
         data = chr_memory[mapped_addr];
         break;
      case ACCESS_VRAM:
         data = ppu_vram[mapped_addr]; // returned mapped address for vram 
         break;
      default: // default case will never happen due to bit masking but who knows
         printf("PPU read error!\n");
         exit(EXIT_FAILURE);
         break;
   }

   return data;
}

void cartridge_ppu_write(uint16_t position, uint8_t data)
{  
   size_t mapped_addr = 0;

   // ppu address space is only 14 bits, hence the 0x3FFF bitmask

   cartridge_access_mode_t mode = mapper.ppu_write(&rom_header, position & 0x3FFF, &mapped_addr, mapper_registers);
   
   switch ( mode )
   {
      case ACCESS_CHR_MEM:
         chr_memory[mapped_addr] = data;
         break;
      case ACCESS_VRAM:
         ppu_vram[mapped_addr] = data;
         break;
      default: // default should case will never happen unless chr-rom is written to, in which case no write will occur 
         //printf("Writing to chr-rom %X\n", (int)position);
         break;
   }
}

bool cartridge_load(const char* const filepath)
{
   FILE *file = fopen(filepath, "rb");
   if (!file)
   {
      fclose(file);
      printf("Cannot open file: %s\n", filepath);
      return false;
   }

   uint8_t iNES_header[iNES_HEADER_SIZE];
   size_t bytes_read = fread(iNES_header, sizeof(uint8_t), iNES_HEADER_SIZE, file);

   if (bytes_read != iNES_HEADER_SIZE)
   {
      fclose(file);
      printf("Failed to read iNES header!\n");
      return false;
   }

   // check if .nes file is a valid rom file
   if ( (iNES_header[0]=='N' && iNES_header[1]=='E' && iNES_header[2]=='S' && iNES_header[3]==0x1A) )
   {
      // iNES 2.0
      if ( (iNES_header[7] & 0x0C) == 0x08 )
      {
         printf("iNES 2.0\n");
         if ( !load_iNES20(iNES_header, &rom_header) )
         {
            fclose(file);
            return false;
         }
      }
      // iNES 1.0
      else
      {
         printf("iNES 1.0\n");
         if ( !load_iNES10(iNES_header, &rom_header) )
         {
            fclose(file);
            return false;
         }
      }
   }
   else
   {
      printf("Invalid nes file!\n");
      fclose(file);
      return false;
   }

   if ( !load_mapper(rom_header.mapper_id, &mapper, (void**) &mapper_registers) )
   {
      fclose(file);
      printf("Mapper %d does not exist or is not supported!\n", rom_header.mapper_id);
      return false;
   }
   else
   {
      mapper.init(&rom_header, mapper_registers);
   }
   

   // determine sizes of prg rom/ram and chr rom/ram in bytes

   size_t prg_rom_size = rom_header.prg_rom_size * 1024 * 16;
   size_t chr_mem_size = 0;
   size_t prg_ram_size = 0;

   if (rom_header.chr_rom_size == 0) // if rom size is zero we use chr_ram which will just be fixed to 8kb of memory
   {
      chr_mem_size = 1024 * 8;
   }
   else
   {
      chr_mem_size = rom_header.chr_rom_size * 1024 * 8;
   }

	prg_ram_size = rom_header.prg_ram_size * 1024 * 8;

   // memory allocation for cartridge prg rom/ram and chr rom/ram

   prg_rom = calloc( prg_rom_size, sizeof(uint8_t) );
   if (prg_rom == NULL)
   {
      fclose(file);
      printf("Failed to allocate memory for PRG-rom!\n");
      return false;
   }

   prg_ram = calloc( prg_ram_size, sizeof(uint8_t) );
   if (prg_ram == NULL)
   {
      fclose(file);
      printf("Failed to allocate memory for PRG-ram!\n");
      return false;
   }

   chr_memory = calloc( chr_mem_size, sizeof(uint8_t) );
   if (chr_memory == NULL)
   {
      fclose(file);
      printf("Failed to allocated memory for CHR-rom!\n");
      return false;
   }

   // read nes file contents into corresponding allocated memory blocks

   if ( rom_header.trainer != 0 )
   {     
      // ignore trainer data in nes file
      printf("Trainer data present.\n");
      fseek(file, TRAINER_SIZE, SEEK_CUR);
   }

   bytes_read = fread(prg_rom, sizeof(uint8_t), prg_rom_size, file);
   if (bytes_read != prg_rom_size)
   {
      fclose(file);
      printf("Program rom reading error!\n");
      return false;
   }

   // when rom size is 0 we are using chr-ram so of course chr-rom data does not exist in the nes file
   if ( rom_header.chr_rom_size != 0 ) 
   {
      bytes_read = fread(chr_memory, sizeof(uint8_t), chr_mem_size, file);
      if (bytes_read != chr_mem_size)
      {
         fclose(file);
         printf("CHR-ram/rom reading error!\n");
         return false;
      }
   }

   printf("%-13s %d\n%-13s %zu\n%-13s %zu\n%-13s %zu\n%-13s %s\n", 
      "Mapper:", rom_header.mapper_id, 
      "Prg-ROM size:", prg_rom_size, 
      "CHR-ROM/RAM", chr_mem_size, 
      "PRG_RAM size:", prg_ram_size, 
      "Mirroring:", (rom_header.nametable_arrangement) ? "Vertical" : "Horizontal"
   );

   fclose(file);

	const char* start = NULL;
	const char* end = NULL;

	if ( (start = strrchr(filepath, '\\')) || (start = strrchr(filepath, '/')) )
	{
		start += 1;
	}
	else
	{
		start = filepath;
	}

	if ( !(end = strrchr(filepath, '.')) )
	{
		end = filepath + strlen(filepath) + 1;
	}

	uint16_t start_pos = (uint16_t) (start - filepath);
	uint16_t end_pos = (uint16_t) (end - filepath);
	uint16_t length = (uint16_t) (end_pos - start_pos);

	strncpy(rom_name, start, length);
	rom_name[length] = '\0';
	printf("%s\n\n", rom_name);

	// load prg ram from disk if exists for roms using battery backed ram
	if (rom_header.battery_backed_ram)
	{
		char buffer[256] = "sav/";
		strcat(buffer, rom_name);
		strcat(buffer, ".sav");

		FILE* save_file = fopen(buffer, "rb");
		if (save_file)
		{
			fread(prg_ram, sizeof(uint8_t), prg_ram_size, save_file);
			fclose(save_file);
		}
	}

   return true;
}

void cartridge_free_memory(void)
{
	// save prg ram to disk if rom uses battery backed ram
	if (rom_header.battery_backed_ram)
	{
		char buffer[256] = "sav/";
		strcat(buffer, rom_name);
		strcat(buffer, ".sav");
		
#if _WIN32
      if (CreateDirectory("sav", NULL) == 0)
      {
         // ERROR_ALREADY_EXISTS means directory already exists which is perfectly fine.
         // Any other error is not fine.
         if (GetLastError() != ERROR_ALREADY_EXISTS)
            printf("Failed to create sav directory! Error Code: %lu", GetLastError());
      }
#elif __APPLE__
      if (mkdir("sav", 0777) == -1)
      {
         // EEXIST error means directory already exists which is perfectly fine.
         // if errno does not equal EEXIST then a serious error has occured.
         if (errno != EEXIST)
            printf("Failed to create sav directory! errno code: %d\n", errno);
      }
#endif

// todo support linux?

      // if sav/ directory exists then save prg ram into that directory
      FILE* file = fopen(buffer, "wb");
      if (file)
      {
         fwrite(prg_ram, sizeof(uint8_t), rom_header.prg_ram_size * 1024 * 8, file);
         fclose(file);
      }
	}

   free(prg_rom);
   free(prg_ram);
   free(chr_memory);
   free(mapper_registers);
   
   prg_rom = NULL;
   prg_ram = NULL;
   chr_memory = NULL;
   mapper_registers = NULL;
}

bool cartridge_is_triggering_irq(void)
{
	return mapper.irq_signaled(mapper_registers);
}

/**
 * Loads data in iNES in 1.0 format into a struct.
 * @param iNES_header array container 16 header
 * @param header struct to contain header information
 * @return false on fail and true on success
*/
static bool load_iNES10(uint8_t *iNES_header, nes_header_t *header)
{
	header->battery_backed_ram = (iNES_header[6] >> 1) & 0x1;
   header->trainer = iNES_header[6] & 0x04;
   header->nametable_arrangement = iNES_header[6] & 0x1;
   header->prg_rom_size = iNES_header[4];
   header->prg_ram_size = 4; // fixed prg ram size
   header->chr_rom_size = iNES_header[5];

   if (header->prg_rom_size == 0)
   {
      printf("ERROR! No program rom size specified?\n");
      return false;
   }

   uint8_t mapper_id_lo = (iNES_header[6] & 0xF0) >> 4;
   //uint8_t mapper_id_hi = (iNES_header[7] & 0xF0);
   header->mapper_id =  mapper_id_lo;

   return true;
}

/**
 * Loads data in iNES in 2.0 format into a struct.
 * @param iNES_header array container 16 header
 * @param header struct to contain header information
 * @return false on fail and true on success
*/
static bool load_iNES20(uint8_t *iNES_header, nes_header_t *header)
{
   (void) iNES_header;
   (void) header;
   // todo: iNES 2.0
   printf("iNES 2.0 not implemented yet.\n");

   return false;
}

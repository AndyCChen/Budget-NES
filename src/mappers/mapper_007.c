#include "mapper_007.h"
#include "mirror_config.h"

#include <string.h>

cartridge_access_mode_t mapper007_cpu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	Registers_007* mapper = (Registers_007*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// no prg ram on this mapper

	// reading 32kb switchable prg bank
	if (position >= 0x8000)
	{
		mode = ACCESS_PRG_ROM;
		*mapped_addr = (position & 0x7FFF) + (mapper->prg_bank * 0x8000);
	}

	return mode;
}

cartridge_access_mode_t mapper007_cpu_write(nes_header_t* header, uint16_t position, uint8_t data, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	(void)mapped_addr;
	Registers_007* mapper = (Registers_007*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// no prg ram on this mapper

	if (position >= 0x8000)
	{
		mapper->prg_bank = data & 0x7;
		mapper->vram_bank = (data >> 4) & 0x1;
	}

	return mode;
}

cartridge_access_mode_t mapper007_ppu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	Registers_007* mapper = (Registers_007*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// reading chr memory
	if (position <= 0x1FFF)
	{
		mode = ACCESS_CHR_MEM;
		*mapped_addr = position;
	}
	// reading vram
	else
	{
		mode = ACCESS_VRAM;

		uint16_t p = position & 0x2FFF;
		mirror_config_single_screen(&p, mapper->vram_bank);

		*mapped_addr = p & 0x7FF;
	}

	return mode;
}

cartridge_access_mode_t mapper007_ppu_write(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	Registers_007* mapper = (Registers_007*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	if (position <= 0x1FFF)
	{
		// allow writes only if chr ram is used, 0 rom size implies 8kb of chr-ram
		if (header->chr_rom_size == 0)
		{
			mode = ACCESS_CHR_MEM;
			*mapped_addr = position & 0x1FFF; // iNES 1.0 only which means chr-ram is fixed to 8kb
		}
	}
	// writing to ppu vram
	else
	{
		mode = ACCESS_VRAM;

		uint16_t p = position & 0x2FFF;
		mirror_config_single_screen(&p, mapper->vram_bank);

		*mapped_addr = p & 0x7FF;
	}

	return mode;
}

void mapper007_init(nes_header_t* header, void* internal_registers)
{
	(void)header;
	Registers_007* mapper = (Registers_007*)internal_registers;
	memset(mapper, 0, sizeof(Registers_007));
}

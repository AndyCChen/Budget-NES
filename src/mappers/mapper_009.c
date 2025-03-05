#include "mapper_009.h"
#include "mirror_config.h"

#include <string.h>

cartridge_access_mode_t mapper009_cpu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	Registers_009* mapper = (Registers_009*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// reading from program ram
	if (position >= 0x6000 && position <= 0x7FFF)
	{
		mode = ACCESS_PRG_RAM;
		*mapped_addr = position & 0x1FFF;
	}
	// selecting 8kb of bank switchable prg rom
	else if (position >= 0x8000 && position <= 0x9FFF)
	{
		mode = ACCESS_PRG_ROM;
		*mapped_addr = (position & 0x1FFF) + (mapper->prg_bank * 0x2000);
	}
	// select last 24k prg rom (last 3 8kb banks)
	else if (position >= 0xA000 && position <= 0xBFFF)
	{
		mode = ACCESS_PRG_ROM;
		//*mapped_addr = (position & 0x5FFF) + (((header->prg_rom_size * 2) - 3) * 0x2000);
		*mapped_addr = (position & 0x1FFF) + (((header->prg_rom_size * 2) - 3) * 0x2000);
	}
	else if (position >= 0xC000 && position <= 0xDFFF)
	{
		mode = ACCESS_PRG_ROM;
		*mapped_addr = (position & 0x1FFF) + (((header->prg_rom_size * 2) - 2) * 0x2000);
	}
	else if (position >= 0xE000 && position <= 0xFFFF)
	{
		mode = ACCESS_PRG_ROM;
		*mapped_addr = (position & 0x1FFF) + (((header->prg_rom_size * 2) - 1) * 0x2000);
	}

	return mode;
}

cartridge_access_mode_t mapper009_cpu_write(nes_header_t* header, uint16_t position, uint8_t data, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	Registers_009* mapper = (Registers_009*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// writing to program ram
	if (position >= 0x6000 && position <= 0x7FFF)
	{
		mode = ACCESS_PRG_RAM;
		*mapped_addr = position & 0x1FFF;
	}
	// select prg bank
	else if (position >= 0xA000 && position <= 0xAFFF)
	{
		mapper->prg_bank = data & 0xF;
	}
	else if (position >= 0xB000 && position <= 0xBFFF)
	{
		mapper->chr_bank_FD_0 = data & 0x1F;
	}
	else if (position >= 0xC000 && position <= 0xCFFF)
	{
		mapper->chr_bank_FE_0 = data & 0x1F;
	}
	else if (position >= 0xD000 && position <= 0xDFFF)
	{
		mapper->chr_bank_FD_1 = data & 0x1F;
	}
	else if (position >= 0xE000 && position <= 0xEFFF)
	{
		mapper->chr_bank_FE_1 = data & 0x1F;
	}
	else if (position >= 0xF000 && position <= 0xFFFF)
	{
		mapper->mirroring_mode = data & 0x1;
	}


	return mode;
}

cartridge_access_mode_t mapper009_ppu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	Registers_009* mapper = (Registers_009*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// switchable 4kb bank at 0x0000 - 0x0FFF
	if (position <= 0xFFF)
	{
		mode = ACCESS_CHR_MEM;

		if (mapper->latch_0 == 0xFD)
		{
			*mapped_addr = (position & 0xFFF) + (mapper->chr_bank_FD_0 * 0x1000);
		}
		else if (mapper->latch_0 == 0xFE)
		{
			*mapped_addr = (position & 0xFFF) + (mapper->chr_bank_FE_0 * 0x1000);
		}

		if (position == 0x0FD8)
		{
			mapper->latch_0 = 0xFD;
		}
		else if (position == 0x0FE8)
		{
			mapper->latch_0 = 0xFE;
		}
	}
	// switchable 4kb bank at 0x1000 - 0x1FFF
	else if (position <= 0x1FFF)
	{
		mode = ACCESS_CHR_MEM;

		if (mapper->latch_1 == 0xFD)
		{
			*mapped_addr = (position & 0xFFF) + (mapper->chr_bank_FD_1 * 0x1000);
		}
		else if (mapper->latch_1 == 0xFE)
		{
			*mapped_addr = (position & 0xFFF) + (mapper->chr_bank_FE_1 * 0x1000);
		}

		if (position >= 0x1FD8 && position <= 0x1FDF)
		{
			mapper->latch_1 = 0xFD;
		}
		else if (position >= 0x1FE8 && position <= 0x1FEF)
		{
			mapper->latch_1 = 0xFE;
		}
	}
	// reading ppu nametable vram
	else
	{
		mode = ACCESS_VRAM;
		uint16_t p = position & 0x2FFF;

		if (mapper->mirroring_mode)
		{
			mirror_config_horizontal(&p);
		}
		else
		{
			mirror_config_vertical(&p);
		}

		*mapped_addr = p & 0x7FF;
	}




	return mode;
}

cartridge_access_mode_t mapper009_ppu_write(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	Registers_009* mapper = (Registers_009*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// writing to chr-memory
	if (position <= 0x1FFF)
	{
		// only allow writes if chr memory is ram and not rom
		if (header->chr_rom_size == 0)
		{
			mode = ACCESS_CHR_MEM;
			*mapped_addr = position & 0x1FFF; // iNES 1.0 only which means chr-ram is fixed to 8kb
		}
	}
	// writing to ppu nametable vram
	else
	{
		mode = ACCESS_VRAM;
		position = position & 0x2FFF;

		if (mapper->mirroring_mode)
		{
			mirror_config_horizontal(&position);
		}
		else
		{
			mirror_config_vertical(&position);
		}

		*mapped_addr = position & 0x7FF;
	}

	return mode;
}

void mapper009_init(nes_header_t* header, void* internal_registers)
{
	Registers_009* mapper = (Registers_009*)internal_registers;
	memset(mapper, 0, sizeof(Registers_009));
	mapper->mirroring_mode = header->nametable_arrangement ? 0 : 1;
}

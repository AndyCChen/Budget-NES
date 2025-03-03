#include "mapper_004.h"
#include "mirror_config.h"
#include "cpu.h"

#include <string.h>

static void mapper004_clock_irq(Registers_004* mapper);

cartridge_access_mode_t mapper004_cpu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	Registers_004* mapper = (Registers_004*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;
	

	// reading program ram if program ram enabled
	if (position >= 0x6000 && position <= 0x7FFF && mapper->prg_ram_enable)
	{
		mode = ACCESS_PRG_RAM;
		*mapped_addr = position & 0x1FFF;
	}
	else if (position >= 0x8000)
	{
		mode = ACCESS_PRG_ROM;

		if (position >= 0x8000 && position <= 0x9FFF)
		{
			// $8000 - $9FFF fixed to second - last bank
			if (mapper->prg_rom_bank_mode)
			{
				*mapped_addr = (position & 0x1FFF) + (((header->prg_rom_size * 2) - 2) * 0x2000);
			}
			// $8000-$9FFF swappable
			else
			{
				*mapped_addr = (position & 0x1FFF) + (mapper->bank_registers[6] * 0x2000);
			}
		}
		else if (position >= 0xA000 && position <= 0xBFFF)
		{
			*mapped_addr = (position & 0x1FFF) + (mapper->bank_registers[7] * 0x2000);
		}
		else if (position >= 0xC000 && position <= 0xDFFF)
		{
			// $C000-$DFFF swappable,
			if (mapper->prg_rom_bank_mode)
			{
				*mapped_addr = (position & 0x1FFF) + (mapper->bank_registers[6] * 0x2000);
			}
			// $C000-$DFFF fixed to second-last bank
			else
			{
				*mapped_addr = (position & 0x1FFF) + (((header->prg_rom_size * 2) - 2) * 0x2000);
			}
		}
		// position >= 0xE000
		else
		{
			// fix to last bank
			*mapped_addr = (position & 0x1FFF) + (((header->prg_rom_size * 2) - 1) * 0x2000);
		}
	}

	return mode;
}

cartridge_access_mode_t mapper004_cpu_write(nes_header_t* header, uint16_t position, uint8_t data, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	Registers_004* mapper = (Registers_004*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	// writing to program ram if program ram enabled
	if (position >= 0x6000 && position <= 0x7FFF && !mapper->prg_write_disable && mapper->prg_ram_enable)
	{
		mode = ACCESS_PRG_RAM;
		*mapped_addr = position & 0x1FFF;
	}
	// otherwise writing to cartridge mapper registers
	else if (position >= 0x8000 && position <= 0x9FFF)
	{

		// odd addresses select the high register
		if (position & 0x1)
		{
			uint8_t index = mapper->register_select;
			if (index == 6 || index == 7)
			{
				mapper->bank_registers[index] = data & 0x3F;
			}
			else if (index == 0 || index == 1)
			{
				mapper->bank_registers[index] = data & 0xFE;
			}
			else
			{
				mapper->bank_registers[index] = data;
			}
		}
		// even addresses select the low register (control register)
		else
		{
			mapper->register_select = data & 0x7;
			mapper->prg_rom_bank_mode = (data >> 6) & 0x1;
			mapper->chr_bank_mode = (data >> 7) & 0x1;
		}
	}
	else if (position >= 0xA000 && position <= 0xBFFF)
	{
		// odd address
		if (position & 0x1)
		{
			mapper->prg_write_disable = (data >> 6) & 0x1;
			mapper->prg_ram_enable = (data >> 7) & 0x1;
		}
		// even address
		else
		{
			mapper->mirroring_mode = data & 0x1;
		}
	}
	else if (position >= 0xC000 && position <= 0xDFFF)
	{
		// odd address
		if (position & 0x1)
		{
			mapper->irq_counter = 0;
		}
		// even address
		else
		{
			mapper->irq_counter_reload = data;
		}
	}
	else if (position >= 0xE000 && position <= 0xFFFF)
	{
		// odd address
		if (position & 0x1)
		{
			mapper->irq_enable = true;
		}
		// even address
		else
		{
			mapper->irq_enable = false;
			mapper->irq_pending = false; // acknowledge pending irqs
		}
	}
	
	return mode;
}

cartridge_access_mode_t mapper004_ppu_read(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	(void)header;
	Registers_004* mapper = (Registers_004*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	{
		size_t cpu_cycle_count = get_cpu()->cycle_count;
		if ((position & 0x1000) == 0 && cpu_cycle_count != mapper->cpu_timestamp)
		{
			mapper->M2 += 1;
		}
		else if ((position & 0x1000) && (mapper->A12 == 0) && mapper->M2 >= 3)
		{
			// rising edge of a12 decrements irq counter
			mapper004_clock_irq(internal_registers);
			mapper->M2 = 0;
		}
		else if ((position & 0x1000) && (mapper->A12 == 0) && mapper->M2 < 3)
		{
			mapper->M2 = 0;
		}

		mapper->cpu_timestamp = cpu_cycle_count;
		mapper->A12 = (position & 0x1000) >> 12;
	}

	// bank mode 1:
	// two 2 KB banks at $1000-$1FFF
	// four 1 KB banks at $0000 - $0FFF

	// bank mode 0:
	// two 2 KB banks at $0000 - $0FFF,
	//	four 1 KB banks at $1000 - $1FFF

	if (position <= 0x1FFF)
	{
		mode = ACCESS_CHR_MEM;

		if (position <= 0x3FF)
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[2] * 0x400);
			else
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[0] * 0x400);
		}
		else if (position <= 0x7FF)
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[3] * 0x400);
			else
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[0] * 0x400);
		}
		else if (position <= 0xBFF)
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[4] * 0x400);
			else
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[1] * 0x400);
		}
		else if (position <= 0xFFF)
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[5] * 0x400);
			else
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[1] * 0x400);
		}
		else if (position <= 0x13FF)
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[0] * 0x400);
			else
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[2] * 0x400);
		}
		else if (position <= 0x17FF)
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[0] * 0x400);
			else
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[3] * 0x400);
		}
		else if (position <= 0x1BFF)
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[1] * 0x400);
			else
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[4] * 0x400);
		}
		// position <= 0x1FFF
		else
		{
			if (mapper->chr_bank_mode)
				*mapped_addr = (position & 0x7FF) + (mapper->bank_registers[1] * 0x400);
			else
				*mapped_addr = (position & 0x3FF) + (mapper->bank_registers[5] * 0x400);
		}
	}
	// reading ppu nametable vram
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

cartridge_access_mode_t mapper004_ppu_write(nes_header_t* header, uint16_t position, size_t* mapped_addr, void* internal_registers)
{
	Registers_004* mapper = (Registers_004*)internal_registers;
	cartridge_access_mode_t mode = NO_CARTRIDGE_DEVICE;

	{
		size_t cpu_cycle_count = get_cpu()->cycle_count;
		if ((position & 0x1000) == 0 && cpu_cycle_count != mapper->cpu_timestamp)
		{
			mapper->M2 += 1;
		}
		else if ((position & 0x1000) && (mapper->A12 == 0) && mapper->M2 >= 3)
		{
			// rising edge of a12 decrements irq counter
			mapper004_clock_irq(internal_registers);
			mapper->M2 = 0;
		}
		else if ((position & 0x1000) && (mapper->A12 == 0) && mapper->M2 < 3)
		{
			mapper->M2 = 0;
		}

		mapper->cpu_timestamp = cpu_cycle_count;
		mapper->A12 = (position & 0x1000) >> 12;
	}

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
	// writting to ppu nametable vram
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

void mapper004_init(nes_header_t* header, void* internal_registers)
{
	Registers_004* mapper = (Registers_004*)internal_registers;
	memset(mapper, 0, sizeof(Registers_004));
	mapper->mirroring_mode = header->nametable_arrangement ? 0 : 1;
}

void mapper004_clock_irq(Registers_004* mapper)
{
	if (mapper->irq_counter == 0)
	{
		mapper->irq_counter = mapper->irq_counter_reload;
	}
	else
	{
		mapper->irq_counter -= 1;
	}
	
	if (mapper->irq_counter == 0 && mapper->irq_enable)
	{
		mapper->irq_pending = true;
	}
}

bool mapper004_irq_signaled(void* internal_registers)
{
	Registers_004* mapper = (Registers_004*)internal_registers;
	return mapper->irq_pending;
}


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "glad_loader/include/glad/glad.h"
#include "SDL.h"

#include "includes/apu.h"
#include "includes/cpu.h"
#include "includes/ppu.h"
#include "includes/cartridge.h"
#include "includes/log.h"
#include "includes/display.h"

static bool budgetNES_init(int argc, char *rom_path[]);
static void budgetNES_run(void);
static void budgetNES_shutdown(void);

int main(int argc, char *argv[])
{
   if ( !budgetNES_init( argc, argv ) )
   {
      return EXIT_FAILURE;
   }

	budgetNES_run();

   budgetNES_shutdown();

   return 0;
}

static bool budgetNES_init(int argc, char *rom_path[])
{
	if (!display_init() || !apu_init())
	{
		return false;
	}

	ppu_load_default_palettes();

   // try loading rom from command line argument if possible
   if (argc > 1)
   {
      if (!cartridge_load(rom_path[1]))
      {
         return false;
      }

      get_emulator_state()->run_state = EMULATOR_RUNNING;
		cpu_init();
		apu_pause(false);
   }
   return true;
}

static void budgetNES_run(void)
{
	Emulator_State_t* emulator_state = get_emulator_state();
	float delta_time = 0;
	float previous_time = 0;
	float current_time = 0;

	bool done = false;
	while (!done)
	{
		current_time = SDL_GetTicks64() / 1000.0f;
		delta_time += current_time - previous_time;

		display_process_event(&done);

		switch (emulator_state->run_state)
		{
			case EMULATOR_RUNNING:
			{
				cpu_run_with_audio(&delta_time);
				break;
			}
			case EMULATOR_PAUSED:
			{
				if (emulator_state->is_instruction_step)
				{
					cpu_emulate_instruction();
					emulator_state->is_instruction_step = false;
				}

				emulator_state->reset_delta_timers = true;
				break;
			}
			default:
				break;
		}

		display_render();
		display_update();
		previous_time = current_time;
	}
}

static void budgetNES_shutdown(void)
{
   apu_shutdown();
   log_free();
   cartridge_free_memory();
   display_shutdown();
}

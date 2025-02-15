#include "SDL_audio.h"

#include "../includes/apu.h"
#include "cpu.h"

#define DUTY_CYCLE_0 0x40 // duty cycle of 12.5%
#define DUTY_CYCLE_1 0x60 // duty cycle of 25%
#define DUTY_CYCLE_2 0x78 // duty cycle of 50%
#define DUTY_CYCLE_3 0x9F // duty cycle of 75%

static SDL_AudioDeviceID audio_device_ID;

static uint8_t        status = 0;
static Pulse_t        pulse_1;
static Pulse_t			 pulse_2;
static Framecounter_t frame_counter;
static size_t sequencer_timer_cpu_tick = 0; // elapsed apu cycles used to track when to clock the next sequence

// lookup table of values used in the lengh counter -> https://www.nesdev.org/wiki/APU_Length_Counter
static uint8_t length_lut[] = 
{
   10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14, 
   12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static void audio_callback(void* userdata, Uint8* stream, int length);

static void clock_pulse_sequencer(Pulse_t *pulse);
static void clock_pulse_envelope(Pulse_t *pulse);
static void clock_pulse_length_counter(Pulse_t *pulse);
static void clock_pulse_sweep(Pulse_t *pulse, uint8_t pulse_channel);
static void clock_quarter_frame(void);
static void clock_half_frame(void);
static bool pulse_sweep_forcing_silence(Pulse_t* pulse);

bool apu_init(void)
{
   SDL_AudioSpec want, have;

   SDL_zero(want);
   want.freq = 44100;
   want.format = AUDIO_U16SYS;
   want.samples = 1024;
   want.channels = 1;
   want.callback = &audio_callback;

   audio_device_ID = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
   if (audio_device_ID == 0)
   {
      printf("Failed to initialize audio device: %s\n", SDL_GetError());
      return false;
   }

   return true;
}

void apu_shutdown(void)
{
   SDL_CloseAudioDevice(audio_device_ID);
}

void apu_pause(bool flag)
{
	SDL_PauseAudioDevice(audio_device_ID, flag ? 1 : 0);
}

void apu_write(uint16_t position, uint8_t data)
{
   switch (position)
   {
      // pulse 1 channel

      case 0x4000:
      {
         // duty cycle mode -> https://www.nesdev.org/wiki/APU_Pulse
         switch ( (data & 0xC0) >> 6 )
         {
            case 0:
               pulse_1.sequence_reload = DUTY_CYCLE_0; // duty cycle of 12.5%
               break;
            case 1:
               pulse_1.sequence_reload = DUTY_CYCLE_1; // duty cycle of 25%
               break;
            case 2:
               pulse_1.sequence_reload = DUTY_CYCLE_2; // duty cycle of 50%
               break;
            case 3:
               pulse_1.sequence_reload = DUTY_CYCLE_3; // duty cycle of 75%
               break;
            default:
               break;
         }

         pulse_1.volume = data & 0x0F;
         pulse_1.length_counter_halt = (data & 0x20) >> 5;
         pulse_1.constant_volume_enable = (data & 0x10) >> 4;
         pulse_1.sequence = pulse_1.sequence_reload;

         break;
      }
      case 0x4001: // eppp nsss        enable, period, negate, shift
      {
         pulse_1.sweep_reload = (data & 0x70) >> 4; // period
         pulse_1.sweep_negate = (data & 0x08) >> 3;
         pulse_1.sweep_shift =  (data & 0x07);
         pulse_1.sweep_enable = pulse_1.sweep_shift != 0 ? (data & 0x80) >> 7 : 0;
			pulse_1.sweep_reset = true;

         break;
      }
      case 0x4002:
      {
			pulse_1.timer_reload = (pulse_1.timer_reload & 0x700) | data; // set low 8 bits of reload timer
         break;
      }
      case 0x4003:
      {
         pulse_1.timer_reload = (pulse_1.timer_reload & 0x00FF) | ((data & 0x7) << 8); // set high 3 bits of reload timer
         pulse_1.timer = pulse_1.timer_reload;
         pulse_1.sequence = pulse_1.sequence_reload;
         pulse_1.envelope_reset = true;

         if (pulse_1.channel_enable == true) 
				pulse_1.length_counter = length_lut[ (data >> 3) & 0x1F ];
         break;
      }

		// pulse 2

		case 0x4004:
		{
			switch ((data & 0xC0) >> 6)
			{
				case 0:
					pulse_2.sequence_reload = DUTY_CYCLE_0; // duty cycle of 12.5%
					break;
				case 1:
					pulse_2.sequence_reload = DUTY_CYCLE_1; // duty cycle of 25%
					break;
				case 2:
					pulse_2.sequence_reload = DUTY_CYCLE_2; // duty cycle of 50%
					break;
				case 3:
					pulse_2.sequence_reload = DUTY_CYCLE_3; // duty cycle of 75%
					break;
				default:
					break;
			}

			pulse_2.volume = data & 0x0F;
			pulse_2.length_counter_halt = (data & 0x20) >> 5;
			pulse_2.constant_volume_enable = (data & 0x10) >> 4;
			pulse_2.sequence = pulse_2.sequence_reload;

			break;
		}
		case 0x4005:
		{
			pulse_2.sweep_reload = (data & 0x70) >> 4; // period
			pulse_2.sweep_negate = (data & 0x08) >> 3;
			pulse_2.sweep_shift = (data & 0x07);
			pulse_2.sweep_enable = pulse_2.sweep_shift != 0 ? (data & 0x80) >> 7 : 0;
			pulse_2.sweep_reset = true;

			break;
		}
		case 0x4006:
		{
			pulse_2.timer_reload = (pulse_2.timer_reload & 0x0700) | data;
			break;
		}
		case 0x4007:
		{
			pulse_2.timer_reload = (pulse_2.timer_reload & 0x00FF) | ((data & 0x7) << 8); // set high 3 bits of reload timer
			pulse_2.timer = pulse_2.timer_reload;

			pulse_2.sequence = pulse_2.sequence_reload;
			pulse_2.envelope_reset = true;
			if (pulse_2.channel_enable == true) 
				pulse_2.length_counter = length_lut[(data >> 3) & 0x1F];

			break;
		}


      // todo: other 3 channels

      // status register
      case 0x4015:
      {
         status = data & 0x1F;
         
			pulse_1.channel_enable = status & 0x1;
         if (pulse_1.channel_enable == false) 
            pulse_1.length_counter = 0;

			pulse_2.channel_enable = (status & 0x2) >> 1;
			if (pulse_2.channel_enable == false)
				pulse_2.length_counter = 0;

         break;
      }
      // frame counter
      case 0x4017:
      {
         frame_counter.sequencer_mode = (data >> 7) & 0x1;
         frame_counter.IRQ_inhibit    = (data >> 6) & 0x1;
			sequencer_timer_cpu_tick = 0;

			if (frame_counter.sequencer_mode)
			{
				clock_quarter_frame();
				clock_half_frame();
			}
			
         break;
      }

      default: break;
   }
}

/**
 * Clocks the apu for one cycle
 */
void apu_tick(void)
{
   
   bool quarterFrame = false;
   bool halfFrame = false;

   ++sequencer_timer_cpu_tick;

   if (frame_counter.sequencer_mode == 0) // 4-step mode
   {
      if (sequencer_timer_cpu_tick == 7457)
      {
         quarterFrame = true;
      }
      else if (sequencer_timer_cpu_tick == 14913)
      {
         quarterFrame = true;
         halfFrame = true;
      }
      else if (sequencer_timer_cpu_tick == 22371)
      {
         quarterFrame = true;
      }
      else if (sequencer_timer_cpu_tick == 29829)
      {
         quarterFrame = true;
         halfFrame = true;
         sequencer_timer_cpu_tick = 0;
      } 
   }
   else // 5-step mode
   {
		if (sequencer_timer_cpu_tick == 7457)
		{
			quarterFrame = true;
		}
		else if (sequencer_timer_cpu_tick == 14913)
		{
			quarterFrame = true;
			halfFrame = true;
		}
		else if (sequencer_timer_cpu_tick == 22371)
		{
			quarterFrame = true;
		}
		else if (sequencer_timer_cpu_tick == 37281)
		{
			quarterFrame = true;
			halfFrame = true;
			sequencer_timer_cpu_tick = 0;
		}
   }

	if (quarterFrame)
	{
		clock_quarter_frame();
	}
	if (halfFrame)
	{
		clock_half_frame();
	}
	
	static bool even = true;
	if (even)
	{
		clock_pulse_sequencer(&pulse_1);
		clock_pulse_sequencer(&pulse_2);
	}
	even = !even;

	if (pulse_1.length_counter != 0 && !pulse_sweep_forcing_silence(&pulse_1))
	{
		if (pulse_1.constant_volume_enable)
		{
			pulse_1.raw_samples[pulse_1.raw_sample_index] = pulse_1.raw_sample * pulse_1.volume;
		}
		else
		{
			pulse_1.raw_samples[pulse_1.raw_sample_index] = pulse_1.raw_sample * pulse_1.envelope_volume;
		}
	}
	else
	{
		pulse_1.raw_samples[pulse_1.raw_sample_index] = 0;
	}

	if (pulse_2.length_counter != 0 && !pulse_sweep_forcing_silence(&pulse_2))
	{
		if (pulse_2.constant_volume_enable)
		{
			pulse_2.raw_samples[pulse_2.raw_sample_index] = pulse_2.raw_sample * pulse_2.volume;
		}
		else
		{
			pulse_2.raw_samples[pulse_2.raw_sample_index] = pulse_2.raw_sample * pulse_2.envelope_volume;
		}
	}
	else
	{
		pulse_2.raw_samples[pulse_2.raw_sample_index] = 0;
	}

	pulse_1.raw_sample_index = (pulse_1.raw_sample_index + 1) % 41;
	pulse_2.raw_sample_index = (pulse_2.raw_sample_index + 1) % 41;
}

static void clock_quarter_frame(void)
{
	clock_pulse_envelope(&pulse_1);
	clock_pulse_envelope(&pulse_2);
}

static void clock_half_frame(void)
{
	clock_pulse_sweep(&pulse_1, 1);
	clock_pulse_sweep(&pulse_2, 2);
	clock_pulse_length_counter(&pulse_1);
	clock_pulse_length_counter(&pulse_2);
}

static void clock_pulse_sequencer(Pulse_t *pulse)
{
   if (pulse->timer > 0)
   {
      pulse->timer -= 1;
   }
   else
   {
      pulse->timer = pulse->timer_reload;
      pulse->raw_sample = pulse->sequence & 1;
      pulse->sequence = ( (pulse->sequence & 0x1) << 7 ) | (pulse->sequence >> 1);
   }
}

static void clock_pulse_envelope(Pulse_t *pulse)
{
   if (pulse->envelope_reset)
   {
      pulse->envelope_reset = false;
      pulse->envelope_volume = 0xF;
		pulse->envelope_counter = pulse->volume;
   }
   else
   {
      if (pulse->envelope_counter > 0)
      {
         pulse->envelope_counter -= 1;
      }
      else
      {
         pulse->envelope_counter = pulse->volume;
			if (pulse->envelope_volume > 0)
			{
				pulse->envelope_volume -= 1;
			}
			else if (pulse->length_counter_halt)
			{
				pulse->envelope_volume = 0xF;
			}
      }
   }
}

static void clock_pulse_length_counter(Pulse_t *pulse)
{
   if (!pulse->length_counter_halt && pulse->length_counter > 0)
   {
      pulse->length_counter -= 1;
   }
}

static void clock_pulse_sweep(Pulse_t *pulse, uint8_t pulse_channel)
{
	if (pulse->sweep_reset)
	{
		pulse->sweep_counter = pulse->sweep_reload;
		pulse->sweep_reset = false;
	}
	else if (pulse->sweep_counter > 0)
   {
      pulse->sweep_counter -= 1;
   }
   else
   {
		pulse->sweep_counter = pulse->sweep_reload;

      if (pulse->sweep_enable && !pulse_sweep_forcing_silence(pulse))
      {
			int offset = pulse->timer_reload >> pulse->sweep_shift;
         if (pulse->sweep_negate)
         {
				int target_period = 0;
				if (pulse_channel == 1)
				{
					target_period = pulse->timer_reload - (offset + 1);
				}
				else
				{
					target_period = pulse->timer_reload - offset;
				}

				pulse->timer_reload = target_period < 0 ? 0 : target_period;
         }
         else
         {
            pulse->timer_reload += offset;
         }

			
      }
   }
}

static bool pulse_sweep_forcing_silence(Pulse_t* pulse)
{
	if (pulse->timer_reload < 8)
	{
		return true;
	}
	else if (!pulse->sweep_negate && (pulse->timer_reload + (pulse->timer_reload >> pulse->sweep_shift)) > 0x7FF)
	{
		return true;
	}
	else
	{
		return false;
	}
}

static void audio_callback(void* userdata, Uint8* stream, int length)
{
	Uint16* audio_buffer = (Uint16*) stream;
	int audio_buffer_length = length / 2;
	size_t* runningSampleIndex = (size_t*) userdata;

	for (size_t sample_index = 0; sample_index < audio_buffer_length; ++sample_index)
	{
		cpu_run_with_audio();
		audio_buffer[sample_index] = apu_get_output_sample();
	}

}

int16_t apu_get_output_sample(void)
{
	/*uint16_t raw = pulse_1.raw_sample;
	uint16_t raw2 = pulse_2.raw_sample;

   if (pulse_1.length_counter != 0)
   {
      if (pulse_1.constant_volume_enable)
      {
         raw *= pulse_1.volume;
      }
      else
      {
         raw *= pulse_1.envelope_volume;
      }
   }
   else
   {
      raw = 0;
   }


	if (pulse_2.length_counter != 0)
	{
		if (pulse_2.constant_volume_enable)
		{
			raw2 *= pulse_2.volume;
		}
		else
		{
			raw2 *= pulse_2.envelope_volume;
		}
	}
	else
	{
		raw2 = 0;
	}*/

	float raw = 0;
	float raw2 = 0;
	for (int i = 0; i < 41; ++i)
	{
		raw += pulse_1.raw_samples[i];
		raw2 += pulse_2.raw_samples[i];
	}
	raw = raw / 41.0f;
	raw2 = raw2 / 41.0f;

	float output = 95.88f / ((8128.0f / (raw + raw2)) + 100);

	int res = (output * 65536) - 32768;
	if (res > 32767)
	{
		res = 32767;
	}
	else if (res < -32768)
	{
		res = -32768;
	}

   return  res;
}

uint8_t apu_read_status(void)
{
   return status;
}

#include "SDL_audio.h"

#include "apu.h"
#include "cpu.h"
#include "bus.h"
#include "cartridge.h"
#include "CBlip_buffer.h"

#define DUTY_CYCLE_0 0x40 // duty cycle of 12.5%
#define DUTY_CYCLE_1 0x60 // duty cycle of 25%
#define DUTY_CYCLE_2 0x78 // duty cycle of 50%
#define DUTY_CYCLE_3 0x9F // duty cycle of 75%

static SDL_AudioDeviceID audio_device_ID;

static Pulse_t        pulse_1;
static Pulse_t			 pulse_2;
static Triangle_t     triangle_1;
static Noise_t        noise_1;
static Dmc_t          dmc_1;
static Framecounter_t frame_counter;
static size_t sequencer_timer_cpu_tick = 0; // elapsed apu cycles used to track when to clock the next sequence

static bool frame_interrupt_flag;
static bool dmc_interrupt_flag;

static CBlip_Buffer* buffer;
static CBlipSynth synth_1;

// lookup table of values used in the lengh counter -> https://www.nesdev.org/wiki/APU_Length_Counter
static uint8_t length_lut[] = 
{
   10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14, 
   12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// looping 32 step sequence of the triangle wave
static uint8_t triangle_sequence_lut[] =
{
	15, 14, 13, 12, 11, 10,  9, 8, 7, 6,  5,  4,  3,  2,  1,  0,
	 0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
};

static uint16_t noise_period_lut[] =
{
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

static uint16_t dmc_period_lut[] =
{
	428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
};

static void clock_quarter_frame(void);
static void clock_half_frame(void);

static void clock_pulse_sequencer(Pulse_t *pulse);
static void clock_pulse_envelope(Pulse_t *pulse);
static void clock_pulse_length_counter(Pulse_t *pulse);

static void clock_pulse_sweep(Pulse_t *pulse, uint8_t pulse_channel);
static bool pulse_sweep_forcing_silence(Pulse_t* pulse);

static void clock_triangle_sequencer(Triangle_t* triangle);
static void clock_triangle_length_counter(Triangle_t* triangle);
static void clock_triangle_linear_counter(Triangle_t* triangle);

static void clock_noise_sequencer(Noise_t* noise);
static void clock_noise_length_counter(Noise_t* noise);
static void clock_noise_envelope(Noise_t* noise);

static void clock_dmc_sequencer(Dmc_t* dmc);
static void dmc_memory_reader(Dmc_t* dmc);

static void mix_audio(long time, float p1, float p2, float t1, float n1, float d1);

bool apu_init(void)
{
   SDL_AudioSpec want, have;

   SDL_zero(want);
   want.freq = 44100;
   want.format = AUDIO_S16SYS;
   want.samples = 1024;
   want.channels = 1;

   audio_device_ID = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
   if (audio_device_ID == 0)
   {
      printf("Failed to initialize audio device: %s\n", SDL_GetError());
      return false;
   }

	buffer = create_cblip_buffer();
	synth_1 = create_cblip_synth();

	if (!buffer)
		return false;
	if (!synth_1)
		return false;

	cblip_buffer_clock_rate(buffer, 1800000);
	if (cblip_buffer_set_sample_rate(buffer, 44100, 1000/60))
		return false;

	cblip_synth_volume(synth_1, 0.01);
	cblip_synth_output(synth_1, buffer);
	cblip_buffer_bass_freq(buffer, 1);
	cblip_synth_treble_eq(synth_1, 5.0);


	apu_reset_internals();

   return true;
}

void apu_reset_internals(void)
{
	frame_interrupt_flag = false;
	dmc_interrupt_flag = false;

	memset(&pulse_1, 0, sizeof(Pulse_t));
	memset(&pulse_2, 0, sizeof(Pulse_t));
	memset(&triangle_1, 0, sizeof(Triangle_t));
	memset(&noise_1, 0, sizeof(Noise_t));
	memset(&dmc_1, 0, sizeof(Dmc_t));

	noise_1.shift_register = 1;
	dmc_1.silence_flag = true;
}

void apu_shutdown(void)
{
	free_cblip_buffer(buffer);
	free_cblip_synth(synth_1);
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
			pulse_2.sequence = pulse_2.sequence_reload;
			pulse_2.envelope_reset = true;

			if (pulse_2.channel_enable == true) 
				pulse_2.length_counter = length_lut[(data >> 3) & 0x1F];

			break;
		}

		// triangle channel

		case 0x4008:
		{
			triangle_1.control_flag = (data & 0x80) >> 7;
			triangle_1.linear_counter_reload = data & 0x7F;

			break;
		}
		// 0x4009 is unused
		case 0x400A:
		{
			triangle_1.timer_reload = (triangle_1.timer_reload & 0x0700) | data; // lo 8 bits of 11 bit timer
			break;
		}
		case 0x400B:
		{
			triangle_1.timer_reload = (triangle_1.timer_reload & 0x00FF) | ((data & 0x7) << 8); // hi 3 bits of 11 bit timer
			triangle_1.length_counter = length_lut[(data >> 3) & 0x1F];
			triangle_1.linear_counter_reset = true;
			break;
		}

		// noise channel

		case 0x400C:
		{
			noise_1.volume = data & 0xF;
			noise_1.constant_volume_enable = (data >> 4) & 0x1;
			noise_1.length_counter_halt = (data >> 5) & 0x1;

			break;
		}
		case 0x400E:
		{
			noise_1.noise_mode = (data >> 7) & 0x1;
			noise_1.timer_reload = noise_period_lut[data & 0xF];

			break;
		}
		// 0x400D is unused
		case 0x400F:
		{
			noise_1.envelope_reset = true;
			if (noise_1.channel_enable)
				noise_1.length_counter = length_lut[(data >> 3) & 0x1F];

			break;
		}

      // dmc channel

		case 0x4010:
		{
			dmc_1.irq_enable = (data >> 7) & 0x1;
			if (dmc_1.irq_enable == false) // clear interupt flag if irq enable is also cleared
				dmc_interrupt_flag = false;

			dmc_1.loop_flag = (data >> 6) & 0x1;
			dmc_1.timer_reload = dmc_period_lut[data & 0xF];

			break;
		}
		case 0x4011:
		{
			dmc_1.out = data & 0x7F;
			break;
		}
		case 0x4012:
		{
			// %11AAAAAA.AA000000 = $C000 + (data * 64)

			dmc_1.sample_address = 0xC000 | (data << 6);
			break;
		}
		case 0x4013:
		{
			// %LLLL.LLLL0001 = (L * 16) + 1 bytes

			dmc_1.sample_bytes_length = 0x001 | (data << 4);
			break;
		}

      // status register
      case 0x4015:
      {
			pulse_1.channel_enable = data & 0x1;
         if (pulse_1.channel_enable == false) 
            pulse_1.length_counter = 0;

			pulse_2.channel_enable = (data & 0x2) >> 1;
			if (pulse_2.channel_enable == false)
				pulse_2.length_counter = 0;

			triangle_1.channel_enable = (data & 0x4) >> 2;
			if (triangle_1.channel_enable == false)
				triangle_1.length_counter = 0;

			noise_1.channel_enable = (data & 0x8) >> 3;
			if (noise_1.channel_enable == false)
				noise_1.length_counter = 0;

			dmc_1.channel_enable = (data & 0x10) >> 4;
			if (dmc_1.channel_enable)
			{
				if (dmc_1.sample_bytes_remaining == 0) // restart dmc sample when remain samples is zero
				{
					dmc_1.sample_bytes_remaining = dmc_1.sample_bytes_length;
					dmc_1.current_sample_address = dmc_1.sample_address;
				}
			}
			else // set remaining sample bytes to zero when dmc is disabled
				dmc_1.sample_bytes_remaining = 0;


			dmc_interrupt_flag = false; // clear/acknowledge interrupt flag on status write

         break;
      }
      // frame counter
      case 0x4017:
      {
         frame_counter.sequencer_mode = (data >> 7) & 0x1;
         frame_counter.IRQ_inhibit    = (data >> 6) & 0x1;
			sequencer_timer_cpu_tick = 0;

			if (frame_counter.IRQ_inhibit)
				frame_interrupt_flag = false;

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
void apu_tick(long audio_time)
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
			if (frame_counter.IRQ_inhibit == 0)
				frame_interrupt_flag = true;

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
	
	// pulse is clocked every 2nd cpu cycle
	static bool even = true;
	if (even)
	{
		clock_pulse_sequencer(&pulse_1);
		clock_pulse_sequencer(&pulse_2);
	}
	even = !even;

	// triangle channel clocked every cpu cycle
	clock_triangle_sequencer(&triangle_1);
	clock_noise_sequencer(&noise_1);
	clock_dmc_sequencer(&dmc_1);
	dmc_memory_reader(&dmc_1);

	if (pulse_1.raw_sample != 0 && pulse_1.length_counter != 0 && !pulse_sweep_forcing_silence(&pulse_1))
	{
		if (pulse_1.constant_volume_enable)
		{
			pulse_1.out = pulse_1.volume;
		}
		else
		{
			pulse_1.out = pulse_1.envelope_volume;
		}
	}
	else
	{
		pulse_1.out = 0;
	}

	if (pulse_2.raw_sample != 0 && pulse_2.length_counter != 0 && !pulse_sweep_forcing_silence(&pulse_2))
	{
		if (pulse_2.constant_volume_enable)
		{
			pulse_2.out = pulse_2.volume;
		}
		else
		{
			pulse_2.out = pulse_2.envelope_volume;
		}
	}
	else
	{
		pulse_2.out = 0;
	}

	triangle_1.out = triangle_1.raw_sample;

	if (noise_1.length_counter != 0 && ((noise_1.shift_register & 0x1) == 0))
	{
		if (noise_1.constant_volume_enable)
		{
			noise_1.raw_samples[noise_1.raw_sample_index] = noise_1.volume;
			noise_1.out = noise_1.volume;
		}
		else
		{
			noise_1.raw_samples[noise_1.raw_sample_index] = noise_1.envelope_volume;
			noise_1.out = noise_1.envelope_volume;
		}
	}
	else
	{
		noise_1.raw_samples[noise_1.raw_sample_index] = 0;
		noise_1.out = 0;
	}

	mix_audio(
		audio_time,
		pulse_1.out,
		pulse_2.out,
		triangle_1.out,
		noise_1.out,
		(float)(dmc_1.out & 0x7F)
	);

	noise_1.raw_sample_index = (noise_1.raw_sample_index + 1) % 41;
}

static void clock_quarter_frame(void)
{
	clock_pulse_envelope(&pulse_1);
	clock_pulse_envelope(&pulse_2);
	clock_triangle_linear_counter(&triangle_1);
	clock_noise_envelope(&noise_1);
}

static void clock_half_frame(void)
{
	clock_pulse_sweep(&pulse_1, 1);
	clock_pulse_sweep(&pulse_2, 2);
	clock_pulse_length_counter(&pulse_1);
	clock_pulse_length_counter(&pulse_2);
	clock_triangle_length_counter(&triangle_1);
	clock_noise_length_counter(&noise_1); 
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
   else if (pulse->envelope_counter > 0)
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
		else if (pulse->length_counter_halt) // looping envelope
		{
			pulse->envelope_volume = 0xF;
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

				pulse->timer_reload = target_period < 0 ? 0 : (uint16_t) target_period;
         }
         else
         {
            pulse->timer_reload += (uint16_t) offset;
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

static void clock_triangle_sequencer(Triangle_t* triangle)
{
	if (triangle->timer > 0)
	{
		triangle->timer -= 1;
	}
	else
	{
		triangle->timer = triangle->timer_reload;
		if (triangle->length_counter > 0 && triangle->linear_counter > 0)
		{
			triangle->raw_sample = triangle_sequence_lut[triangle->sequence_step];
			triangle->sequence_step = (triangle->sequence_step + 1) & 0x1F;
		}
	}
}

static void clock_triangle_length_counter(Triangle_t* triangle)
{
	if (!triangle->control_flag && triangle->length_counter > 0)
	{
		triangle->length_counter -= 1;
	}
}

static void clock_triangle_linear_counter(Triangle_t* triangle)
{
	if (triangle->linear_counter_reset)
	{
		triangle->linear_counter = triangle->linear_counter_reload;
		triangle->linear_counter_reset = triangle->control_flag;
	}
	else if (triangle->linear_counter > 0)
	{
		triangle->linear_counter -= 1;
	}
}

static void clock_noise_sequencer(Noise_t* noise)
{
	if (noise->timer > 0)
	{
		noise->timer -= 1;
	}
	else
	{
		noise->timer = noise->timer_reload;
		
		// xor zero-th bit with another bit depending on noise mode
		uint8_t bit_0 = noise->shift_register & 0x1;
		uint8_t bit_1 = 0;

		if (noise->noise_mode) // xor with the bit 6 if mode is set
		{
			bit_1 = (noise->shift_register >> 6) & 0x1;
		}
		else                  // xor with the bit 1 if mode is cleared
		{
			bit_1 = (noise->shift_register >> 1) & 0x1;
		}

		uint8_t feedback = bit_0 ^ bit_1;
		noise->shift_register = noise->shift_register >> 1;
		noise->shift_register |= feedback << 14;
	}
}

static void clock_noise_length_counter(Noise_t* noise)
{
	if (!noise->length_counter_halt && noise->length_counter > 0)
	{
		noise->length_counter -= 1;
	}
}

static void clock_noise_envelope(Noise_t* noise)
{
	if (noise->envelope_reset)
	{
		noise->envelope_reset = false;
		noise->envelope_volume = 0xF;
		noise->envelope_counter = noise->volume;
	}
	else if (noise->envelope_counter > 0)
	{
		noise->envelope_counter -= 0;
	}
	else
	{
		noise->envelope_counter = noise->volume;
		if (noise->envelope_volume > 0)
		{
			noise->envelope_volume -= 1;
		}
		else if (noise->length_counter_halt) // looping envelope
		{
			noise->envelope_volume = 0xF;
		}
	}
}

void clock_dmc_sequencer(Dmc_t* dmc)
{
	if (dmc->timer > 0)
	{
		dmc->timer -= 1;
	}
	else
	{
		dmc->timer = dmc->timer_reload;

		if (dmc->silence_flag == false)
		{
			// add 2 to output if bit 0 is set
			// dmc output level ranges 0-127, so clamp as needed
			if (dmc->shift_register & 0x1)
			{
				if (dmc->out <= 125)
					dmc->out += 2;
			}
			else // else subtract 2
			{
				if (dmc->out >= 2)
					dmc->out -= 2;
			}
		}

		dmc->shift_register = dmc->shift_register >> 1;

		if (dmc->bits_remaining > 0)
		{
			dmc->bits_remaining -= 1;
		}
		else
		{
			dmc->bits_remaining = 8;

			if (dmc->sample_buffer_filled) // empty sample buffer into shift register
			{
				dmc->shift_register = dmc->sample_buffer;
				dmc->sample_buffer_filled = false;
				dmc->silence_flag = false;
			}
			else // sample buffer is empty
			{
				dmc->silence_flag = true;
			}
		}
	}
}

void dmc_memory_reader(Dmc_t* dmc)
{
	// sample buffer is empty and remaining sample bytes are non-zero
	if (dmc->sample_buffer_filled == false && dmc->sample_bytes_remaining > 0)
	{
		dmc->sample_buffer = cartridge_cpu_read(dmc->current_sample_address);
		dmc->sample_buffer_filled = true;

		dmc->current_sample_address = (dmc->current_sample_address + 1) | 0x8000; // wrap back to 0x8000 if increment exceeds 0xFFFF
		dmc->sample_bytes_remaining -= 1;

		if (dmc->sample_bytes_remaining == 0)
		{
			if (dmc->loop_flag)
			{
				dmc->current_sample_address = dmc->sample_address;
				dmc->sample_bytes_remaining = dmc->sample_bytes_length;
			}
			else if (dmc->irq_enable)
			{
				dmc_interrupt_flag = true;
			}
		}
	}
}

static void mix_audio(long time, float p1, float p2, float t1, float n1, float d1)
{
	float pulse_out = (p1+p2) ? 95.88f / (8128.0f / (p1 + p2) + 100) : 0.0f;

	t1 = t1 / 8227;
	n1 = n1 / 12241;
	d1 = d1 / 22638;

	float tnd_out = (t1 + n1 + d1) ? 159.79f / (1 / (t1 + n1 + d1) + 100) : 0.0f;

	int output = (int) (((pulse_out + tnd_out) * 0.01f * 65536) - 32767);
	if (output > 32767)
	{
		output = 32767;
	}
	else if (output < -32768)
	{
		output = -32768;
	}
	output = (int) (output * 0.5f);

	cblip_synth_update(synth_1, time, output);
}

uint32_t apu_get_queued_audio()
{
	return SDL_GetQueuedAudioSize(audio_device_ID);
}

void apu_queue_audio_frame(long audio_frame_length)
{
	cblip_buffer_end_frame(buffer, audio_frame_length);
	short samples[735];
	long count = cblip_buffer_read_samples(buffer, samples, 735);

	SDL_QueueAudio(audio_device_ID, samples, sizeof(short) * count);
}

void apu_clear_queued_audio(void)
{
	cblip_buffer_clear(buffer);
	SDL_ClearQueuedAudio(audio_device_ID);
}

bool apu_is_triggering_irq(void)
{
	return frame_interrupt_flag || dmc_interrupt_flag;
}

uint8_t apu_read_status(void)
{
	uint8_t status = 0;
	if (pulse_1.length_counter > 0)
		status |= 0x1;
	if (pulse_2.length_counter > 0)
		status |= 0x1 << 1;
	if (triangle_1.length_counter > 0)
		status |= 0x1 << 2;
	if (noise_1.length_counter > 0)
		status |= 0x1 << 3;
	if (dmc_1.sample_bytes_remaining > 0)
		status |= 0x1 << 4;

	status |= (frame_interrupt_flag & 0x1) << 6;
	frame_interrupt_flag = false; // clear/acknowledge frame interrupt when status is read

	status |= (dmc_interrupt_flag & 0x1) << 7;

   return status;
}

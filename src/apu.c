#include "SDL_audio.h"

#include "../includes/apu.h"

#define DUTY_CYCLE_0 0x40 // duty cycle of 12.5%
#define DUTY_CYCLE_1 0x60 // duty cycle of 25%
#define DUTY_CYCLE_2 0x78 // duty cycle of 50%
#define DUTY_CYCLE_3 0x9F // duty cycle of 75%

#define AUDIO_FREQUENCY 41000

static SDL_AudioDeviceID audio_device_ID;

static uint8_t        status = 0;
static Pulse_t        pulse_1;
static Framecounter_t frame_counter;

// lookup table of values used in the lengh counter -> https://www.nesdev.org/wiki/APU_Length_Counter
static uint8_t length_lut[] = 
{
   10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14, 
   12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

//static void clock_pulse_sequencer(Pulse_t *pulse);

bool apu_init(void)
{
   SDL_AudioSpec want, have;

   SDL_zero(want);
   want.freq = AUDIO_FREQUENCY;
   want.format = AUDIO_S16SYS;
   want.channels = 1;
   want.callback = NULL;

   audio_device_ID = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
   if (audio_device_ID == 0)
   {
      printf("Failed to initialize audio device: %s\n", SDL_GetError());
      return false;
   }

   SDL_PauseAudioDevice(audio_device_ID, 0);
   return true;
}

void apu_shutdown(void)
{
   SDL_CloseAudioDevice(audio_device_ID);
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
         pulse_1.sequence = pulse_1.sequence_reload;

         break;
      }
      case 0x4001: break;
      case 0x4002:
      {
         pulse_1.timer_reload = (pulse_1.timer_reload & 0x0700) | data; // set low 8 bits of reload timer
         break;
      }
      case 0x4003:
      {
         pulse_1.timer_reload = (pulse_1.timer_reload & 0x00FF) | ( (data & 0x7) << 8 ); // set high 3 bits of reload timer
         pulse_1.timer = pulse_1.timer_reload;

         pulse_1.sequence = pulse_1.sequence_reload;
         if (pulse_1.enable == true) pulse_1.length_counter = length_lut[ (data >> 3) & 0x1F ];
         break;
      }

      // todo: other 4 channels

      // status register
      case 0x4015:
      {
         status = data;
         pulse_1.enable = data & 0x1;
         if (pulse_1.enable == false) pulse_1.length_counter = 0;
         break;
      }
      // frame counter
      case 0x4017:
      {
         frame_counter.sequencer_mode = (data >> 7) & 0x1;
         frame_counter.IRQ_inhibit    = (data >> 6) & 0x1;
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
   static size_t apu_cycles = 0; // elapsed apu cycles used to track when to clock the next sequence
   bool quarterFrame = false;
   bool halfFrame = false;

   ++apu_cycles;

   if (frame_counter.sequencer_mode == 0) // 4-step mode
   {
      if (apu_cycles == 3729)
      {
         quarterFrame = true;
      }
      else if (apu_cycles == 7457)
      {
         quarterFrame = true;
         halfFrame = true;
      }
      else if (apu_cycles == 11186)
      {
         quarterFrame = true;
      }
      else if (apu_cycles == 14915)
      {
         quarterFrame = true;
         halfFrame = true;
         apu_cycles = 0;
      }

      if (quarterFrame)
      {

      }

      if (halfFrame)
      {
         // clock the length counter
         if (pulse_1.length_counter > 0) pulse_1.length_counter -= 1;
      }
   }
   else // 5-step mode
   {

   }

   if (pulse_1.timer > 0)
   {
      pulse_1.timer -= 1;
   }
   else
   {
      pulse_1.timer = pulse_1.timer_reload;
      pulse_1.raw_sample = pulse_1.sequence & 0x1;
      pulse_1.sequence = ( (pulse_1.sequence & 0x1) << 7 ) | (pulse_1.sequence >> 1);
   }

   

}

uint8_t apu_read_status(void)
{
   return status;
}

uint8_t apu_get_output_sample(void)
{
   static uint8_t counter = 0;
   ++counter;
   if (counter % 42 == 0)
   {
      uint16_t raw =  pulse_1.raw_sample * 2;
      SDL_QueueAudio(audio_device_ID, &raw, 1);
      counter = 0;
   }
   return 0;
}

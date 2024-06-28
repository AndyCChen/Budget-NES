#include "SDL_audio.h"

#include "../includes/apu.h"

static Pulse_t    pulse_1;
static Pulse_t    pulse_2;
static Triangle_t triangle;
static Noise_t    noise;
static DMC_t      dmc;
static uint8_t    status;
static uint8_t    frame_counter;

bool apu_init(void)
{
   bool flag = true;




   return flag;
}

void apu_write(uint16_t position, uint8_t data)
{
   switch (position)
   {
      // pulse 1
      case 0x4000: pulse_1.volume          = data; break;
      case 0x4001: pulse_1.sweep           = data; break;
      case 0x4002: pulse_1.lo              = data; break;
      case 0x4003: pulse_1.hi              = data; break;

      // pulse 2
      case 0x4004: pulse_2.volume          = data; break;
      case 0x4005: pulse_2.sweep           = data; break;
      case 0x4006: pulse_2.lo              = data; break;
      case 0x4007: pulse_2.hi              = data; break;

      // triangle
      case 0x4008: triangle.linear_counter = data; break;
      case 0x400A: triangle.lo             = data; break; 
      case 0x400B: triangle.hi             = data; break;

      // noise
      case 0x400C: noise.volume            = data; break;
      case 0x400E: noise.lo                = data; break;
      case 0x400F: noise.hi                = data; break;

      // dmc
      case 0x4010: dmc.frequency           = data; break;
      case 0x4011: dmc.raw                 = data; break;
      case 0x4012: dmc.start               = data; break;
      case 0x4013: dmc.length              = data; break;

      // status
      case 0x4015: status                  = data; break;

      // frame counter control
      case 0x4017: frame_counter           = data; break;

      default: break;
   }
}

uint8_t apu_read_status(void)
{
   return status;
}



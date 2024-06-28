#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>

// https://www.nesdev.org/wiki/2A03

typedef struct Pulse_t
{
   uint8_t volume; // duty cycle and volume
   uint8_t sweep;  // sweep control register
   uint8_t lo;     // low byte of period
   uint8_t hi;     // high byte of period and length counter
} Pulse_t;

typedef struct Triangle_t
{
   uint8_t linear_counter;
   uint8_t lo; // lo byte of period
   uint8_t hi; // high byte of period and length counter
} Triangle_t;

typedef struct Noise_t
{
   uint8_t volume;
   uint8_t lo; // period and waveform shade
   uint8_t hi; // length counter
} Noise_t;

typedef struct DMC_t
{
   uint8_t frequency; // irq flag, loop flag, and frequency
   uint8_t raw;       // 7 bit dac
   uint8_t start;     // sample starting address
   uint8_t length;    // smaple length 
} DMC_t;

/**
 * Initialze audio device.
 * @returns false on fail, otherwise return true.
 */
bool apu_init(void);

/**
 * Write to a apu register.
 * @param position address of the apu registers to write to
 * @param data the content that is being written to the apu regsiter
 */
void apu_write(uint16_t position, uint8_t data);

/**
 * Only the status register can be read, the rest return the open bus value.
 * @returns the value of the status register
 */
uint8_t apu_read_status(void);


#endif

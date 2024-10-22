#ifndef APU_H
#define APU_H

#include <stdint.h>
#include <stdbool.h>

// https://www.nesdev.org/wiki/2A03

typedef struct Framecounter_t
{
   uint8_t sequencer_mode; // 0: 4-step mode, 1: 5-step mode
   uint8_t IRQ_inhibit;    // 0: IRQ enabled, 1: IRQ disabled
} Framecounter_t;

typedef struct Pulse_t
{
   bool     enable;          // on/off toggle of channel
   uint8_t  sequence;        // duty cycle sequence
   uint8_t  sequence_reload; // reload value of sequence
   uint16_t timer;           // 11 bit timer value
   uint16_t timer_reload;    // reload value of 11 bit timer
   uint8_t  length_counter;  // length counter loaded from lookup table, silences channel when value decrements to zero
   uint8_t  volume;
   uint8_t  raw_sample;
} Pulse_t;

/**
 * Initialze audio device.
 * @returns false on fail, otherwise return true.
 */
bool apu_init(void);

/**
 * Closes the audio device.
 */
void apu_shutdown(void);

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

/**
 * Return output sample after mixing the 5 sound channels.
 */
uint8_t apu_get_output_sample(void);

void apu_tick(void);


#endif

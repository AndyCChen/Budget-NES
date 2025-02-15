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
   bool     channel_enable;         // on/off toggle of channel
   uint8_t  sequence;               // duty cycle sequence
   uint8_t  sequence_reload;        // reload value of sequence
   int      timer;                  // 11 bit timer value
   uint16_t timer_reload;           // reload value of 11 bit timer
   bool     length_counter_halt;    // 1: length counter frozen at current value, 0: length counter decreases to zero
   uint8_t  length_counter;         // length counter loaded from lookup table, silences channel when value decrements to zero
   uint8_t  volume;                 // set volume
   bool     constant_volume_enable; // 1: constant volume is used, 0: envelope volume is used starting at volume 15 and decreasing to 0 overtime
   uint8_t  envelope_volume;        // envelope volume that can be decreased overtime
	uint8_t  envelope_counter;
   bool     envelope_reset;         // Restart envelope if true
   bool     sweep_enable;           // enable sweep unit
   bool     sweep_negate;       
   uint8_t  sweep_shift;            // shift amount
   uint8_t  sweep_counter;          // sweep unit counter/divider counter
   uint8_t  sweep_reload;           // reload value of sweep counter
   bool     sweep_reset;            // reload the sweep counter with sweep reload
   uint8_t  raw_sample;
	uint8_t  raw_samples[41];
	size_t   raw_sample_index;
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
int16_t apu_get_output_sample(void);

void apu_tick(void);

/// <summary>
/// Pauses the audio playback. Video output is synced to the apu so pausing
/// the apu will also pause the NES itself.
/// </summary>
/// <param name="flag">True := Pause, False := Unpause</param>
void apu_pause(bool flag);


#endif

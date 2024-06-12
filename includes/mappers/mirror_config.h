#ifndef MIRROR_CONFIG_H
#define MIRROR_CONFIG_H

#include <stdint.h>

/**
 * Vertical mirroring configuration of nametables.
 * @param position address to mirror
*/
void mirror_config_vertical(uint16_t* position);

/**
 * Horizontal mirroring configuration of nametables.
 * @param position address to mirror
*/
void mirror_config_horizontal(uint16_t* position);

/**
 * Single screen mirroring
 * @param position address to mirror
 * @param bank_number bank of vram to use
*/
void mirror_config_single_screen(uint16_t* position,  uint16_t bank_number);

#endif

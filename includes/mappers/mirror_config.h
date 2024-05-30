#ifndef MIRROR_CONFIG_H
#define MIRROR_CONFIG_H

#include <stdint.h>

/**
 * Vertical mirroring configuration of nametables.
*/
void mirror_config_vertical(uint16_t* position);

/**
 * Horizontal mirroring configuration of nametables.
*/
void mirror_config_horizontal(uint16_t* position);

#endif

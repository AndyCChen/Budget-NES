#ifndef UTIL_H
#define UTIL_H

/**
 * Clears the bit at the specified position of target.
 * @param target target value to set bit on
 * @param position is the bit to clear
*/
#define clear_bit(target, position) target &= ~( 1 << position )

/**
 * Sets the bit at the specified position of target.
 * @param target target value to set bit on
 * @param position is the bit to set
*/
#define set_bit(target, position) target |= ( 1 << position )

/**
 * Stores value of 1 bit (on/off) at the specified positon of target.
 * @param target target value to set bit on
 * @param bit value of 1 or 0 of the bit to store into target
 * @param position is the bit to set
*/
#define store_bit(target, bit, position) target = ( target & ~( 1 << position ) ) | ( bit << position )

#endif

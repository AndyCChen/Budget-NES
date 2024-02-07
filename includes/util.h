#ifndef UTIL_H
#define UTIL_H

/**
 * @param value target value to set bit on
 * @param position is the bit to clear
*/
#define clear_bit(value, position) value &= ~(1 << position)

/**
 * @param value target value to set bit on
 * @param position is the bit to set
*/
#define set_bit(value, position) value |= (1 << position)

#endif
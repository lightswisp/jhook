/* +----------------------------------------------------------------------+ 
 * |  Copyright (C) 2025 lightswisp                                       |
 * |                                                                      |
 * |  Everyone is permitted to copy and distribute verbatim or modified   |
 * |  copies of this license document, and changing it is allowed as long |
 * |  as the name is changed.                                             |
 * |                                                                      |
 * |         DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE                  |
 * |  TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION     |
 * |                                                                      |
 * |  0. You just DO WHAT THE FUCK YOU WANT TO.                           |
 * +----------------------------------------------------------------------+
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "sigscan.h"

static bool valid(uint8_t *mem, uint8_t* signature, uint8_t *mask, size_t size){
  while(size --> 0)
    if(mem[size] != signature[size] && mask[size] == 'x') return false;
  return true;
}

loc_t sigscan(uint8_t *start, uint8_t *end, uint8_t *signature, size_t signature_size, uint8_t *mask, size_t mask_size){
  if(mask_size != signature_size) return NULL;
  while(start <= end){
    if(valid(start, signature, mask, signature_size)) return start;
    start++;
  } 
  return NULL;
}

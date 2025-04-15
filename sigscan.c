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

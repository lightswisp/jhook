#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>

#include "sigscan.h"

loc_t sigscan(char *start, char *end, char *signature, size_t signature_size, char *mask, size_t mask_size){
  if(mask_size != signature_size)
    return NULL;

  size_t sig_i = 0;

  while(start <= end){
    if(*start == signature[sig_i] || mask[sig_i] == '?'){
      sig_i++; 
      if(sig_i == (signature_size - 1))
        return (start - signature_size + 2); 
    }
    else 
      sig_i = 0;

    start++;
  } 

  return NULL;
}

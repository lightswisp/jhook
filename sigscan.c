#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "sigscan.h"

loc_t sigscan(char *memory, ssize_t memory_size, char* signature, ssize_t signature_size, char *mask, ssize_t mask_size){
  loc_t l = -1;

  if(mask_size != signature_size)
    return l;

  ssize_t sig_i = 0;
  for(ssize_t i = 0; i < memory_size; i++){
    if(memory[i] == signature[sig_i] || mask[sig_i] == '?'){
      sig_i++; 
      if(sig_i == (signature_size - 1)){
        l = (i - signature_size + 2); 
        break;
      }
    }
    else 
      sig_i = 0;
  } 
  return l;
}

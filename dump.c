#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

void dump(uint8_t *address, size_t size){
  for(size_t i = 0; i < size; i++){
    if(i % 8 == 0)
      putchar('\n'); 
    printf("%02x ", address[i]);
  }
}

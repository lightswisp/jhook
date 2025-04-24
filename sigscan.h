#pragma once 
#define _GNU_SOURCE
#include <link.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_SIG_LEN 512
#define MAX_MASK_LEN MAX_SIG_LEN

typedef uint8_t* loc_t;
typedef struct {
  uint8_t signature[MAX_SIG_LEN]; 
  size_t  signature_size;
  uint8_t mask[MAX_MASK_LEN]; 
  size_t  mask_size;
  uint8_t *found_at;
} signature_t;

#define DEFINE_SIGNATURE(x, signature, signature_size, mask, mask_size, found_at) \
  signature_t g_signature_##x = { signature, signature_size, mask, mask_size, found_at }

#define OBTAIN_SIGNATURE(x) g_signature_##x

loc_t sigscan(uint8_t *start, uint8_t *end, uint8_t *signature, size_t signature_size, uint8_t *mask, size_t mask_size);

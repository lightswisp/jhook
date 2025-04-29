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

#define DEFINE_SIGNATURE(x, signature, mask, found_at) \
  signature_t g_signature_##x = { signature, sizeof(signature), mask, sizeof(mask), found_at }

#define OBTAIN_SIGNATURE(x) g_signature_##x

loc_t sigscan(uint8_t *start, uint8_t *end, uint8_t *signature, size_t signature_size, uint8_t *mask, size_t mask_size);

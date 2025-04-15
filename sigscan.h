#pragma once 
#define _GNU_SOURCE
#include <link.h>
#include <stdint.h>
#include <sys/types.h>

typedef uint8_t* loc_t;
loc_t sigscan(uint8_t *start, uint8_t *end, uint8_t *signature, size_t signature_size, uint8_t *mask, size_t mask_size);

#pragma once 
#define _GNU_SOURCE
#include <link.h>
#include <stdint.h>
#include <sys/types.h>

typedef char* loc_t;
loc_t sigscan(char *start, char *end, char *signature, size_t signature_size, char *mask, size_t mask_size);

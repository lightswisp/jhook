#pragma once 
#include <stdint.h>
#include <sys/types.h>

typedef int64_t loc_t;
extern loc_t sigscan(char *memory, ssize_t memory_size, char* signature, ssize_t signature_size, char *mask, ssize_t mask_size);

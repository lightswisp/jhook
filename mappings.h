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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <linux/limits.h>

#include "sigscan.h"

#define MAX_READ_SIZE 4096 

typedef struct {
  char *buffer;
  size_t size;
} mapping_text_t;

typedef struct mapping_entry_t{
  void *start; 
  void *end;
  char name[PATH_MAX];
  struct mapping_entry_t *next;
} mapping_entry_t;

typedef struct {
  size_t size;
  mapping_entry_t *entry;
} mapping_parsed_t;

extern FILE*             mappings_open(void);
extern mapping_parsed_t* mappings_parse(FILE *mappings);
extern void              mappings_free(mapping_parsed_t *parsed);
extern void              mappings_print(mapping_parsed_t *parsed);
extern void              mappings_iterate(mapping_parsed_t *parsed, bool(*filter)(const char*), bool(*callback)(mapping_entry_t *parsed, signature_t* signature), signature_t *signature);
extern void*             mapping_find_base(mapping_parsed_t *parsed, bool(*filter)(const char*));

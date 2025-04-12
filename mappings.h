#pragma once
#include <stdio.h>
#include <stdint.h>
#include <linux/limits.h>

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

extern FILE*            mappings_open(void);
extern mapping_text_t   mappings_read(FILE *mappings);
extern void             mappings_set_addr(char *addr, mapping_entry_t* entry);
extern mapping_entry_t* mappings_parse_entry(char *text, size_t text_size);
extern mapping_parsed_t mappings_parse(FILE *mappings);
extern void             mappings_free(mapping_parsed_t *parsed);
extern void             mappings_print(mapping_parsed_t *parsed);

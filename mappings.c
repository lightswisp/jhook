#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <linux/limits.h>

#include "mappings.h"

FILE* mappings_open(void){
  FILE *mappings = fopen("/proc/self/maps", "r");
  assert(mappings != NULL && "fopen failed");
  return mappings;
}

mapping_text_t mappings_read(FILE *mappings){
  mapping_text_t text = {0};
  size_t buf_size = MAX_READ_SIZE; 

  char *buf = NULL;
  char temp_buf[MAX_READ_SIZE];
  size_t read_bytes = 0;

  buf = malloc(MAX_READ_SIZE);
  while((read_bytes = fread(temp_buf, sizeof(char), MAX_READ_SIZE, mappings)) > 0){
    buf_size += read_bytes; 
    strncat(buf, temp_buf,  read_bytes);
    buf = realloc(buf, buf_size);
  }
  text.size = buf_size - MAX_READ_SIZE;
  text.buffer = buf;

  return text;
}

void mappings_set_addr(char *addr, mapping_entry_t* entry){
  char *pch = NULL;
  pch = strsep(&addr, "-");
  void* start = (void*)strtoul(pch, NULL, 16);
  pch = strsep(&addr, "-");
  void* end   = (void*)strtoul(pch, NULL, 16);

  entry->start = start;
  entry->end   = end;
}

mapping_entry_t *mappings_parse_entry(char *text, size_t text_size){
  /* 
   * The format of the file is:
   *
   *   address           perms offset  dev   inode       pathname
   *   00400000-00452000 r-xp 00000000 08:02 173521      /usr/bin/dbus-daemon
   *   00651000-00652000 r--p 00051000 08:02 173521      /usr/bin/dbus-daemon
   *   00652000-00655000 rw-p 00052000 08:02 173521      /usr/bin/dbus-daemon
   *   00e03000-00e24000 rw-p 00000000 00:00 0           [heap]
   *   00e24000-011f7000 rw-p 00000000 00:00 0           [heap]
   *   ...
   *   35b1800000-35b1820000 r-xp 00000000 08:02 135522  /usr/lib64/ld-2.15.so
   *   35b1a1f000-35b1a20000 r--p 0001f000 08:02 135522  /usr/lib64/ld-2.15.so
   *   35b1a20000-35b1a21000 rw-p 00020000 08:02 135522  /usr/lib64/ld-2.15.so
   *   35b1a21000-35b1a22000 rw-p 00000000 00:00 0
   *   35b1c00000-35b1dac000 r-xp 00000000 08:02 135870  /usr/lib64/libc-2.15.so
   *   35b1dac000-35b1fac000 ---p 001ac000 08:02 135870  /usr/lib64/libc-2.15.so
   *   35b1fac000-35b1fb0000 r--p 001ac000 08:02 135870  /usr/lib64/libc-2.15.so
   *   35b1fb0000-35b1fb2000 rw-p 001b0000 08:02 135870  /usr/lib64/libc-2.15.so
   *   ...
   *   f2c6ff8c000-7f2c7078c000 rw-p 00000000 00:00 0    [stack:986]
   *   ...
   *   7fffb2c0d000-7fffb2c2e000 rw-p 00000000 00:00 0   [stack]
   *   7fffb2d48000-7fffb2d49000 r-xp 00000000 00:00 0   [vdso]

   *   The address field is the address space in the process that
   *   the mapping occupies.  The perms field is a set of
   *   permissions:

   *   r = read
   *   w = write
   *   x = execute
   *   s = shared
   *   p = private (copy on write)
   */
  mapping_entry_t *entry = (mapping_entry_t*)malloc(sizeof(mapping_entry_t));

  char *pch  = NULL;
  char *pch2 = NULL;
  int i = 0;
  while ((pch = strsep(&text, " "))){
    if(*pch == '\0') continue;
    switch(i){
      case 0: 
        /* addr */
        pch2 = strdup(pch);
        mappings_set_addr(pch2, entry);
        free(pch2);
        break;
      case 1: /* permissions */ break;
      case 2: /* offset      */ break;
      case 3: /* dev         */ break;
      case 4: /* inode       */ break;
      case 5:
        /* path */
        memcpy(entry->name, pch, strlen(pch));
        break;
    }
    i++;
  }
  return entry;
}

mapping_parsed_t mappings_parse(FILE *mappings){
  mapping_text_t text = mappings_read(mappings);
  mapping_parsed_t parsed = {0};
  mapping_entry_t *current_entry = NULL;
  char *pch = NULL;
  pch = strtok(text.buffer, "\n");
  while(pch != NULL){
    parsed.size++;
    if(current_entry == NULL){
      current_entry = mappings_parse_entry(pch, strlen(pch));
      parsed.entry = current_entry;
    }
    else{
      current_entry->next = mappings_parse_entry(pch, strlen(pch));
      current_entry = current_entry->next;
    }
    pch = strtok(NULL, "\n");
  }
  free(text.buffer);
  return parsed;
}

void mappings_free(mapping_parsed_t *parsed){
  mapping_entry_t *current_entry = parsed->entry;
  mapping_entry_t *to_free = NULL;
  for(size_t i = 0; i < parsed->size; i++){
    to_free = current_entry;
    current_entry = current_entry->next;
    free(to_free);
  }
}

void mappings_print(mapping_parsed_t *parsed){
  printf("[entries]: %lu\n", parsed->size);
  mapping_entry_t *current_entry = parsed->entry;
  for(size_t i = 0; i < parsed->size; i++){
    printf("[%p - %p] %s\n", current_entry->start, current_entry->end, current_entry->name);
    current_entry = current_entry->next;
  }
}


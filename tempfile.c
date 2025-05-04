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
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>

#include "logger.h"
#include "hooks.h"
#include "helpers.h"

#define PATH      "/tmp/"
#define BASE_NAME "abobaXXXXXX"
#define SUFFIX    ".java"
#define TEMPLATE PATH BASE_NAME SUFFIX

static int  g_tempfile_fd = -1;
static char g_tempfile_class_name[12];
static char g_tempfile_path[] = TEMPLATE;

int strpos(const char *heystack, const char *needle){
  char *found_at;
  if((found_at = strstr(heystack, needle)) != NULL){
    return found_at - heystack;
  }
  return -1;
}

bool tempfile_create(void){
  g_tempfile_fd = mkstemps(g_tempfile_path, sizeof(SUFFIX) - 1);
  if(g_tempfile_fd == -1){
    logger_fatal(__func__, "failed to create temp file name"); 
    return false;
  }

  char *bname = basename(g_tempfile_path);
  size_t limit = strpos(bname, SUFFIX);
  strncpy(g_tempfile_class_name, bname, limit);
  logger_log(__func__, "temp file created @ %s", g_tempfile_path);
  logger_log(__func__, "class_name is %s", g_tempfile_class_name);

  return true;
}

char *tempfile_get_path(void){
  return g_tempfile_path;
}

char *tempfile_get_class_name(void){
  return g_tempfile_class_name;
}

void tempfile_remove(void){
  if(g_tempfile_fd == -1) return;
  if(remove(g_tempfile_path) == -1){
    logger_fatal(__func__, "failed to remove temp file @ %s", g_tempfile_path);
    return;
  }
  logger_log(__func__, "removed temp file @ %s", g_tempfile_path); 
}

bool tempfile_generate_java_code(jvmtiEnv *jvmti, hook_t *hooks, size_t size){
  /* 
   * TODO:
   * add automatic name demangler for methods that are contained inside hooks struct
   */

  /* compute the size */
  char *source_code;
  size_t file_size = 0;
  file_size += sizeof("public class ");
  file_size += sizeof(BASE_NAME);
  file_size += sizeof(" {\n");
  for(size_t i = 0; i < size; i++)
    file_size += strlen(hooks[i].method_java_name) + sizeof('\n');

  file_size += sizeof("}\n");
  SAFE_MALLOC(source_code, file_size, false);

  /* append to the source code */
  sprintf(source_code, "public class %s {\n", g_tempfile_class_name);
  for(size_t i = 0; i < size; i++){
    logger_log(__func__, "generating method for %s", hooks[i].method_name);
    strcat(source_code, hooks[i].method_java_name);
    strcat(source_code, "\n");
  }
  strcat(source_code, "}\n");

  /* now write it to the file */
  if(write(g_tempfile_fd, source_code, strlen(source_code)) == -1){
    logger_fatal(__func__, "failed to write to the temp file");
    return false;
  } 
  free(source_code);
  return true;
}

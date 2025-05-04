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
#include <jni.h>
#include <jvmti.h>
#include <stdint.h>
#include <stdbool.h>

#include "defines.h"

typedef struct {
  char class_name[4096];
  char method_name[4096];
  char method_sig[4096];
  char method_java_name[4096];
  bool method_is_static;
  jmethodID method_id_orig;
  jmethodID method_id_hook;
  JNINativeMethod native_detour[1];
} hook_t;

extern void             set_hook(jmethodID mid, uint64_t *hook_addr, uint64_t **orig_i2i, uint64_t **orig_fi);
extern void             remove_hook(jmethodID mid, uint64_t *orig_i2i, uint64_t *orig_fi);
extern bool             init_libjvm(void);
extern __Method         method_create(__Method original_method, const char *method_name, const char *method_sig);
extern __GrowableArray  array_create(int cap);
extern void             array_push(__GrowableArray array, __Method *method);
extern __InstanceKlass  klass_get(__Method method);
extern void             method_merge(__GrowableArray array, __InstanceKlass klass);
extern unsigned char*   method_interpreter_get(__Method method);
extern void             method_set_native(__Method method, unsigned char *entry);

#define HOOK_INIT(x) \
  static bool           g_initialized_##x      = false; \
  static jclass         g_clazz_##x            = NULL;  \
  static jmethodID      g_target_mid_##x       = NULL;  \
  static uint64_t      *g_orig_i2i_entry_##x   = NULL;  \
  static uint64_t      *g_orig_fi_entry_##x    = NULL;  \
  static uint64_t      *g_hook_method_##x      = NULL;  \
  static unsigned char *g_hook_interpreter_##x = NULL;  \

#define PUSH(reg) __asm__ volatile("push %"reg)
#define POP(reg) __asm__ volatile("pop %"reg)

#define SAVE_CTX() do{\
  PUSH("rax"); \
  PUSH("rcx"); \
  PUSH("rdx"); \
  PUSH("rbx"); \
  PUSH("rsi"); \
  PUSH("rdi"); \
  PUSH("r8");  \
  PUSH("r9");  \
  PUSH("r10"); \
  PUSH("r11"); \
  PUSH("r12"); \
  PUSH("r13"); \
  PUSH("r14"); \
  PUSH("r15"); \
}while(0)

#define RESTORE_CTX() do{\
    POP("r15"); \
    POP("r14"); \
    POP("r13"); \
    POP("r12"); \
    POP("r11"); \
    POP("r10"); \
    POP("r9");  \
    POP("r8");  \
    POP("rdi"); \
    POP("rsi"); \
    POP("rbx"); \
    POP("rdx"); \
    POP("rcx"); \
    POP("rax"); \
    POP("rbp"); \
}while(0)

#define HOOK_ENTRY(x) \
  __attribute__((noreturn)) void hook_entry_##x(){\
  /* switching method */                          \
  __asm__ volatile(                               \
      "mov %[hook_method], %%rbx"                 \
      :: [hook_method] "m" (g_hook_method_##x)    \
  );                                              \
  POP("rbp");                                     \
  /* jumping to the native interpreter */         \
  __asm__ volatile(                               \
    "jmpq *%[addr]"                               \
    :: [addr] "m" (g_hook_interpreter_##x)        \
  );                                              \
  while(1);                                       \
}

#define REMOVE_HOOK(x) do{                                                    \
  if(g_initialized_##x)                                                       \
    remove_hook(g_target_mid_##x, g_orig_i2i_entry_##x, g_orig_fi_entry_##x); \
} while(0)

#define _SET_HOOK(x) do{ \
  set_hook(g_target_mid_##x, (uint64_t*)hook_entry_##x, &g_orig_i2i_entry_##x, &g_orig_fi_entry_##x); \
} while(0)

#define SET_HOOK(x, orig, hook) do{                                   \
  g_initialized_##x = true;                                           \
  g_target_mid_##x  = orig;                                           \
  g_hook_method_##x = resolve_jmethod_id(hook);                       \
  g_hook_interpreter_##x = method_interpreter_get(g_hook_method_##x); \
  _SET_HOOK(x);                                                       \
} while(0)

#define GET_HOOK_NAME_BY_IDX(x) (g_target_mid_##x)
#define GET_CLASS_NAME_BY_IDX(x) (g_clazz_##x)

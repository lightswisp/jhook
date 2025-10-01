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
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <jvmti.h>
#include <jni.h>

/* typedefs */
#define ARR_LENGTH(x)            (sizeof(x)/sizeof(x[0]))
#define HOOK_CHAR_BUFF_LIMIT     256
#define CLASS_DEPENDENCIES_LIMIT 5
#define PARAMS_BUFF_LIMIT        2048
#define RETURN_TYPE_BUFF_LIMIT   2048

typedef uintptr_t* __Method;

typedef enum {
  JAVA_UNKNOWN = 0,
  JAVA_BOOL,
  JAVA_BYTE,
  JAVA_CHAR,
  JAVA_SHORT,
  JAVA_INT,
  JAVA_LONG,
  JAVA_FLOAT,
  JAVA_DOUBLE,
  JAVA_VOID,
  JAVA_OBJECT,
  JAVA_ARRAY
} datatype_t;

typedef struct {
  char p[PARAMS_BUFF_LIMIT];
  char r[RETURN_TYPE_BUFF_LIMIT];
} java_descriptor_t;

typedef struct {
  size_t size;
  char name[CLASS_DEPENDENCIES_LIMIT][HOOK_CHAR_BUFF_LIMIT];
} deps_t;

typedef struct {
  char class_name      [HOOK_CHAR_BUFF_LIMIT];
  char method_name     [HOOK_CHAR_BUFF_LIMIT];
  char method_sig      [HOOK_CHAR_BUFF_LIMIT];
  bool method_is_static;
  jmethodID method_id_orig;
  jmethodID method_id_hook;
  JNINativeMethod native_detour[1];
  deps_t dependencies;
} hook_t;

typedef struct {
  bool initialized; 
  jclass clazz;
  jmethodID target_mid;
  uintptr_t *orig_i2i_entry;
  uintptr_t *orig_fi_entry;
  uintptr_t *orig_fc_entry;
  uintptr_t *hook_method;
  uintptr_t *hook_interpreter;
  uintptr_t *hook_compiled;
} hook_info_t;

#define JHOOK extern

/* tempfile */
#define PATH      "/tmp/"
#define BASE_NAME "abobaXXXXXX"
#define SUFFIX    ".java"
#define TEMPLATE PATH BASE_NAME SUFFIX

/* logger */
#define LOG_MAX_BUFFER_SIZE      1024
#define LOG_MAX_TIME_BUFFER_SIZE 128
#define COLOR_START(x) "\033["x"m"
#define COLOR_END      "\033[0m"
#define RED            "31"
#define GREEN          "32" 
#define YELLOW         "33"

/* hooks */
#define HOOK_INIT(x) \
  static hook_info_t hook_info_##x = { \
    false,                             \
    NULL,                              \
    NULL,                              \
    NULL,                              \
    NULL,                              \
    NULL,                              \
    NULL,                              \
    NULL,                              \
    NULL                               \
  };                                   \

#define POP(reg) __asm__ volatile("pop %"reg)

#define HOOK_ENTRY(x) \
  __attribute__((noreturn)) void hook_entry_##x(){     \
  /* switching method */                               \
  __asm__ volatile(                                    \
      "mov %[hook_method], %%rbx"                      \
      :: [hook_method] "m" (hook_info_##x.hook_method) \
  );                                                   \
  POP("rbp");                                          \
  /* jumping to the native interpreter */              \
  __asm__ volatile(                                    \
    "jmpq *%[addr]"                                    \
    :: [addr] "m" (hook_info_##x.hook_interpreter)     \
  );                                                   \
  while(1);                                            \
}

#define REMOVE_HOOK(x) do{              \
  if(hook_info_##x.initialized)         \
    jhook_remove_hook(                  \
        hook_info_##x.target_mid,       \
        hook_info_##x.orig_i2i_entry,   \
        hook_info_##x.orig_fi_entry,    \
        hook_info_##x.orig_fc_entry     \
    );                                  \
} while(0)

#define _SET_HOOK(x) do{             \
  jhook_set_hook(                    \
      hook_info_##x.target_mid,      \
      (uintptr_t*)hook_entry_##x,    \
      hook_info_##x.hook_compiled,   \
      &hook_info_##x.orig_i2i_entry, \
      &hook_info_##x.orig_fi_entry,  \
      &hook_info_##x.orig_fc_entry   \
  );                                 \
} while(0)

#define SET_HOOK(x, hook) do{                                                               \
  hook_info_##x.initialized      = true;                                                    \
  hook_info_##x.target_mid       = hook.method_id_orig;                                     \
  hook_info_##x.hook_method      = jhook_resolve_jmethod_id(hook.method_id_hook);           \
  hook_info_##x.hook_interpreter = jhook_method_interpreter_get(hook_info_##x.hook_method); \
  hook_info_##x.hook_compiled    = jhook_method_from_compiled(hook_info_##x.hook_method);   \
  _SET_HOOK(x);                                                                             \
} while(0)

#define CALL_ORIGINAL(x, code) do{  \
  REMOVE_HOOK(x);                   \
  code;                             \
  _SET_HOOK(x);                     \
} while(0)

#define GET_ORIG_METHOD(x)  (hook_info_##x.target_mid)
#define GET_ORIG_CLASS (x)  (hook_info_##x.clazz)

#define SAFE_RETURN(x, type, code) do{                             \
  code                                                             \
  if(!x){                                                          \
    jhook_logger_fatal(__func__, "failed to get " type " for"#x);  \
    return NULL;                                                   \
  }                                                                \
}while(0)

/* offsets */ 
#define fc_entry_off    0x0000000b
#define i2i_entry_off   0x0000000a
#define fi_entry_off    0x0000000d
#define flags_entry_off 0x00000007

/* functions */

JHOOK bool         jhook_init(int jvmti_version, JavaVM **jvm, JNIEnv **env, jvmtiEnv **jvmti);
JHOOK bool         jhook_register(hook_t *hooks, size_t size, JNIEnv *env, jvmtiEnv *jvmti);
JHOOK void         jhook_logger_log  (const char *func, const char *fmt, ...);
JHOOK void         jhook_logger_warn (const char *func, const char *fmt, ...);
JHOOK void         jhook_logger_fatal(const char *func, const char *fmt, ...);
JHOOK void         jhook_set_hook(jmethodID mid, uintptr_t *hook_addr_interpreted, uintptr_t *hook_addr_compiled, uintptr_t **orig_i2i, uintptr_t **orig_fi, uintptr_t **orig_fc);
JHOOK void         jhook_remove_hook(jmethodID mid, uintptr_t *orig_i2i, uintptr_t *orig_fi, uintptr_t *orig_fc);
JHOOK __Method     jhook_resolve_jmethod_id(jmethodID mid);
JHOOK bool         jhook_resolve_class_dependencies(JNIEnv *env, hook_t *hooks, size_t size);
JHOOK bool         jhook_attach_current_thread(JavaVM *vm, void **penv, void *args);
JHOOK bool         jhook_get_java_vms(JavaVM **vm);
JHOOK bool         jhook_get_jvmti(JavaVM *vm, jvmtiEnv **jvmti, int jvmti_version);
JHOOK char*        jhook_get_class_path(JNIEnv *env, jclass clazz, const char *class_name);
JHOOK void         jhook_set_original_noinline_flags(JNIEnv *env, jvmtiEnv *jvmti, hook_t *hooks, size_t size);
JHOOK bool         jhook_suspend_all_threads(JNIEnv *env, jvmtiEnv *jvmti);
JHOOK bool         jhook_resume_all_threads(JNIEnv *env, jvmtiEnv *jvmti);
JHOOK jclass       jhook_create_class(JNIEnv *env, const char *class_name, const char *src_path);
JHOOK jclass       jhook_find_class(JNIEnv *env, const char *class_name);
JHOOK jmethodID    jhook_find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static);
JHOOK jmethodID    jhook_find_method2(JNIEnv *env, jclass clazz, const char* method_name, const char *method_signature, bool is_static);
JHOOK bool         jhook_resolve_original_methods(JNIEnv *env, jvmtiEnv *jvmti, hook_t *hooks, size_t size);
JHOOK bool         jhook_resolve_hook_methods(JNIEnv *env, jclass clazz, hook_t *hooks, size_t size);
JHOOK bool         jhook_register_hook_methods(JNIEnv *env, jclass clazz, hook_t *hooks, size_t size);
JHOOK int          jhook_strpos(const char *heystack, const char *needle);
JHOOK bool         jhook_tempfile_create(void);
JHOOK void         jhook_tempfile_remove(void);
JHOOK char*        jhook_tempfile_get_path(void);
JHOOK char*        jhook_tempfile_get_class_name(void);
JHOOK datatype_t   jhook_demangler_get_type(char t);
JHOOK char*        jhook_demangler_demangle(char *p, char *r, char *f, bool is_static);
JHOOK bool         jhook_tempfile_generate_java_code(hook_t *hooks, size_t size);
JHOOK uintptr_t*   jhook_method_interpreter_get(__Method method);
JHOOK uintptr_t*   jhook_method_from_compiled(__Method method);

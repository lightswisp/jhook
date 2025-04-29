#include <jni.h>
#include <jvmti.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"
#include "offsets.h"
#include "defines.h"
#include "logger.h"
#include "mappings.h"
#include "sigscan.h"

#define LIBJVM "libjvm.so"
static void *g_libjvm_base  = NULL;

static uint16_t size_of_parameters(__Method this);
DEFINE_SIGNATURE(
    size_of_parameters, 
    "\x48\x8b\x00\x00\x0f\xb7\x00\x00\x3b",
    "xx??xx??x",
    NULL
);

static uint8_t* interpreter_entry_for_kind(enum __MethodKind);
DEFINE_SIGNATURE(
    interpreter_entry_for_kind, 
    "\x4C\x8D\x2D\x2A\x2A\x2A\x2A\xBE\x2A\x2A\x2A\x2A\x49\x89\x45\x2A\x48\x8B\x85\x2A\x2A\x2A\x2A\x48\x8B\x38\xE8\x2A\x2A\x2A\x2A\x48\x8B\x85", 
    "xxx????x????xxx?xxx????xxxx????xxx", 
    NULL
);

static __JavaThread(*java_thread_current)(void);
DEFINE_SIGNATURE(
    java_thread_current, 
    "\x48\x8D\x05\x2A\x2A\x2A\x2A\x8B\x00\x85\xC0\x74\x2A\x48\x8D\x05\x2A\x2A\x2A\x2A\x55\x48\x89\xE5\x8B\x38\xE8\x2A\x2A\x2A\x2A\x48\x85\xC0\x74\x2A\x48\x8B\x80\x2A\x2A\x2A\x2A\x5D\xC3", 
    "xxx????xxxxx?xxx????xxxxxxx????xxxx?xxx????xx", 
    NULL
);

/* libjvm function pointers */
static void              (*remove_unshareable_info)           (__Method this);
static __InstanceKlass   (*method_holder)                     (__Method this);
static __ClassLoaderData (*class_loader_data)                 (__InstanceKlass this);
static __Symbol          (*new_permanent_symbol)              (const char* s);
static __ConstMethod     (*const_method)                      (__Method this);
static __Method          (*method_allocate)                   (__ClassLoaderData, int, __AccessFlags, __InlineTableSizes, int, __Symbol, __JavaThread);
static void              (*growable_array_init)               (__GrowableArray, int, int);
static void              (*growable_array_push)               (__GrowableArray, __Method*);
static __ConstantPool    (*constant_pool_allocate)            (__ClassLoaderData, int, __JavaThread);
static __ConstantPool    (*instance_klass_constants)          (__InstanceKlass);
static void              (*constant_pool_copy_fields)         (__ConstantPool, __ConstantPool);
static void              (*constant_pool_set_pool_holder)     (__ConstantPool, __InstanceKlass);
static void              (*constant_pool_symbol_at_put)       (__ConstantPool, int, __Symbol);
static void              (*method_set_constants)              (__Method, __ConstantPool);
static void              (*method_set_name_index)             (__Method, int);
static void              (*method_set_signature_index)        (__Method, int);
static void              (*method_set_interpreter_entry)      (__Method, unsigned char*);
static unsigned char*    (*method_from_interpreted_entry)     (__Method);
static void              (*method_set_native_function)        (__Method, unsigned char*, bool);
static void              (*const_method_compute_from_sig)     (__ConstMethod, __Symbol, bool);
static void              (*merge_in_new_methods)              (__InstanceKlass, __GrowableArray, __JavaThread);

void set_hook(jmethodID mid, uint64_t *hook_addr, uint64_t **orig_i2i, uint64_t **orig_fi){
  // TODO: check if method is native
  __Method method     = resolve_jmethod_id(mid);
  /* in order to unhook this method 
   * we need to save these two entries somewhere
   * and then assign them back accordingly
   */
  uint64_t **i2i_entry = (uint64_t**)(method + i2i_entry_off);
  uint64_t **fi_entry  = (uint64_t**)(method + fi_entry_off);

  logger_log(__func__,"i2i @ %p", i2i_entry);
  logger_log(__func__,"fi @ %p", fi_entry);

  // save
  *orig_i2i = *i2i_entry; 
  *orig_fi  = *fi_entry;

  // rewrite
  *i2i_entry     = hook_addr;
  *fi_entry      = hook_addr;
}

void remove_hook(jmethodID mid, uint64_t *orig_i2i, uint64_t *orig_fi){
  __Method method        = resolve_jmethod_id(mid);
  uint64_t **i2i_entry = (uint64_t**)(method + i2i_entry_off);
  uint64_t **fi_entry  = (uint64_t**)(method + fi_entry_off);

  // restore
  *i2i_entry     = orig_i2i;
  *fi_entry      = orig_fi;
}

static void resolve_functions(void){

  java_thread_current = (void*)OBTAIN_SIGNATURE(java_thread_current).found_at;
  remove_unshareable_info       = g_libjvm_base + remove_unshareable_info_off;
  method_holder                 = g_libjvm_base + method_holder_off;
  class_loader_data             = g_libjvm_base + class_loader_data_off;
  new_permanent_symbol          = g_libjvm_base + new_permanent_symbol_off;
  const_method                  = g_libjvm_base + const_method_off;
  method_allocate               = g_libjvm_base + method_allocate_off;
  growable_array_init           = g_libjvm_base + growable_array_init_off;
  growable_array_push           = g_libjvm_base + growable_array_push_off;
  constant_pool_allocate        = g_libjvm_base + constant_pool_allocate_off;
  instance_klass_constants      = g_libjvm_base + instance_klass_constants_off;
  constant_pool_copy_fields     = g_libjvm_base + constant_pool_copy_fields_off;
  constant_pool_set_pool_holder = g_libjvm_base + constant_pool_set_pool_holder_off;
  constant_pool_symbol_at_put   = g_libjvm_base + constant_pool_symbol_at_put_off;
  method_set_constants          = g_libjvm_base + method_set_constants_off;
  method_set_name_index         = g_libjvm_base + method_set_name_index_off;
  method_set_signature_index    = g_libjvm_base + method_set_signature_index_off;
  method_set_interpreter_entry  = g_libjvm_base + method_set_interpreter_entry_off;
  method_from_interpreted_entry = g_libjvm_base + method_from_interpreted_entry_off;
  const_method_compute_from_sig = g_libjvm_base + const_method_compute_from_sig_off;
  merge_in_new_methods          = g_libjvm_base + merge_in_new_methods_off;
  method_set_native_function    = g_libjvm_base + method_set_native_function_off;

  logger_log(__func__,"Method::remove_unshareable_info @ %p", remove_unshareable_info);
  logger_log(__func__,"Method::method_holder @ %p", method_holder);
  logger_log(__func__,"Method::constMethod @ %p", const_method);
  logger_log(__func__,"InstanceKlass::class_loader_data @ %p", class_loader_data);                     // ok
  logger_log(__func__,"Symbol::new_permanent_symbol @ %p", new_permanent_symbol);                      // ok
  logger_log(__func__,"GrowableArray<Method*>::GrowableArray @ %p", growable_array_init);              // ok
  logger_log(__func__,"GrowableArray<Method*>::push @ %p", growable_array_push);                       // ok
  logger_log(__func__,"ConstantPool::allocate @ %p", constant_pool_allocate);                          // ok
  logger_log(__func__,"InstanceKlass::constants @ %p", instance_klass_constants);
  logger_log(__func__,"ConstantPool::copy_fields @ %p", constant_pool_copy_fields);
  logger_log(__func__,"ConstantPool::set_pool_holder @ %p", constant_pool_set_pool_holder);
  logger_log(__func__,"ConstantPool::symbol_at_put @ %p", constant_pool_symbol_at_put);
  logger_log(__func__,"Method::set_constants @ %p", method_set_constants);
  logger_log(__func__,"Method::set_name_index @ %p", method_set_name_index);
  logger_log(__func__,"Method::set_signature_index @ %p", method_set_signature_index);
  logger_log(__func__,"Method::set_interpreter_entry @ %p", method_set_interpreter_entry);
  logger_log(__func__,"Method::set_native_function @ %p", method_set_native_function);
  logger_log(__func__,"ConstMethod::compute_from_sig @ %p", const_method_compute_from_sig);
  logger_log(__func__,"AbstractInterpreter::entry_for_kind @ %p", interpreter_entry_for_kind);
  logger_log(__func__,"merge_in_new_methods @ %p", merge_in_new_methods);
}

__GrowableArray array_create(int cap){
  __GrowableArray array;
  SAFE_MALLOC(array, GrowableArray_size, NULL);
  /* mtOther -> 0x0a */
  growable_array_init(array, cap, 0x0a); 
  return array;
}

void array_push(__GrowableArray array, __Method *method){
  growable_array_push(array, method);
}

__InstanceKlass klass_get(__Method method){
  return method_holder(method);
}

unsigned char* method_interpreter_get(__Method method){
  return method_from_interpreted_entry(method); 
}

void method_merge(__GrowableArray array, __InstanceKlass klass){
  __JavaThread thread = java_thread_current();
  merge_in_new_methods(klass, array, thread);
}

void method_set_native(__Method method, unsigned char *entry){
  method_set_native_function(method, entry, true);
}

__Method method_create(__Method original_method, const char *method_name, const char *method_sig){
  uint16_t params_size = size_of_parameters(original_method);
  int max_stack = 0, method_type = 0;

  asm("int3");
  unsigned char*     entry         = interpreter_entry_for_kind(native);
  __JavaThread       thread        = java_thread_current();
  __InstanceKlass    ik            = method_holder(original_method);
  __ClassLoaderData  loader_data   = class_loader_data(ik);
  __ConstMethod      orig_cm       = const_method(original_method);
  __Symbol           name          = new_permanent_symbol(method_name);
  __Symbol           sig           = new_permanent_symbol(method_sig);
  __ConstantPool     cp            = constant_pool_allocate(loader_data, _imcp_limit, thread);
  __ConstantPool     fields        = instance_klass_constants(ik);
  /*                                 (JVM_ACC_PUBLIC | JVM_ACC_NATIVE) */
  /*                                 (     1         |      256      ) */
  __AccessFlags      flags         = (     1         |      256      );
  __InlineTableSizes sizes         = NULL;
  SAFE_MALLOC(sizes, InlineTableSizes_size, NULL);

  constant_pool_copy_fields(cp, fields);
  constant_pool_set_pool_holder(cp, ik);
  constant_pool_symbol_at_put(cp, _imcp_invoke_name, name);
  constant_pool_symbol_at_put(cp, _imcp_invoke_signature, sig);

  __Method m       = method_allocate(loader_data, 0, flags, sizes, method_type, name, thread);
  __ConstMethod cm = const_method(m);
  method_set_constants(m, cp);
  method_set_name_index(m, _imcp_invoke_name);
  method_set_signature_index(m, _imcp_invoke_signature);
  method_set_interpreter_entry(m, entry);
  const_method_compute_from_sig(cm, sig, false);

  logger_log(__func__,"current thread %p", thread);
  logger_log(__func__,"size_of_parameters %d", params_size);
  logger_log(__func__,"ConstMethod @ %p", cm);
  logger_log(__func__,"InstanceKlass @ %p", ik);
  logger_log(__func__,"ClassLoaderData @ %p", loader_data);
  logger_log(__func__,"Symbol 4 name @ %p", name);
  logger_log(__func__,"Symbol 4 sig @ %p", sig);
  logger_log(__func__,"flags 0x%08X", flags);
  logger_log(__func__, "cp @ %p", cp);
  logger_log(__func__, "fields @ %p", fields);
  logger_log(__func__,"sizes @ %p", sizes);
  return m;
}

bool libjvm_filter(const char *name){
  if(strstr(name, LIBJVM) != NULL) return true;
  return false;
}

bool libjvm_callback(mapping_entry_t *entry, signature_t *signature){
  uint8_t *addr = sigscan(entry->start, entry->end, signature->signature, signature->signature_size, signature->mask, signature->mask_size); 
  if(addr != NULL){
    signature->found_at = addr;
    return true;
  }
  return false;
}

static uint8_t* interpreter_entry_for_kind(enum __MethodKind m){
  /* 4C 8D 2D 31 66 55 00  lea r13, [rip+0x00556631] */
  /* ---------^  + 0x03                              */
  uint32_t et_off = *(uint32_t*)(OBTAIN_SIGNATURE(interpreter_entry_for_kind).found_at + 0x03);
  uint8_t **entry_table = (uint8_t**)(OBTAIN_SIGNATURE(interpreter_entry_for_kind).found_at + 0x07 + et_off);
  return entry_table[m];
}

static uint16_t size_of_parameters(__Method this){
  /* 48 8B 51 08  mov rdx, [rcx+8] */
  /* ---------^   + 0x03           */
  uint8_t cm_off = *(OBTAIN_SIGNATURE(size_of_parameters).found_at + 0x03);
  __ConstMethod const_method = *(__ConstMethod*)((uint8_t*)this + cm_off);

  /* 0F B7 72 2E  movzx esi, word ptr [rdx+2Eh] */
  /* ---------^   + 0x07                        */
  uint8_t sz_off = *(OBTAIN_SIGNATURE(size_of_parameters).found_at + 0x07);
  uint16_t _size_of_parameters = *((uint8_t*)const_method + sz_off);

  return _size_of_parameters;
}

bool init_libjvm(void){

#define FOUND_AT_NON_NULL_ASSERT(x) do{                         \
  if(OBTAIN_SIGNATURE(x).found_at == NULL){                     \
    logger_fatal(__func__, "failed to resolve "#x" signature"); \
    return false;                                               \
  }                                                             \
}while(0)

  FILE *mappings = mappings_open();
  if(mappings == NULL) return false;

  mapping_parsed_t *parsed_mappings = mappings_parse(mappings);
  if(parsed_mappings == NULL) return false;

  g_libjvm_base = mapping_find_base(parsed_mappings, libjvm_filter);

  mappings_iterate(
      parsed_mappings, 
      libjvm_filter, 
      libjvm_callback, 
      &(OBTAIN_SIGNATURE(size_of_parameters))
  );
  FOUND_AT_NON_NULL_ASSERT(size_of_parameters);

  mappings_iterate(
      parsed_mappings, 
      libjvm_filter, 
      libjvm_callback, 
      &(OBTAIN_SIGNATURE(interpreter_entry_for_kind))
  );
  FOUND_AT_NON_NULL_ASSERT(interpreter_entry_for_kind);

  mappings_iterate(
      parsed_mappings, 
      libjvm_filter, 
      libjvm_callback, 
      &(OBTAIN_SIGNATURE(java_thread_current))
  );
  FOUND_AT_NON_NULL_ASSERT(java_thread_current);

  logger_log(__func__, "size_of_parameters found at %p", OBTAIN_SIGNATURE(size_of_parameters).found_at);
  logger_log(__func__, "interpreter_entry_for_kind found at %p", OBTAIN_SIGNATURE(interpreter_entry_for_kind).found_at);
  logger_log(__func__, "java_thread_current found at %p", OBTAIN_SIGNATURE(java_thread_current).found_at);
  asm("int3");
  resolve_functions();
  mappings_free(parsed_mappings);
  return true; 
}


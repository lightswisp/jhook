#define _GNU_SOURCE
#include <link.h>
#include <jni.h>
#include <jvmti.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "helpers.h"
#include "offsets.h"
#include "defines.h"
#include "logger.h"

#define LIBJVM "libjvm.so"
/* libjvm lib globals */
static bool     libjvm_found;
static void*    libjvm_base;
/* libjvm function pointers */
static __JavaThread      (*java_thread_current)               (void);
static void              (*remove_unshareable_info)           (__Method this);
static uint16_t          (*size_of_parameters)                (__Method this);
static __InstanceKlass   (*method_holder)                     (__Method this);
static __ClassLoaderData (*class_loader_data)                 (__InstanceKlass this);
static __Symbol          (*new_permanent_symbol)              (const char* s);
static __AccessFlags     (*access_flags_from)                 (uint16_t flags);
static __ConstMethod     (*const_method)                      (__Method this);
static __Method          (*method_allocate)                   (__ClassLoaderData, int, __AccessFlags, __InlineTableSizes, int, __Symbol, __JavaThread);
static void              (*inline_table_sizes)                (__InlineTableSizes, int, int, int, int, int, int, int, int, int, int, int);
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
static unsigned char*    (*interpreter_entry_for_kind)        (enum __MethodKind);
static void              (*merge_in_new_methods)              (__InstanceKlass, __GrowableArray, __JavaThread);
static int               (*checked_exception_length)          (__ConstMethod);
static int               (*localvariable_table_length)        (__ConstMethod);
static int               (*exception_table_length)            (__ConstMethod);
static int               (*method_parameters_length)          (__ConstMethod);
static int               (*method_annotations_length)         (__ConstMethod);
static int               (*parameter_annotations_length)      (__ConstMethod);
static int               (*type_annotations_length)           (__ConstMethod);
static int               (*default_annotations_length)        (__ConstMethod);
static int               (*generic_signature_index)           (__ConstMethod);

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

static int callback(struct dl_phdr_info *info, size_t size, void *module_name){
  int i;
  if(libjvm_found) return 0;

  if(strstr(info->dlpi_name, (const char*)module_name) != NULL){
    logger_log(__func__,"name=%s (%d segments) (%p base)", info->dlpi_name, info->dlpi_phnum, (void*)info->dlpi_addr);
    libjvm_found = true; 
    libjvm_base  = (void*)info->dlpi_addr;
  }
  return 0;
}

static void resolve_functions(void){
  remove_unshareable_info       = libjvm_base + remove_unshareable_info_off;
  size_of_parameters            = libjvm_base + size_of_parameters_off;
  method_holder                 = libjvm_base + method_holder_off;
  class_loader_data             = libjvm_base + class_loader_data_off;
  new_permanent_symbol          = libjvm_base + new_permanent_symbol_off;
  access_flags_from             = libjvm_base + access_flags_from_off;
  const_method                  = libjvm_base + const_method_off;
  inline_table_sizes            = libjvm_base + inline_table_sizes_off;
  method_allocate               = libjvm_base + method_allocate_off;
  java_thread_current           = libjvm_base + java_thread_current_off;
  growable_array_init           = libjvm_base + growable_array_init_off;
  growable_array_push           = libjvm_base + growable_array_push_off;
  constant_pool_allocate        = libjvm_base + constant_pool_allocate_off;
  instance_klass_constants      = libjvm_base + instance_klass_constants_off;
  constant_pool_copy_fields     = libjvm_base + constant_pool_copy_fields_off;
  constant_pool_set_pool_holder = libjvm_base + constant_pool_set_pool_holder_off;
  constant_pool_symbol_at_put   = libjvm_base + constant_pool_symbol_at_put_off;
  method_set_constants          = libjvm_base + method_set_constants_off;
  method_set_name_index         = libjvm_base + method_set_name_index_off;
  method_set_signature_index    = libjvm_base + method_set_signature_index_off;
  method_set_interpreter_entry  = libjvm_base + method_set_interpreter_entry_off;
  method_from_interpreted_entry = libjvm_base + method_from_interpreted_entry_off;
  const_method_compute_from_sig = libjvm_base + const_method_compute_from_sig_off;
  interpreter_entry_for_kind    = libjvm_base + interpreter_entry_for_kind_off;
  merge_in_new_methods          = libjvm_base + merge_in_new_methods_off;
  method_set_native_function    = libjvm_base + method_set_native_function_off;

  checked_exception_length      = libjvm_base + checked_exception_length_off;
  localvariable_table_length    = libjvm_base + localvariable_table_length_off;
  exception_table_length        = libjvm_base + exception_table_length_off;
  method_parameters_length      = libjvm_base + method_parameters_length_off;
  method_annotations_length     = libjvm_base + method_annotations_length_off;
  parameter_annotations_length  = libjvm_base + parameter_annotations_length_off;
  type_annotations_length       = libjvm_base + type_annotations_length_off;
  default_annotations_length    = libjvm_base + default_annotations_length_off;
  generic_signature_index       = libjvm_base + generic_signature_index_off;

  logger_log(__func__,"Method::remove_unshareable_info @ %p", remove_unshareable_info);
  logger_log(__func__,"Method::size_of_parameters @ %p", size_of_parameters);
  logger_log(__func__,"Method::method_holder @ %p", method_holder);
  logger_log(__func__,"Method::constMethod @ %p", const_method);
  logger_log(__func__,"InstanceKlass::class_loader_data @ %p", class_loader_data);
  logger_log(__func__,"Symbol::new_permanent_symbol @ %p", new_permanent_symbol);
  logger_log(__func__,"AccessFlags::accessFlags_from @ %p", access_flags_from);
  logger_log(__func__,"InlineTableSizes::InlineTableSizes @ %p", inline_table_sizes);
  logger_log(__func__,"GrowableArray<Method*>::GrowableArray @ %p", growable_array_init);
  logger_log(__func__,"GrowableArray<Method*>::push @ %p", growable_array_push);
  logger_log(__func__,"ConstantPool::allocate @ %p", constant_pool_allocate);
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
  logger_log(__func__, "ConstMethod::checked_exception_length @ %p", checked_exception_length);
  logger_log(__func__, "ConstMethod::localvariable_table_length @ %p", localvariable_table_length);
  logger_log(__func__, "ConstMethod::exception_table_length @ %p", exception_table_length);
  logger_log(__func__, "ConstMethod::method_parameters_length @ %p", method_parameters_length);
  logger_log(__func__, "ConstMethod::method_annotations_length @ %p", method_annotations_length);
  logger_log(__func__, "ConstMethod::parameter_annotations_length @ %p", parameter_annotations_length);
  logger_log(__func__, "ConstMethod::type_annotations_length @ %p", type_annotations_length);
  logger_log(__func__, "ConstMethod::default_annotations_length @ %p", default_annotations_length);
  logger_log(__func__, "ConstMethod::generic_signature_index @ %p", generic_signature_index);
}

__GrowableArray array_create(int cap){
  __GrowableArray array = malloc(GrowableArray_size);
  assert(array != NULL && "malloc failed");
  memset(array, 0, GrowableArray_size);
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
  __AccessFlags      flags         = access_flags_from(1 | 256);
  __InlineTableSizes sizes         = malloc(InlineTableSizes_size);
  assert(sizes != NULL && "malloc failed");
  memset(sizes, 0, InlineTableSizes_size); 

  int orig_checked_exception_length     = checked_exception_length(orig_cm);
  int orig_localvariable_table_length   = localvariable_table_length(orig_cm);
  int orig_exception_table_length       = exception_table_length(orig_cm);
  int orig_method_parameters_length     = method_parameters_length(orig_cm);
  int orig_method_annotations_length    = method_annotations_length(orig_cm);
  int orig_parameter_annotations_length = parameter_annotations_length(orig_cm);
  int orig_type_annotations_length      = type_annotations_length(orig_cm);
  int orig_default_annotations_length   = default_annotations_length(orig_cm);
  int orig_generic_signature_index      = generic_signature_index(orig_cm);

  // TODO 
  // clone table sizes from orig to our method
  // TODO 
  // also, we need to bypass java security checks so that we can add methods to classes that start with java
  // TODO 
  // Method::link to our new method, so that it could have adapter
  inline_table_sizes(
      sizes,
      orig_localvariable_table_length, 
      0, // compressed_linenumber_size
      orig_exception_table_length, 
      orig_checked_exception_length, 
      orig_method_parameters_length, 
      orig_generic_signature_index, 
      orig_method_annotations_length, 
      orig_parameter_annotations_length, 
      orig_type_annotations_length, 
      orig_default_annotations_length, 
      0
  );

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

bool init_hooks(void){
  dl_iterate_phdr(callback, "libjvm.so");
  if(libjvm_found){
    resolve_functions();
    return true;
  }
  return false;
}


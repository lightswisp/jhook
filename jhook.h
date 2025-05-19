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
#include <time.h>
#include <jvmti.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <linux/limits.h>

/* typedefs */
#define ARR_LENGTH(x)            (sizeof(x)/sizeof(x[0]))
#define HOOK_CHAR_BUFF_LIMIT     256
#define CLASS_DEPENDENCIES_LIMIT 5
typedef uintptr_t* __Method;

typedef struct {
  size_t size;
  char name[CLASS_DEPENDENCIES_LIMIT][HOOK_CHAR_BUFF_LIMIT];
} deps_t;

typedef struct {
  char class_name      [HOOK_CHAR_BUFF_LIMIT];
  char method_name     [HOOK_CHAR_BUFF_LIMIT];
  char method_sig      [HOOK_CHAR_BUFF_LIMIT];
  char method_java_name[HOOK_CHAR_BUFF_LIMIT];
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
#define COLOR_START(x) "\e["x"m"
#define COLOR_END      "\e[0m"
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

#define GET_HOOK_NAME_BY_IDX(x)  (hook_info_##x.target_mid)
#define GET_CLASS_NAME_BY_IDX(x) (hook_info_##x.clazz)

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

/* threads */
bool g_ath_are_suspended = false;

/* tempfile globals */
int  g_tempfile_fd             = -1;
char g_tempfile_class_name[12] = { 0 };
char g_tempfile_path[]         = TEMPLATE;

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
JHOOK bool         jhook_tempfile_generate_java_code(jvmtiEnv *jvmti, hook_t *hooks, size_t size);
JHOOK uintptr_t*    jhook_method_interpreter_get(__Method method);
JHOOK uintptr_t*    jhook_method_from_compiled(__Method method);


/* implementation */

JHOOK bool jhook_init(int jvmti_version, JavaVM **jvm, JNIEnv **env, jvmtiEnv **jvmti){
  if(!jhook_get_java_vms(jvm)) 
    return false;
  if(!jhook_attach_current_thread(*jvm, (void**)env, NULL)) 
    return false;
  if(!jhook_get_jvmti(*jvm, jvmti, jvmti_version)) 
    return false;

  return true;
}

JHOOK bool jhook_register(hook_t *hooks, size_t size, JNIEnv *env, jvmtiEnv *jvmti){
  bool status = false;
  if(!jhook_resolve_original_methods(env, jvmti, hooks, size))
    goto end;
  if(!jhook_suspend_all_threads(env, jvmti))
    goto end;
  if(!jhook_tempfile_create())
    goto end;
  if(!jhook_resolve_class_dependencies(env, hooks, size))
    goto end;
  if(!jhook_tempfile_generate_java_code(jvmti, hooks, size))
    goto end;

  jclass class_hk = jhook_create_class(env, jhook_tempfile_get_class_name(), jhook_tempfile_get_path());
  if(class_hk == NULL)
    goto end;
  if(!jhook_resolve_hook_methods(env, class_hk, hooks, size))
   goto end; 
  if(!jhook_register_hook_methods(env, class_hk, hooks, size))
    goto end;

  jhook_set_original_noinline_flags(env, jvmti, hooks, size);
  status = true;
end:  
  jhook_tempfile_remove();
  jhook_resume_all_threads(env, jvmti);
  return status;
}

JHOOK void jhook_set_hook(jmethodID mid, uintptr_t *hook_addr_interpreted, uintptr_t *hook_addr_compiled, uintptr_t **orig_i2i, uintptr_t **orig_fi, uintptr_t **orig_fc){
  // TODO: check if method is native
  __Method method     = jhook_resolve_jmethod_id(mid);
  /* in order to unhook this method 
   * we need to save these two entries somewhere
   * and then assign them back accordingly
   */
  uintptr_t **i2i_entry = (uintptr_t**)(method + i2i_entry_off);
  uintptr_t **fi_entry  = (uintptr_t**)(method + fi_entry_off);
  uintptr_t **fc_entry  = (uintptr_t**)(method + fc_entry_off);

  jhook_logger_log(__func__,"Method @ %p", method);
  jhook_logger_log(__func__,"i2i @ %p -> %p",  *i2i_entry, hook_addr_interpreted);
  jhook_logger_log(__func__,"fi  @ %p -> %p",  *fi_entry,  hook_addr_interpreted);
  jhook_logger_log(__func__,"fc  @ %p -> %p",  *fc_entry,  hook_addr_compiled);

  // save
  *orig_i2i = *i2i_entry; 
  *orig_fi  = *fi_entry;
  *orig_fc  = *fc_entry;

  // rewrite
  *i2i_entry     = hook_addr_interpreted;
  *fi_entry      = hook_addr_interpreted;
  *fc_entry      = hook_addr_compiled;
}

JHOOK void jhook_set_flags(jmethodID mid, int flags){
  __Method method = jhook_resolve_jmethod_id(mid);
  *(uint16_t*)(method+flags_entry_off) |= flags;
}

JHOOK void jhook_set_original_noinline_flags(JNIEnv *env, jvmtiEnv *jvmti, hook_t *hooks, size_t size){
  for(size_t i = 0; i < size; i++){
    jmethodID method = hooks[i].method_id_orig;
    jclass    clazz  = jhook_find_class(env, hooks[i].class_name);
    (*jvmti)->RetransformClasses(jvmti, 1, &clazz);
    jhook_set_flags(method, ((1 << 12) | (1 << 8) | (1 << 9) | (1 << 10)));
  }
}

JHOOK void jhook_remove_hook(jmethodID mid, uintptr_t *orig_i2i, uintptr_t *orig_fi, uintptr_t *orig_fc){
  __Method method      = jhook_resolve_jmethod_id(mid);
  uintptr_t **i2i_entry = (uintptr_t**)(method + i2i_entry_off);
  uintptr_t **fi_entry  = (uintptr_t**)(method + fi_entry_off);
  uintptr_t **fc_entry  = (uintptr_t**)(method + fc_entry_off);

  // restore
  *i2i_entry     = orig_i2i;
  *fi_entry      = orig_fi;
  *fc_entry      = orig_fc;
}

JHOOK __Method jhook_resolve_jmethod_id(jmethodID mid){
  return *((__Method*)mid);
}

JHOOK bool jhook_attach_current_thread(JavaVM *vm, void **penv, void *args){
  if((*vm)->AttachCurrentThread(vm, penv, args) != JNI_OK){
    jhook_logger_fatal(__func__, "failed to attach to jvm");
    return false;
  }
  jhook_logger_log(__func__, "attached to jvm thread");
  return true;
}

JHOOK bool jhook_get_java_vms(JavaVM **vm){
	jsize jvm_count;
	if(JNI_GetCreatedJavaVMs(vm, 1, &jvm_count) != JNI_OK){
    jhook_logger_fatal(__func__, "failed to get jvms");
		return false;
	}

  jhook_logger_log(__func__,"jvm @ %p", *vm);
  return true;
}

JHOOK bool jhook_get_jvmti(JavaVM *vm, jvmtiEnv **jvmti, int jvmti_version){
  jvmtiCapabilities caps; 

  if(((*vm)->GetEnv(vm, (void**)jvmti, jvmti_version) != JNI_OK)){ 
    jhook_logger_fatal(__func__, "failed to get jvmti");
		return false;
	}

  if((**jvmti)->GetPotentialCapabilities(*jvmti, &caps) != JVMTI_ERROR_NONE){
    jhook_logger_fatal(__func__, "failed to get capabilities");
		return false;
  }

  caps.can_suspend               = true;
  caps.can_retransform_classes   = true;
  caps.can_retransform_any_class = true;

  if((**jvmti)->AddCapabilities(*jvmti, &caps) != JVMTI_ERROR_NONE){
    jhook_logger_fatal(__func__, "failed to set capabilities");
		return false;
  }
  jhook_logger_log(__func__, "jvmti @ %p", **jvmti);
  return true;
}

JHOOK bool jhook_suspend_all_threads(JNIEnv *env, jvmtiEnv *jvmti){
  jthread current;
  jthread *threads;
  jint threads_length;

  if(g_ath_are_suspended) return true;

  if((*jvmti)->GetCurrentThread(jvmti, &current) != JVMTI_ERROR_NONE){
    jhook_logger_fatal(__func__, "failed to get current thread");
    return false;
  }

  if((*jvmti)->GetAllThreads(jvmti, &threads_length, &threads) != JVMTI_ERROR_NONE){
    jhook_logger_fatal(__func__, "failed to get all threads");
    return false;
  }

  for(jint i = 0; i < threads_length; i++){
    if((*env)->IsSameObject(env, current, threads[i])) continue;
    (*jvmti)->SuspendThread(jvmti, threads[i]);  
  }

  g_ath_are_suspended = true;
  jhook_logger_log(__func__, "all threads are suspended");
  return true;
}

JHOOK bool jhook_resume_all_threads(JNIEnv *env, jvmtiEnv *jvmti){
  jthread current;
  jthread *threads;
  jint threads_length;

  if(!g_ath_are_suspended) return true;

  if((*jvmti)->GetCurrentThread(jvmti, &current) != JVMTI_ERROR_NONE){
    jhook_logger_fatal(__func__, "failed to get current thread");
    return false;
  }

  if((*jvmti)->GetAllThreads(jvmti, &threads_length, &threads) != JVMTI_ERROR_NONE){
    jhook_logger_fatal(__func__, "failed to get all threads");
    return false;
  }

  for(jint i = 0; i < threads_length; i++){
    if((*env)->IsSameObject(env, current, threads[i])) continue;
    (*jvmti)->ResumeThread(jvmti, threads[i]);  
  }

  g_ath_are_suspended = false;
  jhook_logger_log(__func__, "all threads are resumed");
  return true;
}

JHOOK jclass jhook_create_class(JNIEnv *env, const char *class_name, const char *src_path){
  /* classes */
  jclass string_class, file_class, url_class, tool_provider_class, compiler_class, loader_class; 
  /* methods */
  jmethodID file_constructor, get_compiler, compiler_run, get_parent_file, to_uri, to_url, loader_constructor, load_class; 

  SAFE_RETURN(string_class, "class", {
    string_class = jhook_find_class(env, "java/lang/String");         
  });
  SAFE_RETURN(file_class, "class", {
    file_class = jhook_find_class(env, "java/io/File");             
  });
  SAFE_RETURN(url_class, "class", {
    url_class = jhook_find_class(env, "java/net/URL");             
  });
  SAFE_RETURN(tool_provider_class, "class", {
    tool_provider_class = jhook_find_class(env, "javax/tools/ToolProvider"); 
  });
  SAFE_RETURN(compiler_class, "class", {
    compiler_class = jhook_find_class(env, "javax/tools/JavaCompiler"); 
  });
  SAFE_RETURN(loader_class, "class", {
    loader_class = jhook_find_class(env, "java/net/URLClassLoader");  
  });

  SAFE_RETURN(file_constructor, "method", {
    file_constructor = (*env)->GetMethodID(env, file_class, "<init>", "(Ljava/lang/String;)V");
  });
  SAFE_RETURN(get_compiler, "method", {
    get_compiler = (*env)->GetStaticMethodID(env, tool_provider_class, "getSystemJavaCompiler", "()Ljavax/tools/JavaCompiler;");
  });
  SAFE_RETURN(compiler_run, "method", {
    compiler_run = (*env)->GetMethodID(env, compiler_class, "run", "(Ljava/io/InputStream;Ljava/io/OutputStream;Ljava/io/OutputStream;[Ljava/lang/String;)I");
  });
  SAFE_RETURN(get_parent_file, "method", {
    get_parent_file = (*env)->GetMethodID(env, file_class, "getParentFile", "()Ljava/io/File;");
  });
  SAFE_RETURN(to_uri, "method", {
    to_uri = (*env)->GetMethodID(env, file_class, "toURI", "()Ljava/net/URI;");
  });

  /* vars */
  jstring path_str             = (*env)->NewStringUTF(env, src_path);
  jobject source_file          = (*env)->NewObject(env, file_class, file_constructor, path_str);
  jobjectArray args_arr        = (*env)->NewObjectArray(env, 1, string_class, NULL);
  jobject parent_directory     = (*env)->CallObjectMethod(env, source_file, get_parent_file);
  jobject compiler             = (*env)->CallStaticObjectMethod(env, tool_provider_class, get_compiler);
  
  // compiler.run 
  (*env)->SetObjectArrayElement(env, args_arr, 0, path_str);
  (*env)->CallIntMethod(env, compiler, compiler_run, NULL, NULL, NULL, args_arr); 

  jobject uri                  = (*env)->CallObjectMethod(env, parent_directory, to_uri);
  jclass uri_class             = (*env)->GetObjectClass(env, uri);
  SAFE_RETURN(to_url, "method", {
    to_url = (*env)->GetMethodID(env, uri_class, "toURL", "()Ljava/net/URL;");
  });
  jobject url                  = (*env)->CallObjectMethod(env, uri, to_url);
  jobjectArray url_array       = (*env)->NewObjectArray(env, 1, url_class, url);
  SAFE_RETURN(loader_constructor, "method", {
    loader_constructor = (*env)->GetStaticMethodID(env, loader_class, "newInstance", "([Ljava/net/URL;)Ljava/net/URLClassLoader;");
  });
  jobject class_loader         = (*env)->CallStaticObjectMethod(env, loader_class, loader_constructor, url_array);
  
  SAFE_RETURN(load_class, "method", {
    load_class = (*env)->GetMethodID(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
  });
  jstring class_name_str       = (*env)->NewStringUTF(env, class_name);
  jclass loaded_class          = (*env)->CallObjectMethod(env, class_loader, load_class, class_name_str);

  return loaded_class;
}

JHOOK jclass jhook_find_class(JNIEnv *env, const char *class_name){
  jclass tmp_class;
  tmp_class = (*env)->FindClass(env, class_name);
  if(tmp_class == NULL){
    jhook_logger_fatal(__func__, "failed to obtain class <%s> address", class_name);
    return NULL;
  }
  jhook_logger_log(__func__,"class address @ %p", tmp_class);
  return tmp_class;
}

JHOOK jmethodID jhook_find_method2(JNIEnv *env, jclass clazz, const char* method_name, const char *method_signature, bool is_static){
  jmethodID t_method;

  if(is_static)
    t_method = (*env)->GetStaticMethodID(env, clazz, method_name, method_signature);
  else
    t_method = (*env)->GetMethodID(env, clazz, method_name, method_signature);

  if(t_method == NULL){
    jhook_logger_fatal(__func__, "failed to get <%s> method id", method_name);
    return NULL;
  }

  return t_method;
}

JHOOK jmethodID jhook_find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static){
  jclass t_class;
  jmethodID t_method;
  t_class = jhook_find_class(env, class_name);

  if(t_class == NULL){
    jhook_logger_fatal(__func__, "failed to obtain class <%s> for method", class_name);
    return NULL;
  }

  if(is_static)
    t_method = (*env)->GetStaticMethodID(env, t_class, method_name, method_signature);
  else
    t_method = (*env)->GetMethodID(env, t_class, method_name, method_signature);

  if(t_method == NULL){
    jhook_logger_fatal(__func__, "failed to get <%s> method id", method_name);
    return NULL;
  }

  return t_method;
}

JHOOK bool jhook_resolve_original_methods(JNIEnv *env, jvmtiEnv *jvmti, hook_t *hooks, size_t size){
  jboolean is_native;
  for(size_t i = 0; i < size; i++){
    jmethodID method_id = jhook_find_method(env, hooks[i].class_name, hooks[i].method_name, hooks[i].method_sig, hooks[i].method_is_static); 
    if(method_id == NULL){
      jhook_logger_fatal(__func__, "failed to resolve %s", hooks[i].method_name);
      return false;
    }
    if((*jvmti)->IsMethodNative(jvmti, method_id, &is_native) != JVMTI_ERROR_NONE){
      jhook_logger_fatal(__func__, "failed to detect if method %s is native", hooks[i].method_name); 
      return false;
    }
    if(is_native){
      jhook_logger_warn(__func__, "not yet implemented! method %s is native", hooks[i].method_name);
      return false;
    }
    
    hooks[i].method_id_orig = method_id;
  }
  return true;
}

JHOOK char* jhook_get_class_path(JNIEnv *env, jclass clazz, const char *class_name){
  jclass clazz_class, pd_class, cs_class, url_class;
  jmethodID get_pd, get_cs, get_location, get_file;

  SAFE_RETURN(clazz_class, "class", {
    clazz_class = (*env)->FindClass(env, "java/lang/Class");                
  });

  SAFE_RETURN(pd_class  , "class", {
    pd_class = (*env)->FindClass(env, "java/security/ProtectionDomain"); 
  });

  SAFE_RETURN(cs_class  , "class", {
    cs_class = (*env)->FindClass(env, "java/security/CodeSource");       
  });

  SAFE_RETURN(url_class , "class", {
    url_class = (*env)->FindClass(env, "java/net/URL");                   
  });

  SAFE_RETURN(get_pd, "method", {
    get_pd = (*env)->GetMethodID(env, clazz_class, "getProtectionDomain", "()Ljava/security/ProtectionDomain;");
  });
  jobject pd = (*env)->CallObjectMethod(env, clazz, get_pd);
  
  SAFE_RETURN(get_cs, "method", {
    get_cs = (*env)->GetMethodID(env, pd_class, "getCodeSource", "()Ljava/security/CodeSource;");
  });
  jobject cs = (*env)->CallObjectMethod(env, pd, get_cs);
  
  SAFE_RETURN(get_location, "method", {
    get_location = (*env)->GetMethodID(env, cs_class, "getLocation", "()Ljava/net/URL;");
  });
  jobject url = (*env)->CallObjectMethod(env, cs, get_location);
  
  SAFE_RETURN(get_file, "method", {
    get_file = (*env)->GetMethodID(env, url_class, "getFile", "()Ljava/lang/String;");
  });
  jstring file_path = (jstring)(*env)->CallObjectMethod(env, url, get_file);

  const char *temp_path = (*env)->GetStringUTFChars(env, file_path, NULL);
  char *path            = malloc(PATH_MAX);
  if(path == NULL){
    jhook_logger_fatal(__func__, "failed to allocate memory"); 
    goto end;
  }
  snprintf(path, PATH_MAX, "%s/%s.class", temp_path, class_name);
end:
  (*env)->ReleaseStringUTFChars(env, file_path, temp_path);
  return path;
}

JHOOK bool jhook_resolve_hook_methods(JNIEnv *env, jclass clazz, hook_t *hooks, size_t size){
  for(size_t i = 0; i < size; i++){
    jmethodID method_id = jhook_find_method2(env, clazz, hooks[i].native_detour[0].name, hooks[i].native_detour[0].signature, hooks[i].method_is_static); 
    if(method_id == NULL){
      jhook_logger_fatal(__func__, "failed to resolve %s", hooks[i].method_name);
      return false;
    }
    hooks[i].method_id_hook = method_id;
  }
  return true;
}

JHOOK bool jhook_register_hook_methods(JNIEnv *env, jclass clazz, hook_t *hooks, size_t size){
  for(size_t i = 0; i < size; i++){
    if((*env)->RegisterNatives(env, clazz, hooks[i].native_detour, ARR_LENGTH(hooks[i].native_detour)) != 0){
      jhook_logger_fatal(__func__, "failed to register native method!");
      return false;
    }
  }
  return true;
}

JHOOK uintptr_t* jhook_method_interpreter_get(__Method method){
  return *(uintptr_t**)(method + i2i_entry_off); 
}

JHOOK uintptr_t* jhook_method_from_compiled(__Method method){
  return *(uintptr_t**)(method + fc_entry_off); 
}

JHOOK int jhook_strpos(const char *heystack, const char *needle){
  char *found_at;
  if((found_at = strstr(heystack, needle)) != NULL){
    return found_at - heystack;
  }
  return -1;
}

JHOOK bool jhook_resolve_class_dependencies(JNIEnv *env, hook_t *hooks, size_t size){
#define MAX_READ 4096
  bool status = false;
  char *path_from = NULL;
  char path_to[PATH_MAX]; 
  FILE *fd_from, *fd_to;
  size_t bytes_read;
  char buffer[MAX_READ];

  for(size_t i = 0; i < size; i++){
    if(hooks[i].dependencies.size == 0) continue;
    for(size_t j = 0; j < hooks[i].dependencies.size; j++){
      jclass clazz = jhook_find_class(env, hooks[i].dependencies.name[j]);

      if(clazz == NULL){
        jhook_logger_fatal(__func__, "failed to obtain class %s", hooks[i].dependencies.name[j]);
        goto end;
      }
      
      if( (path_from = jhook_get_class_path(env, clazz, hooks[i].dependencies.name[j])) == NULL ){
        jhook_logger_fatal(__func__, "failed to get class path for %s", hooks[i].dependencies.name[j]);
        goto end;
      }

      snprintf(path_to, PATH_MAX, "%s/%s.class",  PATH, hooks[i].dependencies.name[j]);
      fd_from = fopen(path_from, "rb");
      fd_to   = fopen(path_to, "wb");

      if(fd_from == NULL){
        jhook_logger_fatal(__func__, "failed to open %s", path_from);
        goto end;
      }
      if(fd_to == NULL){
        jhook_logger_fatal(__func__, "failed to open %s", path_to);
        goto end;
      }

      while ((bytes_read = fread(buffer, 1, MAX_READ, fd_from)) != 0) {
        if(fwrite(buffer, 1, bytes_read, fd_to) != bytes_read) {
          jhook_logger_fatal(__func__, "write failed");
          goto end; 
        }
      }
      fclose(fd_to);   fd_to     = NULL;
      fclose(fd_from); fd_from   = NULL;
      free(path_from);    path_from = NULL;
    }
  }
  status = true;
end:
  if(fd_to     != NULL) fclose(fd_to);
  if(fd_from   != NULL) fclose(fd_from);
  free(path_from);
  return status;
}

JHOOK bool jhook_tempfile_create(void){
  g_tempfile_fd = mkstemps(g_tempfile_path, sizeof(SUFFIX) - 1);
  if(g_tempfile_fd == -1){
    jhook_logger_fatal(__func__, "failed to create temp file name"); 
    return false;
  }

  char *bname = basename(g_tempfile_path);
  size_t limit = jhook_strpos(bname, SUFFIX);
  strncpy(g_tempfile_class_name, bname, limit);
  jhook_logger_log(__func__, "temp file created @ %s", g_tempfile_path);
  jhook_logger_log(__func__, "class_name is %s", g_tempfile_class_name);

  return true;
}

JHOOK char *jhook_tempfile_get_path(void){
  return g_tempfile_path;
}

JHOOK char *jhook_tempfile_get_class_name(void){
  return g_tempfile_class_name;
}

JHOOK void jhook_tempfile_remove(void){
  if(g_tempfile_fd == -1) return;
  if(remove(g_tempfile_path) == -1){
    jhook_logger_fatal(__func__, "failed to remove temp file @ %s", g_tempfile_path);
    return;
  }
  jhook_logger_log(__func__, "removed temp file @ %s", g_tempfile_path); 
}

JHOOK bool jhook_tempfile_generate_java_code(jvmtiEnv *jvmti, hook_t *hooks, size_t size){
  /* 
   * TODO:
   * add automatic name demangler for methods that are contained inside hooks struct
   */

  /* compute the size */
  bool status = false;
  char *source_code;
  size_t file_size = 0;
  file_size += sizeof("public class ");
  file_size += sizeof(BASE_NAME);
  file_size += sizeof(" {\n");
  for(size_t i = 0; i < size; i++)
    file_size += strlen(hooks[i].method_java_name) + sizeof('\n');

  file_size += sizeof("}\n");
  source_code = malloc(file_size);
  if(source_code == NULL){
    jhook_logger_fatal(__func__, "malloc failed");
    goto end;
  }

  /* append to the source code */
  sprintf(source_code, "public class %s {\n", g_tempfile_class_name);
  for(size_t i = 0; i < size; i++){
    jhook_logger_log(__func__, "generating method for %s", hooks[i].method_name);
    strcat(source_code, hooks[i].method_java_name);
    strcat(source_code, "\n");
  }
  strcat(source_code, "}\n");

  /* now write it to the file */
  if(write(g_tempfile_fd, source_code, strlen(source_code)) == -1){
    jhook_logger_fatal(__func__, "failed to write to the temp file");
    goto end;
  } 
  status = true;
end:
  free(source_code);
  return status;
}

JHOOK void jhook_logger_log(const char *func, const char *fmt, ...){
  char buffer[LOG_MAX_BUFFER_SIZE];
  char time_buffer[LOG_MAX_TIME_BUFFER_SIZE];
  time_t now;
  struct tm* tm;
  va_list args;

  now = time(NULL);
  tm  = localtime(&now);
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  strftime(time_buffer, sizeof(time_buffer), "%F %X", tm);
  printf(COLOR_START(GREEN)"[LOG @ %s]"COLOR_START(YELLOW)" %s:"COLOR_END" %s\n", time_buffer, func, buffer);
  va_end(args);
}

JHOOK void jhook_logger_warn(const char *func, const char *fmt, ...){
  char buffer[LOG_MAX_BUFFER_SIZE];
  char time_buffer[LOG_MAX_TIME_BUFFER_SIZE];
  time_t now;
  struct tm* tm;
  va_list args;

  now = time(NULL);
  tm  = localtime(&now);
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  strftime(time_buffer, sizeof(time_buffer), "%F %X", tm);
  printf(COLOR_START(RED)"[WARN @ %s]"COLOR_START(YELLOW)" %s:"COLOR_END" %s\n", time_buffer, func, buffer);
  va_end(args);
}

JHOOK void jhook_logger_fatal(const char *func, const char *fmt, ...){
  char buffer[LOG_MAX_BUFFER_SIZE];
  char time_buffer[LOG_MAX_TIME_BUFFER_SIZE];
  time_t now;
  struct tm* tm;
  va_list args;

  now = time(NULL);
  tm  = localtime(&now);
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  strftime(time_buffer, sizeof(time_buffer), "%F %X", tm);
  fprintf(stderr, COLOR_START(RED)"[FATAL @ %s]"COLOR_START(YELLOW)" %s:"COLOR_END" %s\n", time_buffer, func, buffer);
  va_end(args);
}

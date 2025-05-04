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
#include <jni.h>
#include <jvmti.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "logger.h"
#include "hooks.h"
#include "helpers.h"

void* safe_malloc(const char *f, size_t n){
  void* b = malloc(n);
  if(b == NULL){
    logger_fatal(f, "malloc failed");
    return NULL;
  }
  memset(b, 0, n);
  return b;
}

void* safe_realloc(const char *f, void *p, size_t n){
  void* b = realloc(p, n);
  if(b == NULL){
    logger_fatal(f, "realloc failed");
  }
  return b;
}

bool attach_current_thread(JavaVM *vm, void **penv, void *args){
  if((*vm)->AttachCurrentThread(vm, penv, args) != JNI_OK){
    logger_fatal(__func__, "failed to attach to jvm");
    return false;
  }
  logger_log(__func__, "attached to jvm thread");
  return true;
}

jclass create_class(JNIEnv *env, const char *class_name, const char *src_path){
  /* classes */
  jclass string_class        = (*env)->FindClass(env, "java/lang/String");
  jclass file_class          = (*env)->FindClass(env, "java/io/File");
  jclass url_class           = (*env)->FindClass(env, "java/net/URL");
  jclass tool_provider_class = (*env)->FindClass(env, "javax/tools/ToolProvider");
  jclass compiler_class      = (*env)->FindClass(env, "javax/tools/JavaCompiler");

  /* methods */
  jmethodID file_constructor  = (*env)->GetMethodID(env, file_class, "<init>", "(Ljava/lang/String;)V");
  jmethodID get_compiler_m    = (*env)->GetStaticMethodID(env, tool_provider_class, "getSystemJavaCompiler", "()Ljavax/tools/JavaCompiler;");
  jmethodID compiler_run      = (*env)->GetMethodID(env, compiler_class, "run", "(Ljava/io/InputStream;Ljava/io/OutputStream;Ljava/io/OutputStream;[Ljava/lang/String;)I");
  jmethodID get_parent_file_m = (*env)->GetMethodID(env, file_class, "getParentFile", "()Ljava/io/File;");
  jmethodID to_uri            = (*env)->GetMethodID(env, file_class, "toURI", "()Ljava/net/URI;");

  /* vars */
  jstring path_str         = (*env)->NewStringUTF(env, src_path);
  jobject source_file      = (*env)->NewObject(env, file_class, file_constructor, path_str);
  jobjectArray args_arr    = (*env)->NewObjectArray(env, 1, string_class, NULL);
  jobject parent_directory = (*env)->CallObjectMethod(env, source_file, get_parent_file_m);
  jobject compiler         = (*env)->CallStaticObjectMethod(env, tool_provider_class, get_compiler_m);
  
  // compiler.run 
  (*env)->SetObjectArrayElement(env, args_arr, 0, path_str);
  (*env)->CallIntMethod(env, compiler, compiler_run, NULL, NULL, NULL, args_arr); 

  jobject uri                  = (*env)->CallObjectMethod(env, parent_directory, to_uri);
  jclass uri_class             = (*env)->GetObjectClass(env, uri);
  jmethodID to_url             = (*env)->GetMethodID(env, uri_class, "toURL", "()Ljava/net/URL;");
  jobject url                  = (*env)->CallObjectMethod(env, uri, to_url);
  jobjectArray url_array       = (*env)->NewObjectArray(env, 1, url_class, url);
  jclass loader_class          = (*env)->FindClass(env, "java/net/URLClassLoader");
  jmethodID loader_constructor = (*env)->GetStaticMethodID(env, loader_class, "newInstance",
      "([Ljava/net/URL;)Ljava/net/URLClassLoader;");
  jobject class_loader         = (*env)->CallStaticObjectMethod(env, loader_class, loader_constructor, url_array);
  
  jmethodID load_class   = (*env)->GetMethodID(env, loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
  jstring class_name_str = (*env)->NewStringUTF(env, class_name);
  jclass loaded_class    = (*env)->CallObjectMethod(env, class_loader, load_class, class_name_str);

  return loaded_class;
}

bool get_jvmti(JavaVM *vm, jvmtiEnv **jvmti){
  jvmtiCapabilities caps; 

  if(((*vm)->GetEnv(vm, (void**)jvmti, JVMTI_VERSION_1_2) != JNI_OK)){ 
    logger_fatal(__func__, "failed to get jvmti");
		return false;
	}

  if((**jvmti)->GetPotentialCapabilities(*jvmti, &caps) != JVMTI_ERROR_NONE){
    logger_fatal(__func__, "failed to get capabilities");
		return false;
  }

  caps.can_suspend = true;
  if((**jvmti)->AddCapabilities(*jvmti, &caps) != JVMTI_ERROR_NONE){
    logger_fatal(__func__, "failed to set capabilities");
		return false;
  }
  logger_log(__func__, "jvmti @ %p", **jvmti);
  return true;
}

bool resolve_original_methods(JNIEnv *env, hook_t *hooks, size_t size){
  for(size_t i = 0; i < size; i++){
    jmethodID method_id = find_method(env, hooks[i].class_name, hooks[i].method_name, hooks[i].method_sig, hooks[i].method_is_static); 
    if(method_id == NULL){
      logger_fatal(__func__, "failed to resolve %s", hooks[i].method_name);
      return false;
    }
    hooks[i].method_id_orig = method_id;
  }
  return true;
}

bool resolve_hook_methods(JNIEnv *env, jclass clazz, hook_t *hooks, size_t size){
  for(size_t i = 0; i < size; i++){
    jmethodID method_id = (*env)->GetMethodID(env, clazz, hooks[i].method_name, hooks[i].method_sig);
    if(method_id == NULL){
      logger_fatal(__func__, "failed to resolve %s", hooks[i].method_name);
      return false;
    }
    hooks[i].method_id_hook = method_id;
  }
  return true;
}

bool register_hook_methods(JNIEnv *env, jclass clazz, hook_t *hooks, size_t size){
  for(size_t i = 0; i < size; i++){
    if((*env)->RegisterNatives(env, clazz, hooks[i].native_detour, ARR_LENGTH(hooks[i].native_detour)) != 0){
      logger_fatal(__func__, "failed to register native method!");
      return false;
    }
  }
  return true;
}

bool suspend_all_threads(JNIEnv *env, jvmtiEnv *jvmti){
  jthread current;
  jthread *threads;
  jint threads_length;

  if((*jvmti)->GetCurrentThread(jvmti, &current) != JVMTI_ERROR_NONE){
    logger_fatal(__func__, "failed to get current thread");
    return false;
  }

  if((*jvmti)->GetAllThreads(jvmti, &threads_length, &threads) != JVMTI_ERROR_NONE){
    logger_fatal(__func__, "failed to get all threads");
    return false;
  }

  for(jint i = 0; i < threads_length; i++){
    if((*env)->IsSameObject(env, current, threads[i])) continue;
    (*jvmti)->SuspendThread(jvmti, threads[i]);  
  }

  logger_log(__func__, "all threads are suspended");
  return true;
}

bool resume_all_threads(JNIEnv *env, jvmtiEnv *jvmti){
  jthread current;
  jthread *threads;
  jint threads_length;

  if((*jvmti)->GetCurrentThread(jvmti, &current) != JVMTI_ERROR_NONE){
    logger_fatal(__func__, "failed to get current thread");
    return false;
  }

  if((*jvmti)->GetAllThreads(jvmti, &threads_length, &threads) != JVMTI_ERROR_NONE){
    logger_fatal(__func__, "failed to get all threads");
    return false;
  }

  for(jint i = 0; i < threads_length; i++){
    if((*env)->IsSameObject(env, current, threads[i])) continue;
    (*jvmti)->ResumeThread(jvmti, threads[i]);  
  }

  logger_log(__func__, "all threads are resumed");
  return true;
}

bool get_java_vms(JavaVM **vm){
	jsize jvm_count;
	if(JNI_GetCreatedJavaVMs(vm, 1, &jvm_count) != JNI_OK) {
    logger_fatal(__func__, "failed to get jvms");
		return false;
	}

  logger_log(__func__,"jvm @ %p", *vm);
  return true;
}

jclass find_class(JNIEnv *env, const char *class_name){
  jclass tmp_class;
  tmp_class = (*env)->FindClass(env, class_name);
  if(tmp_class == NULL){
    logger_fatal(__func__, "failed to obtain class address");
    return NULL;
  }
  logger_log(__func__,"class address @ %p", tmp_class);
  return tmp_class;
}

jmethodID find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static){
  jclass t_class;
  jmethodID t_method;
  t_class = find_class(env, class_name);

  if(t_class == NULL){
    logger_fatal(__func__, "failed to obtain class <%s> for method", class_name);
    return NULL;
  }

  if(is_static)
    t_method = (*env)->GetStaticMethodID(env, t_class, method_name, method_signature);
  else
    t_method = (*env)->GetMethodID(env, t_class, method_name, method_signature);

  if(t_method == NULL){
    logger_fatal(__func__, "failed to get <%s> method id", method_name);
    return NULL;
  }

  return t_method;
}

__Method resolve_jmethod_id(jmethodID mid){
  return *((__Method*)mid);
}

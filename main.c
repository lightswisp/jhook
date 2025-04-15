#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <jni.h>
#include <jvmti.h>

#include "helpers.h"
#include "offsets.h"
#include "hooks.h"
#include "dump.h"
#include "defines.h"
#include "logger.h"

/* jvm globals */
static JavaVM   *g_jvm;
static JNIEnv   *g_env;

void      *__main_thread(void *a);
pthread_t g_thread;

/* run this function on library load */
__attribute__((constructor)) void __library_startup(){
  logger_log(__func__, "injected !");
  logger_log(__func__, "now trying to create new thread");
  pthread_attr_t attr;

  /* start thread detached */
  if (-1 == pthread_attr_init(&attr)) 
    return;
  if (-1 == pthread_attr_setdetachstate(&attr,
                          PTHREAD_CREATE_DETACHED)) 
    return;

  /* spawn a thread to do the real work */
  pthread_create(&g_thread, NULL, __main_thread, NULL);
}

HOOK_INIT(str_test);
HOOK_ENTRY(str_test);
JNIEXPORT jstring JNICALL test_native
  (JNIEnv* env, jobject thisObject, jint a) {
    printf("grabbed args: %d\n", a);
    char buf[] = "test_native!";
    
    REMOVE_HOOK(str_test);
    jobject r = (*env)->CallObjectMethod(env, thisObject, GET_HOOK_NAME_BY_IDX(str_test), a);
    _SET_HOOK(str_test);

    if(r == NULL) return r;

    const char *r_s = (*env)->GetStringUTFChars(env, r, 0);
    printf("real result: %s\n", r_s);
    return r;
}

HOOK_INIT(readLine);
HOOK_ENTRY(readLine);
JNIEXPORT jstring JNICALL readLine_hk (JNIEnv* env, jobject thisObject) {

    REMOVE_HOOK(readLine);
    jobject r = (*env)->CallObjectMethod(env, thisObject, GET_HOOK_NAME_BY_IDX(readLine));
    _SET_HOOK(readLine);

    if(r == NULL) return r;

    const char *r_s = (*env)->GetStringUTFChars(env, r, 0);
    printf("INTERCEPTED: %s\n", r_s);
    return r;
}

/* our main loop */
void* __main_thread(void *a)
{
  // TODO 
  // replace asserts with custom made assert that will uninject 
  // the library on failure. It is really crutial to keep the java process alive 
  // instead of just exiting it.

  /* this is where we specify the function to hook */
  #define CLASS_NAME        "java/io/BufferedReader"
  #define METHOD_NAME       "readLine"
  #define METHOD_NAME_HK    "aboba"
  #define METHOD_SIG        "()Ljava/lang/String;"
  #define METHOD_IS_STATIC  false

  if(!init_libjvm())
    return NULL;

  if(get_java_vms(&g_jvm) != JNI_OK)             
    return NULL;

  if(attach_current_thread(g_jvm, (void**)&g_env, NULL) != JNI_OK) 
    return NULL;

  logger_log(__func__, "attached to jvm thread");

  GET_CLASS_NAME_BY_IDX(readLine) = find_class(g_env, CLASS_NAME);
  GET_HOOK_NAME_BY_IDX(readLine) = find_method(
      g_env, 
      CLASS_NAME, 
      METHOD_NAME, 
      METHOD_SIG, 
      METHOD_IS_STATIC);

  if(GET_HOOK_NAME_BY_IDX(readLine) == NULL) return NULL;

  static JNINativeMethod methods[] = {
  {METHOD_NAME_HK, METHOD_SIG, (void*) &readLine_hk },
  }; 

  /* this one shouldn't fail */
  __Method orig = resolve_jmethod_id(GET_HOOK_NAME_BY_IDX(readLine));
  __Method m = method_create(orig, METHOD_NAME_HK, METHOD_SIG);
  if(m == NULL) return NULL;

  __GrowableArray array = array_create(1);
  if(array == NULL) return NULL;

  array_push(array, &m);
  method_merge(array, klass_get(orig)); 

  int r = (*g_env)->RegisterNatives(g_env, GET_CLASS_NAME_BY_IDX(readLine), methods, sizeof(methods)/sizeof(methods[0]));
  if(r != 0){
    logger_fatal(__func__, "failed to register native method!");
    return NULL;
  }

  SET_HOOK(readLine, m, method_interpreter_get(m));
  return NULL;
}

__attribute__((destructor)) void __library_shutdown(){
  logger_log(__func__, "unloading");
  //REMOVE_HOOK(readLine);
  //if(g_jvm != NULL) (*g_jvm)->DetachCurrentThread(g_jvm);
}

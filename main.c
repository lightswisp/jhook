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

//#include "helpers.h"
//#include "hooks.h"
//#include "tempfile.h"
//#include "logger.h"
#include "jhook.h"

void      *__main_thread(void *a);
pthread_t g_thread;

/* run this function on library load */
__attribute__((constructor)) void __library_startup(){
  jhook_logger_log(__func__, "injected !");
  jhook_logger_log(__func__, "now trying to create new thread");
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

HOOK_INIT(readLine);
HOOK_ENTRY(readLine);
JNIEXPORT jstring JNICALL readLine_hk(JNIEnv* env, jobject this) {

    REMOVE_HOOK(readLine);
    jobject r = (*env)->CallObjectMethod(env, this, GET_HOOK_NAME_BY_IDX(readLine));
    _SET_HOOK(readLine);

    if(r == NULL) return r;

    const char *r_s = (*env)->GetStringUTFChars(env, r, 0);
    printf("INTERCEPTED: %s\n", r_s);
    return r;
}

HOOK_INIT(println);
HOOK_ENTRY(println);
JNIEXPORT void JNICALL println_hk(JNIEnv *env, jobject this, jstring string){
  printf("println was called!\n");
  return;
}

/* our main loop */
void* __main_thread(void *a)
{
  // TODO 
  // replace asserts with custom made assert that will uninject 
  // the library on failure. It is really crutial to keep the java process alive 
  // instead of just exiting it.

  /* this is where we specify functions to hook */

  hook_t hooks[] = {
    {
      "java/io/BufferedReader", 
      "readLine", 
      "()Ljava/lang/String;", 
      "public native java.lang.String readLine();",
      false,
      NULL,
      NULL,
      {
        {"readLine", "()Ljava/lang/String;", (void*) &readLine_hk },
      }
    },
    {
      "java/io/PrintWriter", 
      "println", 
      "(Ljava/lang/String;)V", 
      "public native void println(java.lang.String x);",
      false,
      NULL,
      NULL,
      {
        {"println", "(Ljava/lang/String;)V", (void*) &println_hk },
      }
    }
  };

  if(!jhook_init(JVMTI_VERSION_1_2)){
    jhook_logger_fatal(__func__, "failed to initialize jhook");
    return NULL;
  }

  if(!jhook_register(hooks, ARR_LENGTH(hooks))){
    jhook_logger_fatal(__func__, "failed to register hooks");
    return NULL;
  }

  SET_HOOK(readLine, hooks[0].method_id_orig, hooks[0].method_id_hook);
  return NULL;
}

__attribute__((destructor)) void __library_shutdown(){
  jhook_logger_log(__func__, "unloading");
  REMOVE_HOOK(readLine);
  //if(g_jvm != NULL) (*g_jvm)->DetachCurrentThread(g_jvm);
}

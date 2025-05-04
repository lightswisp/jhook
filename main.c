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

#include "helpers.h"
#include "hooks.h"
#include "tempfile.h"
#include "logger.h"

/* jvm globals */
static JavaVM   *g_jvm;
static jvmtiEnv *g_jvmti;
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

  if(!init_libjvm())
    goto end;

  if(!get_java_vms(&g_jvm))             
    goto end;

  if(!attach_current_thread(g_jvm, (void**)&g_env, NULL)) 
    goto end;

  if(!get_jvmti(g_jvm, &g_jvmti))
    goto end;

  if(!resolve_original_methods(g_env, hooks, ARR_LENGTH(hooks)))
    goto end;

  if(!suspend_all_threads(g_env, g_jvmti))
    goto end;

  if(!tempfile_create())
    goto end;

  if(!tempfile_generate_java_code(g_jvmti, hooks, ARR_LENGTH(hooks)))
    goto end;

  jclass class_hk = create_class(g_env, tempfile_get_class_name(), tempfile_get_path());

  if(!resolve_hook_methods(g_env, class_hk, hooks, ARR_LENGTH(hooks)))
   goto end; 

  if(!register_hook_methods(g_env, class_hk, hooks, ARR_LENGTH(hooks)))
    goto end;

  SET_HOOK(readLine, hooks[0].method_id_orig, hooks[0].method_id_hook);
  resume_all_threads(g_env, g_jvmti);
end:
  tempfile_remove();
  resume_all_threads(g_env, g_jvmti);
  return NULL;
}

__attribute__((destructor)) void __library_shutdown(){
  logger_log(__func__, "unloading");
  //REMOVE_HOOK(readLine);
  //if(g_jvm != NULL) (*g_jvm)->DetachCurrentThread(g_jvm);
}

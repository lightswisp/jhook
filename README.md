# jhook
Hooking java methods from C using JNI and some silly hacks

## quick demo
https://github.com/user-attachments/assets/bd073edf-46cc-4923-8ca8-a4f95d1fe720

## how does it work?
It works by replacing the original method’s `_i2i_entry` field with our 'bridge' function, which eventually leads to the native code. This bridge is created when defining `HOOK_ENTRY`. When execution lands inside it, the original `Method*` is located in the rbx register. If we replace it with our own `Method*`, the call will then be forwarded to our native function.

It is also important to jump to the native interpreter entry afterward, because our method is native and uses a different interpreter. To create such `Method`, we can either call JVM internal functions directly by their addresses or use JNI/JVMTI to assist us. The second option is preferable, as it delegates all the hassle to the JVM.

This library actually uses the second option by generating Java code and compiling it using JNI. We then load the class and register all methods so that they point to our native code.

## prerequisites
- installed java-sdk (we are dependant on javax)
- make 
- ruby
- gcc/clang

## how to use?

Include jhook.h to your project:
```c 
#include "jhook.h"
```
Define your hooks in a native way. This is where we will land after java calls the original method:
```c
HOOK_INIT(readLine);
HOOK_ENTRY(readLine);
JNIEXPORT jstring JNICALL readLine_hk(JNIEnv* env, jobject this) {
    jobject r;
    const char *r_s;

    // call the original and save its result
    CALL_ORIGINAL(readLine, {
      r = (*env)->CallObjectMethod(env, this, GET_HOOK_NAME_BY_IDX(readLine));
    });

    if(r == NULL) return r;

    r_s = (*env)->GetStringUTFChars(env, r, 0);
    jhook_logger_log(__func__,"readLine: %s", r_s);
    return r;
}
```
Then create an array of hooks:
```c
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
  }
};
```
Initialize everything and register our hooks, so that jvm could see them:
```c
if(!jhook_init(JVMTI_VERSION_1_2)){
  jhook_logger_fatal(__func__, "failed to initialize jhook");
  return NULL;
}

if(!jhook_register(hooks, ARR_LENGTH(hooks))){
  jhook_logger_fatal(__func__, "failed to register hooks");
  return NULL;
}
```
Finally call "SET_HOOK" for each individual method: 
```c
SET_HOOK(readLine, hooks[0]);
```
If you want to unset it:
```c
REMOVE_HOOK(readLine);
```

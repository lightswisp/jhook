#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <jni.h>
#include <jvmti.h>

static JavaVM   *g_jvm;
static JNIEnv   *g_env;
void set_hook(jmethodID mid, uint64_t *hook_addr, uint64_t **orig_i2i, uint64_t **orig_fi);
jmethodID find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static);

//#define CLASS_NAME "java/lang/Math"
//#define METHOD_NAME "pow"
//#define METHOD_SIGNATURE "(DD)D"

/* offsets */
#define fi_entry_off   0x0d
#define i2i_entry_off  0x0a
#define cmethod_off    0x02
#define arg_sz_off     0x2e
#define string_sz_off  0x24
#define string_ptr_off 0x28

/* hook helpers */
#define HOOK_INIT(x) \
  static jmethodID g_target_mid_##x;     \
  static uint64_t *g_orig_i2i_entry_##x; \
  static uint64_t *g_orig_fi_entry_##x;  \

#define HOOK_BODY(x) void hook_body_##x(uint64_t *args)

#define GET_METHOD_INSIDE_HOOK_BODY_BY_HOOK_IDX(x) resolve_jmethod_id(g_target_mid_##x)

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
  /* saving the context */                        \
  SAVE_CTX();                                     \
  /* passing arguments */                         \
  __asm__ volatile("mov %r13, %rdi");             \
  __asm__ volatile("call hook_body_"#x);          \
  /* restoring the context */                     \
  RESTORE_CTX();                                  \
  /* jumping to the original */                   \
  __asm__ volatile(                               \
    "jmpq *%[addr]"                               \
    :: [addr] "m" (g_orig_i2i_entry_##x)          \
  );                                              \
  while(1);                                       \
}

#define SET_HOOK(x) set_hook(g_target_mid_##x, (uint64_t*)hook_entry_##x, &g_orig_i2i_entry_##x, &g_orig_fi_entry_##x)
#define GET_HOOK_NAME_BY_IDX(x) (g_target_mid_##x)

void dump(uint8_t *address, size_t size){
  for(size_t i = 0; i < size; i++){
    if(i % 8 == 0)
      putchar('\n'); 
    printf("%02x ", address[i]);
  }
}

uint64_t *resolve_jmethod_id(jmethodID mid){
  return *((uint64_t**)mid);
}

void remove_hook(jmethodID mid, uint64_t *orig_i2i, uint64_t *orig_fi){
  uint64_t *method     = resolve_jmethod_id(mid);
  uint64_t **i2i_entry = (uint64_t**)(method + i2i_entry_off);
  uint64_t **fi_entry  = (uint64_t**)(method + fi_entry_off);

  // restore
  *i2i_entry     = orig_i2i;
  *fi_entry      = orig_fi;
}

void set_hook(jmethodID mid, uint64_t *hook_addr, uint64_t **orig_i2i, uint64_t **orig_fi){
  // TODO: check if method is native
  uint64_t *method     = resolve_jmethod_id(mid);
  /* in order to unhook this method 
   * we need to save these two entries somewhere
   * and then assign them back accordingly
   */
  uint64_t **i2i_entry = (uint64_t**)(method + i2i_entry_off);
  uint64_t **fi_entry  = (uint64_t**)(method + fi_entry_off);

  printf("i2i @ %p\n", i2i_entry);
  printf("fi @ %p\n", fi_entry);

  // save
  *orig_i2i = *i2i_entry; 
  *orig_fi  = *fi_entry;

  // rewrite
  *i2i_entry     = hook_addr;
  *fi_entry      = hook_addr;
}

void *__main_thread(void *a);

/* run this function on library load */
__attribute__((constructor)) void __library_startup(){
  printf("[+] injected!\n");
  printf("[+] now trying to create new thread\n");
  pthread_attr_t attr;
  pthread_t main_thread;

  /* start thread detached */
  if (-1 == pthread_attr_init(&attr)) 
    return;
  if (-1 == pthread_attr_setdetachstate(&attr,
                          PTHREAD_CREATE_DETACHED)) 
    return;

  /* spawn a thread to do the real work */
  pthread_create(&main_thread, NULL, __main_thread, NULL);
}

__attribute__((destructor)) void __library_shutdown(){
  printf("unloading...\n");
  (*g_jvm)->DetachCurrentThread(g_jvm);
}

jclass find_class(JNIEnv *env, const char *class_name){
  jclass tmp_class;
  tmp_class = (*env)->FindClass(env, class_name);
  if(tmp_class == NULL){
    fprintf(stderr, "[-] failed to obtain class address\n");
    return NULL;
  }
  printf("[+] class address @ %p\n", tmp_class);
  return tmp_class;
}

jint get_java_vms(){
  jint r;
	jsize jvm_count;
	if ((r = JNI_GetCreatedJavaVMs(&g_jvm, 1, &jvm_count)) != JNI_OK) {
		fprintf(stderr, "[-] failed to get jvms\n");
		return r;
	}
  printf("[+] jvm @ %p\n", g_jvm);
  return r;
}

jmethodID find_method(JNIEnv *env, const char *class_name, const char* method_name, const char *method_signature, bool is_static){
  jclass t_class;
  jmethodID t_method;
  t_class = find_class(env, class_name);

  if(t_class == NULL){
    fprintf(stderr, "[-] failed to obtain class <%s> for method\n", class_name);
    return NULL;
  }

  if(is_static)
    t_method = (*env)->GetStaticMethodID(env, t_class, method_name, method_signature);
  else
    t_method = (*env)->GetMethodID(env, t_class, method_name, method_signature);

  if(t_method == NULL){
    fprintf(stderr, "[-] failed to get <%s> method id\n", method_name);
    return NULL;
  }

  return t_method;
}

jint attach_current_thread(JavaVM *vm, void **penv, void *args){
  jint status;
  if((status = (*g_jvm)->AttachCurrentThread(g_jvm, penv, args)) != JNI_OK)
    fprintf(stderr, "[-] failed to attach to jvm\n");
  return status;
}

HOOK_INIT(println);
HOOK_BODY(println){
  uint64_t *method = GET_METHOD_INSIDE_HOOK_BODY_BY_HOOK_IDX(println);
  uint16_t arg_size = *(uint16_t*)((uint8_t*)*(method+cmethod_off)+arg_sz_off);

  printf("println hooked!\n");
  printf("arg size: %d\n", arg_size);

  uint32_t string_size = *(uint32_t*)(((uint8_t*)*args)+string_sz_off);
  char *temp_str = calloc(string_size+1, sizeof(char));
  printf("string size: %u\n", string_size); 
  memcpy(temp_str, ((uint8_t*)*args)+string_ptr_off, string_size);
  printf("%s\n", temp_str);
  dump((uint8_t*)temp_str, string_size);
  free(temp_str);
}
HOOK_ENTRY(println);

HOOK_INIT(readLine);
HOOK_BODY(readLine){
  uint64_t *method = GET_METHOD_INSIDE_HOOK_BODY_BY_HOOK_IDX(readLine);
  uint16_t arg_size = *(uint16_t*)((uint8_t*)*(method+cmethod_off)+arg_sz_off);

  printf("readLine hooked!\n");
  printf("arg size: %d\n", arg_size);

  asm("int3");
}
HOOK_ENTRY(readLine);

/* our main loop */
void* __main_thread(void *a)
{

  if(get_java_vms() != JNI_OK)             
    return NULL;

  if(attach_current_thread(g_jvm, (void**)&g_env, NULL) != JNI_OK) 
    return NULL;

  printf("[+] attached to jvm thread\n");
  GET_HOOK_NAME_BY_IDX(println) = find_method(
      g_env, 
      "java/io/PrintWriter", 
      "println", 
      "(Ljava/lang/String;)V", 
      false);

  GET_HOOK_NAME_BY_IDX(readLine) = find_method(
      g_env, 
      "java/io/BufferedReader", 
      "readLine", 
      "()Ljava/lang/String;", 
      false);

  if(GET_HOOK_NAME_BY_IDX(println) == NULL) return NULL;
  if(GET_HOOK_NAME_BY_IDX(readLine) == NULL) return NULL;

  printf("[+] method 1 @ %p\n", GET_HOOK_NAME_BY_IDX(println));
  printf("[+] method 2 @ %p\n", GET_HOOK_NAME_BY_IDX(readLine));

  printf("[+] resolved Method* for println  @ %p\n", resolve_jmethod_id(GET_HOOK_NAME_BY_IDX(println)));
  printf("[+] resolved Method* for readLine @ %p\n", resolve_jmethod_id(GET_HOOK_NAME_BY_IDX(readLine)));

  SET_HOOK(println);
  SET_HOOK(readLine);

  jstring result = (*g_env)->NewStringUTF(g_env, "Hello world123");
  printf("result @ %p\n", result);

  return NULL;
}

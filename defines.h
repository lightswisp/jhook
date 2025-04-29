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
#include <stdint.h>

enum __MethodKind {
  zerolocals,                                                 // method needs locals initialization
  zerolocals_synchronized,                                    // method needs locals initialization & is synchronized
  native,                                                     // native method
  native_synchronized,                                        // native method & is synchronized
  empty,                                                      // empty method (code: _return)
  getter,                                                     // getter method
  setter,                                                     // setter method
  abstract,                                                   // abstract method (throws an AbstractMethodException)
  method_handle_invoke_FIRST,                                 // java.lang.invoke.MethodHandles::invokeExact, etc.
  method_handle_invoke_LAST                                   = 14,
  java_lang_math_sin,                                         // implementation of java.lang.Math.sin   (x)
  java_lang_math_cos,                                         // implementation of java.lang.Math.cos   (x)
  java_lang_math_tan,                                         // implementation of java.lang.Math.tan   (x)
  java_lang_math_tanh,                                        // implementation of java.lang.Math.tanh  (x)
  java_lang_math_abs,                                         // implementation of java.lang.Math.abs   (x)
  java_lang_math_sqrt,                                        // implementation of java.lang.Math.sqrt  (x)
  java_lang_math_sqrt_strict,                                 // implementation of java.lang.StrictMath.sqrt(x)
  java_lang_math_log,                                         // implementation of java.lang.Math.log   (x)
  java_lang_math_log10,                                       // implementation of java.lang.Math.log10 (x)
  java_lang_math_pow,                                         // implementation of java.lang.Math.pow   (x,y)
  java_lang_math_exp,                                         // implementation of java.lang.Math.exp   (x)
  java_lang_math_fmaF,                                        // implementation of java.lang.Math.fma   (x, y, z)
  java_lang_math_fmaD,                                        // implementation of java.lang.Math.fma   (x, y, z)
  java_lang_ref_reference_get,                                // implementation of java.lang.ref.Reference.get()
  java_util_zip_CRC32_update,                                 // implementation of java.util.zip.CRC32.update()
  java_util_zip_CRC32_updateBytes,                            // implementation of java.util.zip.CRC32.updateBytes()
  java_util_zip_CRC32_updateByteBuffer,                       // implementation of java.util.zip.CRC32.updateByteBuffer()
  java_util_zip_CRC32C_updateBytes,                           // implementation of java.util.zip.CRC32C.updateBytes(crc, b[], off, end)
  java_util_zip_CRC32C_updateDirectByteBuffer,                // implementation of java.util.zip.CRC32C.updateDirectByteBuffer(crc, address, off, end)
  java_lang_Float_intBitsToFloat,                             // implementation of java.lang.Float.intBitsToFloat()
  java_lang_Float_floatToRawIntBits,                          // implementation of java.lang.Float.floatToRawIntBits()
  java_lang_Float_float16ToFloat,                             // implementation of java.lang.Float.float16ToFloat()
  java_lang_Float_floatToFloat16,                             // implementation of java.lang.Float.floatToFloat16()
  java_lang_Double_longBitsToDouble,                          // implementation of java.lang.Double.longBitsToDouble()
  java_lang_Double_doubleToRawLongBits,                       // implementation of java.lang.Double.doubleToRawLongBits()
  java_lang_Thread_currentThread,                             // implementation of java.lang.Thread.currentThread()
  number_of_method_entries,
  invalid = -1
};

enum {
  _imcp_invoke_name = 1,        // utf8: 'invokeExact', etc.
  _imcp_invoke_signature,       // utf8: (variable Symbol*)
  _imcp_limit
};

#define InlineTableSizes_size 44
#define GrowableArray_size    56

/* typedefs */
typedef uint64_t*      __Method;
typedef uint64_t*      __Symbol;
typedef uint64_t*      __JavaThread;
typedef uint64_t*      __ConstMethod;
typedef unsigned short __AccessFlags;
typedef uint64_t*      __InstanceKlass;
typedef uint64_t*      __ClassLoaderData;
typedef uint64_t*      __InlineTableSizes;
typedef uint64_t*      __GrowableArray;
typedef uint64_t*      __ConstantPool;

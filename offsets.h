/* just a quick note:
 * for better portability and support of various java versions, 
 * please consider adding signature scan in the future 
 */
#pragma once
/* field offsets */
#define fi_entry_off                      0x0000000d
#define i2i_entry_off                     0x0000000a
#define cmethod_off                       0x00000002
#define arg_sz_off                        0x0000002e
#define string_sz_off                     0x00000024
#define string_ptr_off                    0x00000028

/* function offsets */
#define remove_unshareable_info_off       0x014c3480
#define size_of_parameters_off            0x00426af4
#define method_holder_off                 0x00426b16
#define class_loader_data_off             0x004202e4
#define new_permanent_symbol_off          0x019545c2
#define access_flags_from_off             0x00b9e1f9
#define inline_table_sizes_off            0x00a45f42
#define const_method_off                  0x00423522
#define method_allocate_off               0x014c192c
#define java_thread_current_off           0x00505fc3
#define checked_exception_length_off      0x00b0f46c
#define localvariable_table_length_off    0x00b0f546
#define exception_table_length_off        0x00b0f62a
#define method_parameters_length_off      0x00b0f3ea
#define method_annotations_length_off     0x014cbdd6
#define generic_signature_index_off       0x01239352
#define parameter_annotations_length_off  0x014cbe10
#define type_annotations_length_off       0x014cbe4a
#define default_annotations_length_off    0x014cbe84
#define growable_array_init_off           0x00fbcc30
#define growable_array_push_off           0x00b9febc
#define constant_pool_allocate_off        0x00b11b6c
#define instance_klass_constants_off      0x006e2b72
#define constant_pool_copy_fields_off     0x00b11c1c
#define constant_pool_set_pool_holder_off 0x0080cc5a
#define constant_pool_symbol_at_put_off   0x0080cd62
#define method_set_constants_off          0x00a4803c
#define method_set_name_index_off         0x00a47fe0
#define method_set_signature_index_off    0x00a4800e
#define const_method_compute_from_sig_off 0x00b0e786
#define method_set_interpreter_entry_off  0x014cc1c6
#define interpreter_entry_for_kind_off    0x004248bb
#define merge_in_new_methods_off          0x00b9daa1
#define method_from_interpreted_entry_off 0x00fa2126
#define method_set_native_function_off    0x014c5674

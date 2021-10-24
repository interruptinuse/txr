/* Copyright 2017-2021
 * Kaz Kylheku <kaz@kylheku.com>
 * Vancouver, Canada
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

extern val uint8_s, int8_s;
extern val uint16_s, int16_s;
extern val uint32_s, int32_s;
extern val uint64_s, int64_s;

extern val char_s, uchar_s, bchar_s, wchar_s;
extern val short_s, ushort_s;
extern val int_s, uint_s;
extern val long_s, ulong_s;
extern val void_s;
extern val double_s;

extern val val_s;

extern val be_uint16_s, be_int16_s;
extern val be_uint32_s, be_int32_s;
extern val be_uint64_s, be_int64_s;
extern val be_float_s, be_double_s;

extern val le_uint16_s, le_int16_s;
extern val le_uint32_s, le_int32_s;
extern val le_uint64_s, le_int64_s;
extern val le_float_s, le_double_s;

extern val array_s, zarray_s, carray_s;

extern val struct_s, union_s;

extern val str_d_s, wstr_s, wstr_d_s, bstr_s, bstr_d_s;

extern val buf_d_s;

extern val ptr_in_s, ptr_out_s, ptr_in_d_s, ptr_out_d_s, ptr_out_s_s, ptr_s;

extern val closure_s;

extern val sbit_s, ubit_s, bit_s;

extern val enum_s, enumed_s;

extern val align_s;

extern val bool_s;

extern val ffi_type_s, ffi_call_desc_s, ffi_closure_s;

extern struct cobj_class *carray_cls;

val ffi_type_compile(val syntax);
val ffi_type_operator_p(val sym);
val ffi_type_p(val sym);
val ffi_make_call_desc(val ntotal, val nfixed, val rettype, val argtypes,
                       val name);
val ffi_make_closure(val fun, val call_desc, val safe_p_in, val abort_ret_in);
mem_t *ffi_closure_get_fptr(val self, val closure);
val ffi_call_wrap(val fptr, val ffi_call_desc, struct args *args);
val ffi_typedef(val name, val type);
val ffi_size(val type);
val ffi_alignof(val type);
val ffi_offsetof(val type, val memb);
val ffi_arraysize(val type);
val ffi_elemsize(val type);
val ffi_elemtype(val type);
val ffi_put_into(val dstbuf, val obj, val type, val offset);
val ffi_put(val obj, val type);
val ffi_in(val srcbuf, val obj, val type, val copy_p, val offset);
val ffi_get(val srcbuf, val type, val offset);
val ffi_out(val dstbuf, val obj, val type, val copy_p, val offset);
val make_carray(val type, mem_t *data, cnum nelem, val ref, cnum offs);
val carrayp(val obj);
val carray_set_length(val carray, val nelem);
val carray_dup(val carray);
val carray_own(val carray);
val carray_free(val carray);
val carray_type(val carray);
val length_carray(val carray);
val copy_carray(val carray);
mem_t *carray_ptr(val carray, val type, val self);
void carray_set_ptr(val carray, val type, mem_t *ptr, val self);
val carray_vec(val vec, val type, val null_term_p);
val carray_list(val list, val type, val null_term_p);
val carray_blank(val nelem, val type);
val carray_buf(val buf, val type, val offs);
val carray_buf_sync(val carray);
val buf_carray(val carray);
val carray_cptr(val cptr, val type, val len);
val cptr_carray(val carray, val type_sym_in);
val vec_carray(val carray, val null_term_p);
val list_carray(val carray, val null_term_p);
val carray_ref(val carray, val idx);
val carray_refset(val carray, val idx, val newval);
val buf_carray(val carray);
val carray_sub(val carray, val from, val to);
val carray_replace(val carray, val values, val from, val to);
val carray_get(val carray);
val carray_getz(val carray);
val carray_put(val array, val seq);
val carray_putz(val array, val seq);
val carray_pun(val carray, val type, val offset_in, val lim_in);
val carray_uint(val num, val type);
val carray_int(val num, val type);
val uint_carray(val carray);
val int_carray(val carray);
val put_carray(val carray, val offs, val stream);
val fill_carray(val carray, val offs, val stream);
#if HAVE_MMAP
val mmap_wrap(val type, val len, val prot, val flags,
              val source_opt, val offset_opt, val addr_opt);
val munmap_wrap(val carray);
val mprotect_wrap(val carray, val prot, val offset, val size);
val madvise_wrap(val carray, val advice, val offset, val size);
val msync_wrap(val carray, val flags, val offset, val size);
#endif
mem_t *union_get_ptr(val self, val uni);
val make_union(val type, val init, val memb);
val union_members(val uni);
val union_get(val uni, val memb);
val union_put(val uni, val memb, val newval);
val union_in(val uni, val memb, val memb_obj);
val union_out(val uni, val memb, val memb_obj);
val make_zstruct(val type, struct args *args);
val zero_fill(val type, val obj);
val put_obj(val obj, val type, val stream);
val get_obj(val type, val stream);
val fill_obj(val obj, val type, val stream);
void ffi_init(void);
void ffi_compat_fixup(int compat_ver);

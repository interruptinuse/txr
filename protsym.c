/* This file is generated by genprotsym.txr */

/* Copyright 2009-2017
 * Kaz Kylheku <kaz@kylheku.com>
 * Vancouver, Canada
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include "config.h"
#include "lib.h"

extern val accept_s, addr_k, align_s, all_s, alloc_error_s;
extern val and_s, ap_s, apf_s, append_each_s, append_each_star_s;
extern val append_k, append_s, apply_s, aret_s, args_k;
extern val array_s, assert_s, atime_k, atime_s, atom_s;
extern val auto_k, bchar_s, be_double_s, be_float_s, be_int16_s;
extern val be_int32_s, be_int64_s, be_uint16_s, be_uint32_s, be_uint64_s;
extern val bignum_s, bind_s, bit_s, blksize_k, blksize_s;
extern val block_s, block_star_s, blocks_k, blocks_s, bool_s;
extern val bstr_d_s, bstr_s, buf_d_s, buf_s, byte_oriented_k;
extern val call_s, car_s, carray_s, caseq_s, caseq_star_s;
extern val caseql_s, caseql_star_s, casequal_s, casequal_star_s, cases_s;
extern val cat_s, catch_s, cdigit_k, cdr_s, chain_s;
extern val chand_s, char_s, chars_k, choose_s, chr_s;
extern val chset_s, circref_s, clear_error_s, close_s, closure_s;
extern val cobj_s, coll_s, collect_each_s, collect_each_star_s, collect_s;
extern val colon_k, compl_s, compound_s, cond_s, cons_s;
extern val continue_k, continue_s, counter_k, cptr_s, cset_s;
extern val cspace_k, ctime_k, ctime_s, cword_char_k, data_s;
extern val day_s, decline_k, defex_s, deffilter_s, define_s;
extern val defmacro_s, defparm_s, defparml_s, defr_warning_s, defsymacro_s;
extern val defun_s, defvar_s, defvarl_s, dev_k, dev_s;
extern val digit_k, do_s, dohash_s, double_s, downcase_k;
extern val dst_s, dvbind_s, dwim_s, each_op_s, each_s;
extern val each_star_s, empty_s, enum_s, enumed_s, env_k;
extern val env_s, eof_s, eol_s, eq_s, eql_based_k;
extern val eql_s, equal_based_k, equal_s, error_s, eval_error_s;
extern val expr_s, fail_s, fbind_s, fd_k, ffi_call_desc_s;
extern val ffi_closure_s, ffi_type_s, file_error_s, fill_buf_s, filter_k;
extern val filter_s, filters_s, finally_s, finish_k, first_s;
extern val fixnum_s, flatten_s, flet_s, float_s, flush_s;
extern val for_op_s, for_s, for_star_s, force_s, forget_s;
extern val form_k, format_s, freeform_s, from_current_k, from_end_k;
extern val from_list_s, from_start_k, frombase64_k, fromhtml_k, frompercent_k;
extern val fromurl_k, fun_k, fun_s, fuzz_s, gap_k;
extern val gather_s, gen_s, generate_s, gensym_counter_s, get_byte_s;
extern val get_char_s, get_error_s, get_error_str_s, get_fd_s, get_line_s;
extern val get_prop_s, gid_k, gid_s, gmtoff_s, greedy_k;
extern val gun_s, handler_bind_s, hash_construct_s, hash_iter_s, hash_lit_s;
extern val hash_s, hextoint_k, hour_s, iapply_s, identity_s;
extern val if_s, iflet_s, in_package_s, inc_s, include_s;
extern val init_k, ino_k, ino_s, int16_s, int32_s;
extern val int64_s, int8_s, int_s, integer_s, internal_error_s;
extern val into_k, intr_s, ipf_s, keyword_package_s, labels_s;
extern val lambda_s, lambda_set_s, last_s, lbind_s, lcons_s;
extern val le_double_s, le_float_s, le_int16_s, le_int32_s, le_int64_s;
extern val le_uint16_s, le_uint32_s, le_uint64_s, length_s, let_s;
extern val let_star_s, lfilt_k, line_s, lines_k, lisp1_setq_s;
extern val list_k, list_s, list_star_s, listener_hist_len_s, listener_multi_line_p_s;
extern val listener_sel_inclusive_p_s, lists_k, lit_s, load_path_s, load_recursive_s;
extern val load_s, local_s, long_s, longest_k, lstr_s;
extern val mac_param_bind_s, macro_s, macro_time_s, macrolet_s, make_struct_lit_s;
extern val mandatory_k, maxgap_k, maxtimes_k, maybe_s, mdo_s;
extern val memq_s, memql_s, memqual_s, merge_s, meth_s;
extern val min_s, mingap_k, mintimes_k, mod_s, mode_k;
extern val mode_s, modlast_s, month_s, mtime_k, mtime_s;
extern val name_k, name_s, named_k, next_s, next_spec_k;
extern val nlink_k, nlink_s, none_s, nongreedy_s, not_s;
extern val nothrow_k, noval_s, null_s, nullify_s, number_s;
extern val numeric_error_s, oand_s, oneplus_s, op_s, opip_s;
extern val optional_s, or_s, output_s, package_alist_s, package_s;
extern val panic_s, parser_s, path_s, pkg_s, plus_s;
extern val postinit_k, pprint_flo_format_s, print_base_s, print_circle_s, print_flo_digits_s;
extern val print_flo_format_s, print_flo_precision_s, print_s, process_error_s, prof_s;
extern val prog1_s, progn_s, promise_forced_s, promise_inprogress_s, promise_s;
extern val ptr_in_d_s, ptr_in_s, ptr_out_d_s, ptr_out_s, ptr_out_s_s;
extern val ptr_s, put_buf_s, put_byte_s, put_char_s, put_string_s;
extern val qquote_s, qref_s, quasi_s, quasilist_s, query_error_s;
extern val quote_s, random_state_s, random_state_var_s, random_warmup_s, range_error_s;
extern val range_s, rcons_s, rdev_k, rdev_s, real_time_k;
extern val rebind_s, reflect_k, regex_s, rep_s, repeat_s;
extern val repeat_spec_k, require_s, resolve_k, rest_s, restart_s;
extern val ret_s, return_from_s, return_s, rfilt_k, sbit_s;
extern val sec_s, seek_s, sequence_s, set_prop_s, set_s;
extern val setq_s, setqf_s, short_s, shortest_k, single_s;
extern val size_k, size_s, skip_s, slot_s, some_s;
extern val space_k, special_s, splice_s, stat_s, static_slot_s;
extern val stddebug_s, stderr_s, stdin_s, stdio_stream_s, stdnull_s;
extern val stdout_s, str_d_s, str_s, stream_s, string_k;
extern val string_s, struct_lit_s, struct_s, struct_type_s, switch_s;
extern val sym_s, symacro_k, symacrolet_s, syntax_error_s, sys_abscond_from_s;
extern val sys_apply_s, sys_catch_s, sys_lisp1_value_s, sys_mark_special_s, sys_qquote_s;
extern val sys_splice_s, sys_unquote_s, system_error_s, system_package_s, text_s;
extern val throw_s, time_local_s, time_parse_s, time_s, time_string_s;
extern val time_utc_s, timeout_error_s, times_k, tlist_k, tobase64_k;
extern val tofloat_k, tohtml_k, tohtml_star_k, toint_k, tonumber_k;
extern val topercent_k, tourl_k, trailer_s, tree_bind_s, tree_case_s;
extern val truncate_s, try_s, type_error_s, ubit_s, uchar_s;
extern val uid_k, uid_s, uint16_s, uint32_s, uint64_s;
extern val uint8_s, uint_s, ulong_s, unbound_s, unget_byte_s;
extern val unget_char_s, union_s, unique_s, unquote_s, until_s;
extern val until_star_s, upcase_k, uref_s, user_package_s, userdata_k;
extern val ushort_s, uw_protect_s, val_s, var_k, var_s;
extern val vars_k, vec_list_s, vec_s, vecref_s, vector_lit_s;
extern val void_s, warning_s, wchar_s, weak_keys_k, weak_vals_k;
extern val when_s, while_s, while_star_s, whole_k, wild_s;
extern val with_dyn_rebinds_s, word_char_k, wrap_k, wstr_d_s, wstr_s;
extern val year_s, zap_s, zarray_s, zeroplus_s, zone_s;

#if CONFIG_DEBUG_SUPPORT
extern val debug_quit_s;
#endif
#if HAVE_DLOPEN
extern val dlhandle_s, dlsym_s;
#endif
#if HAVE_GRGID
extern val group_s, mem_s;
#endif
#if HAVE_PWUID
extern val dir_s, gecos_s, passwd_s, shell_s;
#endif
#if HAVE_SOCKETS
extern val addr_s, addrinfo_s, canonname_s, family_s, flags_s;
extern val flow_info_s, port_s, protocol_s, scope_id_s, sockaddr_in6_s;
extern val sockaddr_in_s, sockaddr_un_s, socket_error_s, socktype_s;
#endif
#if HAVE_SYSLOG
extern val prio_k;
#endif
#if HAVE_TERMIOS
extern val cc_s, cflag_s, iflag_s, ispeed_s, lflag_s;
extern val oflag_s, ospeed_s, termios_s;
#endif
#if HAVE_UNAME
extern val domainname_s, machine_s, nodename_s, release_s, sysname_s;
extern val utsname_s, version_s;
#endif

val *protected_sym[] = {
  &accept_s, &addr_k, &align_s, &all_s, &alloc_error_s,
  &and_s, &ap_s, &apf_s, &append_each_s, &append_each_star_s,
  &append_k, &append_s, &apply_s, &aret_s, &args_k,
  &array_s, &assert_s, &atime_k, &atime_s, &atom_s,
  &auto_k, &bchar_s, &be_double_s, &be_float_s, &be_int16_s,
  &be_int32_s, &be_int64_s, &be_uint16_s, &be_uint32_s, &be_uint64_s,
  &bignum_s, &bind_s, &bit_s, &blksize_k, &blksize_s,
  &block_s, &block_star_s, &blocks_k, &blocks_s, &bool_s,
  &bstr_d_s, &bstr_s, &buf_d_s, &buf_s, &byte_oriented_k,
  &call_s, &car_s, &carray_s, &caseq_s, &caseq_star_s,
  &caseql_s, &caseql_star_s, &casequal_s, &casequal_star_s, &cases_s,
  &cat_s, &catch_s, &cdigit_k, &cdr_s, &chain_s,
  &chand_s, &char_s, &chars_k, &choose_s, &chr_s,
  &chset_s, &circref_s, &clear_error_s, &close_s, &closure_s,
  &cobj_s, &coll_s, &collect_each_s, &collect_each_star_s, &collect_s,
  &colon_k, &compl_s, &compound_s, &cond_s, &cons_s,
  &continue_k, &continue_s, &counter_k, &cptr_s, &cset_s,
  &cspace_k, &ctime_k, &ctime_s, &cword_char_k, &data_s,
  &day_s, &decline_k, &defex_s, &deffilter_s, &define_s,
  &defmacro_s, &defparm_s, &defparml_s, &defr_warning_s, &defsymacro_s,
  &defun_s, &defvar_s, &defvarl_s, &dev_k, &dev_s,
  &digit_k, &do_s, &dohash_s, &double_s, &downcase_k,
  &dst_s, &dvbind_s, &dwim_s, &each_op_s, &each_s,
  &each_star_s, &empty_s, &enum_s, &enumed_s, &env_k,
  &env_s, &eof_s, &eol_s, &eq_s, &eql_based_k,
  &eql_s, &equal_based_k, &equal_s, &error_s, &eval_error_s,
  &expr_s, &fail_s, &fbind_s, &fd_k, &ffi_call_desc_s,
  &ffi_closure_s, &ffi_type_s, &file_error_s, &fill_buf_s, &filter_k,
  &filter_s, &filters_s, &finally_s, &finish_k, &first_s,
  &fixnum_s, &flatten_s, &flet_s, &float_s, &flush_s,
  &for_op_s, &for_s, &for_star_s, &force_s, &forget_s,
  &form_k, &format_s, &freeform_s, &from_current_k, &from_end_k,
  &from_list_s, &from_start_k, &frombase64_k, &fromhtml_k, &frompercent_k,
  &fromurl_k, &fun_k, &fun_s, &fuzz_s, &gap_k,
  &gather_s, &gen_s, &generate_s, &gensym_counter_s, &get_byte_s,
  &get_char_s, &get_error_s, &get_error_str_s, &get_fd_s, &get_line_s,
  &get_prop_s, &gid_k, &gid_s, &gmtoff_s, &greedy_k,
  &gun_s, &handler_bind_s, &hash_construct_s, &hash_iter_s, &hash_lit_s,
  &hash_s, &hextoint_k, &hour_s, &iapply_s, &identity_s,
  &if_s, &iflet_s, &in_package_s, &inc_s, &include_s,
  &init_k, &ino_k, &ino_s, &int16_s, &int32_s,
  &int64_s, &int8_s, &int_s, &integer_s, &internal_error_s,
  &into_k, &intr_s, &ipf_s, &keyword_package_s, &labels_s,
  &lambda_s, &lambda_set_s, &last_s, &lbind_s, &lcons_s,
  &le_double_s, &le_float_s, &le_int16_s, &le_int32_s, &le_int64_s,
  &le_uint16_s, &le_uint32_s, &le_uint64_s, &length_s, &let_s,
  &let_star_s, &lfilt_k, &line_s, &lines_k, &lisp1_setq_s,
  &list_k, &list_s, &list_star_s, &listener_hist_len_s, &listener_multi_line_p_s,
  &listener_sel_inclusive_p_s, &lists_k, &lit_s, &load_path_s, &load_recursive_s,
  &load_s, &local_s, &long_s, &longest_k, &lstr_s,
  &mac_param_bind_s, &macro_s, &macro_time_s, &macrolet_s, &make_struct_lit_s,
  &mandatory_k, &maxgap_k, &maxtimes_k, &maybe_s, &mdo_s,
  &memq_s, &memql_s, &memqual_s, &merge_s, &meth_s,
  &min_s, &mingap_k, &mintimes_k, &mod_s, &mode_k,
  &mode_s, &modlast_s, &month_s, &mtime_k, &mtime_s,
  &name_k, &name_s, &named_k, &next_s, &next_spec_k,
  &nlink_k, &nlink_s, &none_s, &nongreedy_s, &not_s,
  &nothrow_k, &noval_s, &null_s, &nullify_s, &number_s,
  &numeric_error_s, &oand_s, &oneplus_s, &op_s, &opip_s,
  &optional_s, &or_s, &output_s, &package_alist_s, &package_s,
  &panic_s, &parser_s, &path_s, &pkg_s, &plus_s,
  &postinit_k, &pprint_flo_format_s, &print_base_s, &print_circle_s, &print_flo_digits_s,
  &print_flo_format_s, &print_flo_precision_s, &print_s, &process_error_s, &prof_s,
  &prog1_s, &progn_s, &promise_forced_s, &promise_inprogress_s, &promise_s,
  &ptr_in_d_s, &ptr_in_s, &ptr_out_d_s, &ptr_out_s, &ptr_out_s_s,
  &ptr_s, &put_buf_s, &put_byte_s, &put_char_s, &put_string_s,
  &qquote_s, &qref_s, &quasi_s, &quasilist_s, &query_error_s,
  &quote_s, &random_state_s, &random_state_var_s, &random_warmup_s, &range_error_s,
  &range_s, &rcons_s, &rdev_k, &rdev_s, &real_time_k,
  &rebind_s, &reflect_k, &regex_s, &rep_s, &repeat_s,
  &repeat_spec_k, &require_s, &resolve_k, &rest_s, &restart_s,
  &ret_s, &return_from_s, &return_s, &rfilt_k, &sbit_s,
  &sec_s, &seek_s, &sequence_s, &set_prop_s, &set_s,
  &setq_s, &setqf_s, &short_s, &shortest_k, &single_s,
  &size_k, &size_s, &skip_s, &slot_s, &some_s,
  &space_k, &special_s, &splice_s, &stat_s, &static_slot_s,
  &stddebug_s, &stderr_s, &stdin_s, &stdio_stream_s, &stdnull_s,
  &stdout_s, &str_d_s, &str_s, &stream_s, &string_k,
  &string_s, &struct_lit_s, &struct_s, &struct_type_s, &switch_s,
  &sym_s, &symacro_k, &symacrolet_s, &syntax_error_s, &sys_abscond_from_s,
  &sys_apply_s, &sys_catch_s, &sys_lisp1_value_s, &sys_mark_special_s, &sys_qquote_s,
  &sys_splice_s, &sys_unquote_s, &system_error_s, &system_package_s, &text_s,
  &throw_s, &time_local_s, &time_parse_s, &time_s, &time_string_s,
  &time_utc_s, &timeout_error_s, &times_k, &tlist_k, &tobase64_k,
  &tofloat_k, &tohtml_k, &tohtml_star_k, &toint_k, &tonumber_k,
  &topercent_k, &tourl_k, &trailer_s, &tree_bind_s, &tree_case_s,
  &truncate_s, &try_s, &type_error_s, &ubit_s, &uchar_s,
  &uid_k, &uid_s, &uint16_s, &uint32_s, &uint64_s,
  &uint8_s, &uint_s, &ulong_s, &unbound_s, &unget_byte_s,
  &unget_char_s, &union_s, &unique_s, &unquote_s, &until_s,
  &until_star_s, &upcase_k, &uref_s, &user_package_s, &userdata_k,
  &ushort_s, &uw_protect_s, &val_s, &var_k, &var_s,
  &vars_k, &vec_list_s, &vec_s, &vecref_s, &vector_lit_s,
  &void_s, &warning_s, &wchar_s, &weak_keys_k, &weak_vals_k,
  &when_s, &while_s, &while_star_s, &whole_k, &wild_s,
  &with_dyn_rebinds_s, &word_char_k, &wrap_k, &wstr_d_s, &wstr_s,
  &year_s, &zap_s, &zarray_s, &zeroplus_s, &zone_s,

#if CONFIG_DEBUG_SUPPORT
  &debug_quit_s,
#endif
#if HAVE_DLOPEN
  &dlhandle_s, &dlsym_s,
#endif
#if HAVE_GRGID
  &group_s, &mem_s,
#endif
#if HAVE_PWUID
  &dir_s, &gecos_s, &passwd_s, &shell_s,
#endif
#if HAVE_SOCKETS
  &addr_s, &addrinfo_s, &canonname_s, &family_s, &flags_s,
  &flow_info_s, &port_s, &protocol_s, &scope_id_s, &sockaddr_in6_s,
  &sockaddr_in_s, &sockaddr_un_s, &socket_error_s, &socktype_s,
#endif
#if HAVE_SYSLOG
  &prio_k,
#endif
#if HAVE_TERMIOS
  &cc_s, &cflag_s, &iflag_s, &ispeed_s, &lflag_s,
  &oflag_s, &ospeed_s, &termios_s,
#endif
#if HAVE_UNAME
  &domainname_s, &machine_s, &nodename_s, &release_s, &sysname_s,
  &utsname_s, &version_s,
#endif
  convert(val *, 0)
};

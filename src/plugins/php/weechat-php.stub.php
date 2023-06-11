<?php

/** @generate-legacy-arginfo */

// Keep these function stubs in sync with the API. This ensures
// `--enable-debug` builds of PHP work. After updating this file, run
//
// ```
// cd src/plugins/php && ~/php-src/build/gen_stub.php
// ```
//
// to regenerate `weechat-php_*arginfo.h` files.

function weechat_register(string $p0, string $p1, string $p2, string $p3, string $p4, mixed $p5, string $p6): int {}
function weechat_plugin_get_name(string $p0): string {}
function weechat_charset_set(string $p0): int {}
function weechat_iconv_to_internal(string $p0, string $p1): string {}
function weechat_iconv_from_internal(string $p0, string $p1): string {}
function weechat_gettext(string $p0): string {}
function weechat_ngettext(string $p0, string $p1, int $p2): string {}
function weechat_strlen_screen(string $p0): int {}
function weechat_string_match(string $p0, string $p1, int $p2): int {}
function weechat_string_match_list(string $p0, string $p1, int $p2): int {}
function weechat_string_has_highlight(string $p0, string $p1): int {}
function weechat_string_has_highlight_regex(string $p0, string $p1): int {}
function weechat_string_mask_to_regex(string $p0): string {}
function weechat_string_format_size(int $p0): string {}
function weechat_string_parse_size(string $p0): int {}
function weechat_string_color_code_size(string $p0): int {}
function weechat_string_remove_color(string $p0, string $p1): string {}
function weechat_string_is_command_char(string $p0): int {}
function weechat_string_input_for_buffer(string $p0): string {}
function weechat_string_eval_expression(string $p0, array $p1, array $p2, array $p3): string {}
function weechat_string_eval_path_home(string $p0, array $p1, array $p2, array $p3): string {}
function weechat_mkdir_home(string $p0, int $p1): int {}
function weechat_mkdir(string $p0, int $p1): int {}
function weechat_mkdir_parents(string $p0, int $p1): int {}
function weechat_list_new(): string {}
function weechat_list_add(string $p0, string $p1, string $p2, string $p3): string {}
function weechat_list_search(string $p0, string $p1): string {}
function weechat_list_search_pos(string $p0, string $p1): int {}
function weechat_list_casesearch(string $p0, string $p1): string {}
function weechat_list_casesearch_pos(string $p0, string $p1): int {}
function weechat_list_get(string $p0, int $p1): string {}
function weechat_list_set(string $p0, string $p1): int {}
function weechat_list_next(string $p0): string {}
function weechat_list_prev(string $p0): string {}
function weechat_list_string(string $p0): string {}
function weechat_list_size(string $p0): int {}
function weechat_list_remove(string $p0, string $p1): int {}
function weechat_list_remove_all(string $p0): int {}
function weechat_list_free(string $p0): int {}
function weechat_config_new(string $p0, mixed $p1, string $p2): string {}
function weechat_config_set_version(string $p0, int $p1, mixed $p2, string $p3): int {}
function weechat_config_new_section(): string {}
function weechat_config_search_section(string $p0, string $p1): string {}
function weechat_config_new_option(): string {}
function weechat_config_search_option(string $p0, string $p1, string $p2): string {}
function weechat_config_string_to_boolean(string $p0): int {}
function weechat_config_option_reset(string $p0, int $p1): int {}
function weechat_config_option_set(string $p0, string $p1, int $p2): int {}
function weechat_config_option_set_null(string $p0, int $p1): int {}
function weechat_config_option_unset(string $p0): int {}
function weechat_config_option_rename(string $p0, string $p1): int {}
function weechat_config_option_is_null(string $p0): int {}
function weechat_config_option_default_is_null(string $p0): int {}
function weechat_config_boolean(string $p0): int {}
function weechat_config_boolean_default(string $p0): int {}
function weechat_config_integer(string $p0): int {}
function weechat_config_integer_default(string $p0): int {}
function weechat_config_string(string $p0): string {}
function weechat_config_string_default(string $p0): string {}
function weechat_config_color(string $p0): string {}
function weechat_config_color_default(string $p0): string {}
function weechat_config_write_option(string $p0, string $p1): int {}
function weechat_config_write_line(string $p0, string $p1, string $p2): int {}
function weechat_config_write(string $p0): int {}
function weechat_config_read(string $p0): int {}
function weechat_config_reload(string $p0): int {}
function weechat_config_option_free(string $p0): int {}
function weechat_config_section_free_options(string $p0): int {}
function weechat_config_section_free(string $p0): int {}
function weechat_config_free(string $p0): int {}
function weechat_config_get(string $p0): string {}
function weechat_config_get_plugin(string $p0): string {}
function weechat_config_is_set_plugin(string $p0): int {}
function weechat_config_set_plugin(string $p0, string $p1): int {}
function weechat_config_set_desc_plugin(string $p0, string $p1): int {}
function weechat_config_unset_plugin(string $p0): int {}
function weechat_key_bind(string $p0, array $p1): int {}
function weechat_key_unbind(string $p0, string $p1): int {}
function weechat_prefix(string $p0): string {}
function weechat_color(string $p0): string {}
function weechat_print(string $p0, string $p1): int {}
function weechat_print_date_tags(string $p0, int $p1, string $p2, string $p3): int {}
function weechat_print_y(string $p0, int $p1, string $p2): int {}
function weechat_print_y_date_tags(string $p0, int $p1, int $p2, string $p3, string $p4): int {}
function weechat_log_print(string $p0): int {}
function weechat_hook_command(string $p0, string $p1, string $p2, string $p3, string $p4, mixed $p5, string $p6): string {}
function weechat_hook_completion(string $p0, string $p1, mixed $p2, string $p3): string {}
function weechat_hook_completion_get_string(string $p0, string $p1): string {}
function weechat_hook_completion_list_add(string $p0, string $p1, int $p2, string $p3): int {}
function weechat_hook_command_run(string $p0, mixed $p1, string $p2): string {}
function weechat_hook_timer(int $p0, int $p1, int $p2, mixed $p3, string $p4): string {}
function weechat_hook_fd(int $p0, int $p1, int $p2, int $p3, mixed $p4, string $p5): string {}
function weechat_hook_process(string $p0, int $p1, mixed $p2, string $p3): string {}
function weechat_hook_process_hashtable(string $p0, array $p1, int $p2, mixed $p3, string $p4): string {}
function weechat_hook_connect(): string {}
function weechat_hook_line(string $p0, string $p1, string $p2, mixed $p3, string $p4): string {}
function weechat_hook_print(string $p0, string $p1, string $p2, int $p3, mixed $p4, string $p5): string {}
function weechat_hook_signal(string $p0, mixed $p1, string $p2): string {}
function weechat_hook_signal_send(string $p0, string $p1, string $p2): int {}
function weechat_hook_hsignal(string $p0, mixed $p1, string $p2): string {}
function weechat_hook_hsignal_send(string $p0, array $p1): int {}
function weechat_hook_config(string $p0, mixed $p1, string $p2): string {}
function weechat_hook_modifier(string $p0, mixed $p1, string $p2): string {}
function weechat_hook_modifier_exec(string $p0, string $p1, string $p2): string {}
function weechat_hook_info(string $p0, string $p1, string $p2, mixed $p3, string $p4): string {}
function weechat_hook_info_hashtable(string $p0, string $p1, string $p2, string $p3, string $p4, mixed $p5, string $p6): string {}
function weechat_hook_infolist(string $p0, string $p1, string $p2, string $p3, mixed $p4, string $p5): string {}
function weechat_hook_focus(string $p0, mixed $p1, string $p2): string {}
function weechat_hook_set(string $p0, string $p1, string $p2): int {}
function weechat_unhook(string $p0): int {}
function weechat_unhook_all(string $p0): int {}
function weechat_buffer_new(string $p0, mixed $p1, string $p2, mixed $p3, string $p4): string {}
function weechat_buffer_new_props(string $p0, array $p1, mixed $p2, string $p3, mixed $p4, string $p5): string {}
function weechat_buffer_search(string $p0, string $p1): string {}
function weechat_buffer_search_main(): string {}
function weechat_current_buffer(): string {}
function weechat_buffer_clear(string $p0): int {}
function weechat_buffer_close(string $p0): int {}
function weechat_buffer_merge(string $p0, string $p1): int {}
function weechat_buffer_unmerge(string $p0, int $p1): int {}
function weechat_buffer_get_integer(string $p0, string $p1): int {}
function weechat_buffer_get_string(string $p0, string $p1): string {}
function weechat_buffer_get_pointer(string $p0, string $p1): string {}
function weechat_buffer_set(string $p0, string $p1, string $p2): int {}
function weechat_buffer_string_replace_local_var(string $p0, string $p1): string {}
function weechat_buffer_match_list(string $p0, string $p1): int {}
function weechat_current_window(): string {}
function weechat_window_search_with_buffer(string $p0): string {}
function weechat_window_get_integer(string $p0, string $p1): int {}
function weechat_window_get_string(string $p0, string $p1): string {}
function weechat_window_get_pointer(string $p0, string $p1): string {}
function weechat_window_set_title(string $p0): int {}
function weechat_nicklist_add_group(string $p0, string $p1, string $p2, string $p3, int $p4): string {}
function weechat_nicklist_search_group(string $p0, string $p1, string $p2): string {}
function weechat_nicklist_add_nick(string $p0, string $p1, string $p2, string $p3, string $p4, string $p5, int $p6): string {}
function weechat_nicklist_search_nick(string $p0, string $p1, string $p2): string {}
function weechat_nicklist_remove_group(string $p0, string $p1): int {}
function weechat_nicklist_remove_nick(string $p0, string $p1): int {}
function weechat_nicklist_remove_all(string $p0): int {}
function weechat_nicklist_group_get_integer(string $p0, string $p1, string $p2): int {}
function weechat_nicklist_group_get_string(string $p0, string $p1, string $p2): string {}
function weechat_nicklist_group_get_pointer(string $p0, string $p1, string $p2): string {}
function weechat_nicklist_group_set(string $p0, string $p1, string $p2, string $p3): int {}
function weechat_nicklist_nick_get_integer(string $p0, string $p1, string $p2): int {}
function weechat_nicklist_nick_get_string(string $p0, string $p1, string $p2): string {}
function weechat_nicklist_nick_get_pointer(string $p0, string $p1, string $p2): string {}
function weechat_nicklist_nick_set(string $p0, string $p1, string $p2, string $p3): int {}
function weechat_bar_item_search(string $p0): string {}
function weechat_bar_item_new(string $p0, mixed $p1, string $p2): string {}
function weechat_bar_item_update(string $p0): int {}
function weechat_bar_item_remove(string $p0): int {}
function weechat_bar_search(string $p0): string {}
function weechat_bar_new(): string {}
function weechat_bar_set(string $p0, string $p1, string $p2): int {}
function weechat_bar_update(string $p0): int {}
function weechat_bar_remove(string $p0): int {}
function weechat_command(string $p0, string $p1): int {}
function weechat_command_options(string $p0, string $p1, array $p2): int {}
function weechat_completion_new(string $p0): string {}
function weechat_completion_search(string $p0, string $p1, int $p2, int $p3): int {}
function weechat_completion_get_string(string $p0, string $p1): string {}
function weechat_completion_list_add(string $p0, string $p1, int $p2, string $p3): int {}
function weechat_completion_free(string $p0): int {}
function weechat_info_get(string $p0, string $p1): string {}
function weechat_info_get_hashtable(string $p0, array $p1): void {}
function weechat_infolist_new(): string {}
function weechat_infolist_new_item(string $p0): string {}
function weechat_infolist_new_var_integer(string $p0, string $p1, int $p2): string {}
function weechat_infolist_new_var_string(string $p0, string $p1, string $p2): string {}
function weechat_infolist_new_var_pointer(string $p0, string $p1, string $p2): string {}
function weechat_infolist_new_var_time(string $p0, string $p1, int $p2): string {}
function weechat_infolist_search_var(string $p0, string $p1): string {}
function weechat_infolist_get(string $p0, string $p1, string $p2): string {}
function weechat_infolist_next(string $p0): int {}
function weechat_infolist_prev(string $p0): int {}
function weechat_infolist_reset_item_cursor(string $p0): int {}
function weechat_infolist_fields(string $p0): string {}
function weechat_infolist_integer(string $p0, string $p1): int {}
function weechat_infolist_string(string $p0, string $p1): string {}
function weechat_infolist_pointer(string $p0, string $p1): string {}
function weechat_infolist_time(string $p0, string $p1): int {}
function weechat_infolist_free(string $p0): int {}
function weechat_hdata_get(string $p0): string {}
function weechat_hdata_get_var_offset(string $p0, string $p1): int {}
function weechat_hdata_get_var_type_string(string $p0, string $p1): string {}
function weechat_hdata_get_var_array_size(string $p0, string $p1, string $p2): int {}
function weechat_hdata_get_var_array_size_string(string $p0, string $p1, string $p2): string {}
function weechat_hdata_get_var_hdata(string $p0, string $p1): string {}
function weechat_hdata_get_list(string $p0, string $p1): string {}
function weechat_hdata_check_pointer(string $p0, string $p1, string $p2): int {}
function weechat_hdata_move(string $p0, string $p1, int $p2): string {}
function weechat_hdata_search(string $p0, string $p1, string $p2, array $p3, array $p4, array $p5, int $p6): string {}
function weechat_hdata_char(string $p0, string $p1, string $p2): int {}
function weechat_hdata_integer(string $p0, string $p1, string $p2): int {}
function weechat_hdata_long(string $p0, string $p1, string $p2): int {}
function weechat_hdata_string(string $p0, string $p1, string $p2): string {}
function weechat_hdata_pointer(string $p0, string $p1, string $p2): string {}
function weechat_hdata_time(string $p0, string $p1, string $p2): int {}
function weechat_hdata_hashtable(string $p0, string $p1, string $p2): void {}
function weechat_hdata_compare(string $p0, string $p1, string $p2, string $p3, int $p4): int {}
function weechat_hdata_update(string $p0, string $p1, array $p2): int {}
function weechat_hdata_get_string(string $p0, string $p1): string {}
function weechat_upgrade_new(string $p0, mixed $p1, string $p2): string {}
function weechat_upgrade_write_object(string $p0, int $p1, string $p2): int {}
function weechat_upgrade_read(string $p0): int {}
function weechat_upgrade_close(string $p0): int {}
function forget_class(string $p0): bool {}
function forget_function(string $p0): bool {}

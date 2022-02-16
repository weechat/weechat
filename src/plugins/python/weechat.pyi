#
# WeeChat Python stub file, auto-generated by python_stub.py.
# DO NOT EDIT BY HAND!
#

from typing import Dict

WEECHAT_RC_OK: int
WEECHAT_RC_ERROR: int
WEECHAT_CONFIG_READ_OK: int
WEECHAT_CONFIG_READ_FILE_NOT_FOUND: int
WEECHAT_CONFIG_WRITE_ERROR: int
WEECHAT_CONFIG_OPTION_SET_OK_CHANGED: int
WEECHAT_CONFIG_OPTION_SET_ERROR: int
WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET: int
WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED: int
WEECHAT_LIST_POS_SORT: str
WEECHAT_LIST_POS_END: str
WEECHAT_HOTLIST_LOW: str
WEECHAT_HOTLIST_PRIVATE: str
WEECHAT_HOOK_PROCESS_RUNNING: int
WEECHAT_HOOK_CONNECT_OK: int
WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND: int
WEECHAT_HOOK_CONNECT_PROXY_ERROR: int
WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR: int
WEECHAT_HOOK_CONNECT_MEMORY_ERROR: int
WEECHAT_HOOK_CONNECT_SOCKET_ERROR: int
WEECHAT_HOOK_SIGNAL_STRING: str
WEECHAT_HOOK_SIGNAL_POINTER: str


def register(name: str, author: str, version: str, license: str, description: str, shutdown_function: str, charset: str) -> int:
    """`register in WeeChat plugin API reference <https://weechat.org/doc/api#_register>`_"""
    ...


def plugin_get_name(plugin: str) -> str:
    """`plugin_get_name in WeeChat plugin API reference <https://weechat.org/doc/api#_plugin_get_name>`_"""
    ...


def charset_set(charset: str) -> int:
    """`charset_set in WeeChat plugin API reference <https://weechat.org/doc/api#_charset_set>`_"""
    ...


def iconv_to_internal(charset: str, string: str) -> str:
    """`iconv_to_internal in WeeChat plugin API reference <https://weechat.org/doc/api#_iconv_to_internal>`_"""
    ...


def iconv_from_internal(charset: str, string: str) -> str:
    """`iconv_from_internal in WeeChat plugin API reference <https://weechat.org/doc/api#_iconv_from_internal>`_"""
    ...


def gettext(string: str) -> str:
    """`gettext in WeeChat plugin API reference <https://weechat.org/doc/api#_gettext>`_"""
    ...


def ngettext(string: str, plural: str, count: int) -> str:
    """`ngettext in WeeChat plugin API reference <https://weechat.org/doc/api#_ngettext>`_"""
    ...


def strlen_screen(string: str) -> int:
    """`strlen_screen in WeeChat plugin API reference <https://weechat.org/doc/api#_strlen_screen>`_"""
    ...


def string_match(string: str, mask: str, case_sensitive: int) -> int:
    """`string_match in WeeChat plugin API reference <https://weechat.org/doc/api#_string_match>`_"""
    ...


def string_match_list(string: str, masks: str, case_sensitive: int) -> int:
    """`string_match_list in WeeChat plugin API reference <https://weechat.org/doc/api#_string_match_list>`_"""
    ...


def string_eval_path_home(path: str, pointers: Dict[str, str], extra_vars: Dict[str, str], options: Dict[str, str]) -> str:
    """`string_eval_path_home in WeeChat plugin API reference <https://weechat.org/doc/api#_string_eval_path_home>`_"""
    ...


def string_mask_to_regex(mask: str) -> str:
    """`string_mask_to_regex in WeeChat plugin API reference <https://weechat.org/doc/api#_string_mask_to_regex>`_"""
    ...


def string_has_highlight(string: str, highlight_words: str) -> int:
    """`string_has_highlight in WeeChat plugin API reference <https://weechat.org/doc/api#_string_has_highlight>`_"""
    ...


def string_has_highlight_regex(string: str, regex: str) -> int:
    """`string_has_highlight_regex in WeeChat plugin API reference <https://weechat.org/doc/api#_string_has_highlight_regex>`_"""
    ...


def string_format_size(size: int) -> str:
    """`string_format_size in WeeChat plugin API reference <https://weechat.org/doc/api#_string_format_size>`_"""
    ...


def string_color_code_size(string: str) -> int:
    """`string_color_code_size in WeeChat plugin API reference <https://weechat.org/doc/api#_string_color_code_size>`_"""
    ...


def string_remove_color(string: str, replacement: str) -> str:
    """`string_remove_color in WeeChat plugin API reference <https://weechat.org/doc/api#_string_remove_color>`_"""
    ...


def string_is_command_char(string: str) -> int:
    """`string_is_command_char in WeeChat plugin API reference <https://weechat.org/doc/api#_string_is_command_char>`_"""
    ...


def string_input_for_buffer(string: str) -> str:
    """`string_input_for_buffer in WeeChat plugin API reference <https://weechat.org/doc/api#_string_input_for_buffer>`_"""
    ...


def string_eval_expression(expr: str, pointers: Dict[str, str], extra_vars: Dict[str, str], options: Dict[str, str]) -> str:
    """`string_eval_expression in WeeChat plugin API reference <https://weechat.org/doc/api#_string_eval_expression>`_"""
    ...


def mkdir_home(directory: str, mode: int) -> int:
    """`mkdir_home in WeeChat plugin API reference <https://weechat.org/doc/api#_mkdir_home>`_"""
    ...


def mkdir(directory: str, mode: int) -> int:
    """`mkdir in WeeChat plugin API reference <https://weechat.org/doc/api#_mkdir>`_"""
    ...


def mkdir_parents(directory: str, mode: int) -> int:
    """`mkdir_parents in WeeChat plugin API reference <https://weechat.org/doc/api#_mkdir_parents>`_"""
    ...


def list_new() -> str:
    """`list_new in WeeChat plugin API reference <https://weechat.org/doc/api#_list_new>`_"""
    ...


def list_add(list: str, data: str, where: str, user_data: str) -> str:
    """`list_add in WeeChat plugin API reference <https://weechat.org/doc/api#_list_add>`_"""
    ...


def list_search(list: str, data: str) -> str:
    """`list_search in WeeChat plugin API reference <https://weechat.org/doc/api#_list_search>`_"""
    ...


def list_search_pos(list: str, data: str) -> int:
    """`list_search_pos in WeeChat plugin API reference <https://weechat.org/doc/api#_list_search_pos>`_"""
    ...


def list_casesearch(list: str, data: str) -> str:
    """`list_casesearch in WeeChat plugin API reference <https://weechat.org/doc/api#_list_casesearch>`_"""
    ...


def list_casesearch_pos(list: str, data: str) -> int:
    """`list_casesearch_pos in WeeChat plugin API reference <https://weechat.org/doc/api#_list_casesearch_pos>`_"""
    ...


def list_get(list: str, position: int) -> str:
    """`list_get in WeeChat plugin API reference <https://weechat.org/doc/api#_list_get>`_"""
    ...


def list_set(item: str, value: str) -> int:
    """`list_set in WeeChat plugin API reference <https://weechat.org/doc/api#_list_set>`_"""
    ...


def list_next(item: str) -> str:
    """`list_next in WeeChat plugin API reference <https://weechat.org/doc/api#_list_next>`_"""
    ...


def list_prev(item: str) -> str:
    """`list_prev in WeeChat plugin API reference <https://weechat.org/doc/api#_list_prev>`_"""
    ...


def list_string(item: str) -> str:
    """`list_string in WeeChat plugin API reference <https://weechat.org/doc/api#_list_string>`_"""
    ...


def list_size(list: str) -> int:
    """`list_size in WeeChat plugin API reference <https://weechat.org/doc/api#_list_size>`_"""
    ...


def list_remove(list: str, item: str) -> int:
    """`list_remove in WeeChat plugin API reference <https://weechat.org/doc/api#_list_remove>`_"""
    ...


def list_remove_all(list: str) -> int:
    """`list_remove_all in WeeChat plugin API reference <https://weechat.org/doc/api#_list_remove_all>`_"""
    ...


def list_free(list: str) -> int:
    """`list_free in WeeChat plugin API reference <https://weechat.org/doc/api#_list_free>`_"""
    ...


def config_new(name: str, callback_reload: str, callback_reload_data: str) -> str:
    """`config_new in WeeChat plugin API reference <https://weechat.org/doc/api#_config_new>`_"""
    ...


def config_new_section(config_file: str, name: str,
                       user_can_add_options: int, user_can_delete_options: int,
                       callback_read: str, callback_read_data: str,
                       callback_write: str, callback_write_data: str,
                       callback_write_default: str, callback_write_default_data: str,
                       callback_create_option: str, callback_create_option_data: str,
                       callback_delete_option: str, callback_delete_option_data: str) -> str:
    """`config_new_section in WeeChat plugin API reference <https://weechat.org/doc/api#_config_new_section>`_"""
    ...


def config_search_section(config_file: str, section_name: str) -> str:
    """`config_search_section in WeeChat plugin API reference <https://weechat.org/doc/api#_config_search_section>`_"""
    ...


def config_new_option(config_file: str, section: str, name: str, type: str, description: str,
                      string_values: str, min: int, max: int,
                      default_value: str, value: str, null_value_allowed: int,
                      callback_check_value: str, callback_check_value_data: str,
                      callback_change: str, callback_change_data: str,
                      callback_delete: str, callback_delete_data: str) -> str:
    """`config_new_option in WeeChat plugin API reference <https://weechat.org/doc/api#_config_new_option>`_"""
    ...


def config_search_option(config_file: str, section: str, option_name: str) -> str:
    """`config_search_option in WeeChat plugin API reference <https://weechat.org/doc/api#_config_search_option>`_"""
    ...


def config_string_to_boolean(text: str) -> int:
    """`config_string_to_boolean in WeeChat plugin API reference <https://weechat.org/doc/api#_config_string_to_boolean>`_"""
    ...


def config_option_reset(option: str, run_callback: int) -> int:
    """`config_option_reset in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_reset>`_"""
    ...


def config_option_set(option: str, value: str, run_callback: int) -> int:
    """`config_option_set in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_set>`_"""
    ...


def config_option_set_null(option: str, run_callback: int) -> int:
    """`config_option_set_null in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_set_null>`_"""
    ...


def config_option_unset(option: str) -> int:
    """`config_option_unset in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_unset>`_"""
    ...


def config_option_rename(option: str, new_name: str) -> int:
    """`config_option_rename in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_rename>`_"""
    ...


def config_option_is_null(option: str) -> int:
    """`config_option_is_null in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_is_null>`_"""
    ...


def config_option_default_is_null(option: str) -> int:
    """`config_option_default_is_null in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_default_is_null>`_"""
    ...


def config_boolean(option: str) -> int:
    """`config_boolean in WeeChat plugin API reference <https://weechat.org/doc/api#_config_boolean>`_"""
    ...


def config_boolean_default(option: str) -> int:
    """`config_boolean_default in WeeChat plugin API reference <https://weechat.org/doc/api#_config_boolean_default>`_"""
    ...


def config_integer(option: str) -> int:
    """`config_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_config_integer>`_"""
    ...


def config_integer_default(option: str) -> int:
    """`config_integer_default in WeeChat plugin API reference <https://weechat.org/doc/api#_config_integer_default>`_"""
    ...


def config_string(option: str) -> str:
    """`config_string in WeeChat plugin API reference <https://weechat.org/doc/api#_config_string>`_"""
    ...


def config_string_default(option: str) -> str:
    """`config_string_default in WeeChat plugin API reference <https://weechat.org/doc/api#_config_string_default>`_"""
    ...


def config_color(option: str) -> str:
    """`config_color in WeeChat plugin API reference <https://weechat.org/doc/api#_config_color>`_"""
    ...


def config_color_default(option: str) -> str:
    """`config_color_default in WeeChat plugin API reference <https://weechat.org/doc/api#_config_color_default>`_"""
    ...


def config_write_option(config_file: str, option: str) -> int:
    """`config_write_option in WeeChat plugin API reference <https://weechat.org/doc/api#_config_write_option>`_"""
    ...


def config_write_line(config_file: str, option_name: str, value: str) -> int:
    """`config_write_line in WeeChat plugin API reference <https://weechat.org/doc/api#_config_write_line>`_"""
    ...


def config_write(config_file: str) -> int:
    """`config_write in WeeChat plugin API reference <https://weechat.org/doc/api#_config_write>`_"""
    ...


def config_read(config_file: str) -> int:
    """`config_read in WeeChat plugin API reference <https://weechat.org/doc/api#_config_read>`_"""
    ...


def config_reload(config_file: str) -> int:
    """`config_reload in WeeChat plugin API reference <https://weechat.org/doc/api#_config_reload>`_"""
    ...


def config_option_free(option: str) -> int:
    """`config_option_free in WeeChat plugin API reference <https://weechat.org/doc/api#_config_option_free>`_"""
    ...


def config_section_free_options(section: str) -> int:
    """`config_section_free_options in WeeChat plugin API reference <https://weechat.org/doc/api#_config_section_free_options>`_"""
    ...


def config_section_free(section: str) -> int:
    """`config_section_free in WeeChat plugin API reference <https://weechat.org/doc/api#_config_section_free>`_"""
    ...


def config_free(config_file: str) -> int:
    """`config_free in WeeChat plugin API reference <https://weechat.org/doc/api#_config_free>`_"""
    ...


def config_get(option_name: str) -> str:
    """`config_get in WeeChat plugin API reference <https://weechat.org/doc/api#_config_get>`_"""
    ...


def config_get_plugin(option_name: str) -> str:
    """`config_get_plugin in WeeChat plugin API reference <https://weechat.org/doc/api#_config_get_plugin>`_"""
    ...


def config_is_set_plugin(option_name: str) -> int:
    """`config_is_set_plugin in WeeChat plugin API reference <https://weechat.org/doc/api#_config_is_set_plugin>`_"""
    ...


def config_set_plugin(option_name: str, value: str) -> int:
    """`config_set_plugin in WeeChat plugin API reference <https://weechat.org/doc/api#_config_set_plugin>`_"""
    ...


def config_set_desc_plugin(option_name: str, description: str) -> int:
    """`config_set_desc_plugin in WeeChat plugin API reference <https://weechat.org/doc/api#_config_set_desc_plugin>`_"""
    ...


def config_unset_plugin(option_name: str) -> int:
    """`config_unset_plugin in WeeChat plugin API reference <https://weechat.org/doc/api#_config_unset_plugin>`_"""
    ...


def key_bind(context: str, keys: Dict[str, str]) -> int:
    """`key_bind in WeeChat plugin API reference <https://weechat.org/doc/api#_key_bind>`_"""
    ...


def key_unbind(context: str, key: str) -> int:
    """`key_unbind in WeeChat plugin API reference <https://weechat.org/doc/api#_key_unbind>`_"""
    ...


def prefix(prefix: str) -> str:
    """`prefix in WeeChat plugin API reference <https://weechat.org/doc/api#_prefix>`_"""
    ...


def color(color_name: str) -> str:
    """`color in WeeChat plugin API reference <https://weechat.org/doc/api#_color>`_"""
    ...


def prnt(buffer: str, message: str) -> int:
    """`prnt in WeeChat plugin API reference <https://weechat.org/doc/api#_prnt>`_"""
    ...


def prnt_date_tags(buffer: str, date: int, tags: str, message: str) -> int:
    """`prnt_date_tags in WeeChat plugin API reference <https://weechat.org/doc/api#_prnt_date_tags>`_"""
    ...


def prnt_y(buffer: str, y: int, message: str) -> int:
    """`prnt_y in WeeChat plugin API reference <https://weechat.org/doc/api#_prnt_y>`_"""
    ...


def prnt_y_date_tags(buffer: str, y: int, date: int, tags: str, message: str) -> int:
    """`prnt_y_date_tags in WeeChat plugin API reference <https://weechat.org/doc/api#_prnt_y_date_tags>`_"""
    ...


def log_print(message: str) -> int:
    """`log_print in WeeChat plugin API reference <https://weechat.org/doc/api#_log_print>`_"""
    ...


def hook_command(command: str, description: str, args: str, args_description: str,
                 completion: str, callback: str, callback_data: str) -> str:
    """`hook_command in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_command>`_"""
    ...


def hook_completion(completion_item: str, description: str, callback: str, callback_data: str) -> str:
    """`hook_completion in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_completion>`_"""
    ...


def hook_command_run(command: str, callback: str, callback_data: str) -> str:
    """`hook_command_run in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_command_run>`_"""
    ...


def hook_timer(interval: int, align_second: int, max_calls: int, callback: str, callback_data: str) -> str:
    """`hook_timer in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_timer>`_"""
    ...


def hook_fd(fd: int, flag_read: int, flag_write: int, flag_exception: int, callback: str, callback_data: str) -> str:
    """`hook_fd in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_fd>`_"""
    ...


def hook_process(command: str, timeout: int, callback: str, callback_data: str) -> str:
    """`hook_process in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_process>`_"""
    ...


def hook_process_hashtable(command: str, options: Dict[str, str], timeout: int, callback: str, callback_data: str) -> str:
    """`hook_process_hashtable in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_process_hashtable>`_"""
    ...


def hook_connect(proxy: str, address: str, port: int, ipv6: int, retry: int, local_hostname: str,
                 callback: str, callback_data: str) -> str:
    """`hook_connect in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_connect>`_"""
    ...


def hook_line(buffer_type: str, buffer_name: str, tags: str, callback: str, callback_data: str) -> str:
    """`hook_line in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_line>`_"""
    ...


def hook_print(buffer: str, tags: str, message: str, strip_colors: int, callback: str, callback_data: str) -> str:
    """`hook_print in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_print>`_"""
    ...


def hook_signal(signal: str, callback: str, callback_data: str) -> str:
    """`hook_signal in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_signal>`_"""
    ...


def hook_signal_send(signal: str, type_data: str, signal_data: str) -> int:
    """`hook_signal_send in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_signal_send>`_"""
    ...


def hook_hsignal(signal: str, callback: str, callback_data: str) -> str:
    """`hook_hsignal in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_hsignal>`_"""
    ...


def hook_hsignal_send(signal: str, hashtable: Dict[str, str]) -> int:
    """`hook_hsignal_send in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_hsignal_send>`_"""
    ...


def hook_config(option: str, callback: str, callback_data: str) -> str:
    """`hook_config in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_config>`_"""
    ...


def hook_modifier(modifier: str, callback: str, callback_data: str) -> str:
    """`hook_modifier in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_modifier>`_"""
    ...


def hook_modifier_exec(modifier: str, modifier_data: str, string: str) -> str:
    """`hook_modifier_exec in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_modifier_exec>`_"""
    ...


def hook_info(info_name: str, description: str, args_description: str,
              callback: str, callback_data: str) -> str:
    """`hook_info in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_info>`_"""
    ...


def hook_info_hashtable(info_name: str, description: str, args_description: str,
                        output_description: str, callback: str, callback_data: str) -> str:
    """`hook_info_hashtable in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_info_hashtable>`_"""
    ...


def hook_infolist(infolist_name: str, description: str, pointer_description: str,
                  args_description: str, callback: str, callback_data: str) -> str:
    """`hook_infolist in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_infolist>`_"""
    ...


def hook_focus(area: str, callback: str, callback_data: str) -> str:
    """`hook_focus in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_focus>`_"""
    ...


def hook_set(hook: str, property: str, value: str) -> int:
    """`hook_set in WeeChat plugin API reference <https://weechat.org/doc/api#_hook_set>`_"""
    ...


def unhook(hook: str) -> int:
    """`unhook in WeeChat plugin API reference <https://weechat.org/doc/api#_unhook>`_"""
    ...


def unhook_all() -> int:
    """`unhook_all in WeeChat plugin API reference <https://weechat.org/doc/api#_unhook_all>`_"""
    ...


def buffer_new(name: str, input_callback: str, input_callback_data: str,
               close_callback: str, close_callback_data: str) -> str:
    """`buffer_new in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_new>`_"""
    ...


def current_buffer() -> str:
    """`current_buffer in WeeChat plugin API reference <https://weechat.org/doc/api#_current_buffer>`_"""
    ...


def buffer_search(plugin: str, name: str) -> str:
    """`buffer_search in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_search>`_"""
    ...


def buffer_search_main() -> str:
    """`buffer_search_main in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_search_main>`_"""
    ...


def buffer_clear(buffer: str) -> int:
    """`buffer_clear in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_clear>`_"""
    ...


def buffer_close(buffer: str) -> int:
    """`buffer_close in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_close>`_"""
    ...


def buffer_merge(buffer: str, target_buffer: str) -> int:
    """`buffer_merge in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_merge>`_"""
    ...


def buffer_unmerge(buffer: str, number: int) -> int:
    """`buffer_unmerge in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_unmerge>`_"""
    ...


def buffer_get_integer(buffer: str, property: str) -> int:
    """`buffer_get_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_get_integer>`_"""
    ...


def buffer_get_string(buffer: str, property: str) -> str:
    """`buffer_get_string in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_get_string>`_"""
    ...


def buffer_get_pointer(buffer: str, property: str) -> str:
    """`buffer_get_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_get_pointer>`_"""
    ...


def buffer_set(buffer: str, property: str, value: str) -> int:
    """`buffer_set in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_set>`_"""
    ...


def buffer_string_replace_local_var(buffer: str, string: str) -> str:
    """`buffer_string_replace_local_var in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_string_replace_local_var>`_"""
    ...


def buffer_match_list(buffer: str, string: str) -> int:
    """`buffer_match_list in WeeChat plugin API reference <https://weechat.org/doc/api#_buffer_match_list>`_"""
    ...


def current_window() -> str:
    """`current_window in WeeChat plugin API reference <https://weechat.org/doc/api#_current_window>`_"""
    ...


def window_search_with_buffer(buffer: str) -> str:
    """`window_search_with_buffer in WeeChat plugin API reference <https://weechat.org/doc/api#_window_search_with_buffer>`_"""
    ...


def window_get_integer(window: str, property: str) -> int:
    """`window_get_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_window_get_integer>`_"""
    ...


def window_get_string(window: str, property: str) -> str:
    """`window_get_string in WeeChat plugin API reference <https://weechat.org/doc/api#_window_get_string>`_"""
    ...


def window_get_pointer(window: str, property: str) -> str:
    """`window_get_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_window_get_pointer>`_"""
    ...


def window_set_title(title: str) -> int:
    """`window_set_title in WeeChat plugin API reference <https://weechat.org/doc/api#_window_set_title>`_"""
    ...


def nicklist_add_group(buffer: str, parent_group: str, name: str, color: str, visible: int) -> str:
    """`nicklist_add_group in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_add_group>`_"""
    ...


def nicklist_search_group(buffer: str, from_group: str, name: str) -> str:
    """`nicklist_search_group in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_search_group>`_"""
    ...


def nicklist_add_nick(buffer: str, group: str, name: str, color: str, prefix: str, prefix_color: str, visible: int) -> str:
    """`nicklist_add_nick in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_add_nick>`_"""
    ...


def nicklist_search_nick(buffer: str, from_group: str, name: str) -> str:
    """`nicklist_search_nick in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_search_nick>`_"""
    ...


def nicklist_remove_group(buffer: str, group: str) -> int:
    """`nicklist_remove_group in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_remove_group>`_"""
    ...


def nicklist_remove_nick(buffer: str, nick: str) -> int:
    """`nicklist_remove_nick in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_remove_nick>`_"""
    ...


def nicklist_remove_all(buffer: str) -> int:
    """`nicklist_remove_all in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_remove_all>`_"""
    ...


def nicklist_group_get_integer(buffer: str, group: str, property: str) -> int:
    """`nicklist_group_get_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_group_get_integer>`_"""
    ...


def nicklist_group_get_string(buffer: str, group: str, property: str) -> str:
    """`nicklist_group_get_string in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_group_get_string>`_"""
    ...


def nicklist_group_get_pointer(buffer: str, group: str, property: str) -> str:
    """`nicklist_group_get_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_group_get_pointer>`_"""
    ...


def nicklist_group_set(buffer: str, group: str, property: str, value: str) -> int:
    """`nicklist_group_set in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_group_set>`_"""
    ...


def nicklist_nick_get_integer(buffer: str, nick: str, property: str) -> int:
    """`nicklist_nick_get_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_nick_get_integer>`_"""
    ...


def nicklist_nick_get_string(buffer: str, nick: str, property: str) -> str:
    """`nicklist_nick_get_string in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_nick_get_string>`_"""
    ...


def nicklist_nick_get_pointer(buffer: str, nick: str, property: str) -> str:
    """`nicklist_nick_get_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_nick_get_pointer>`_"""
    ...


def nicklist_nick_set(buffer: str, nick: str, property: str, value: str) -> int:
    """`nicklist_nick_set in WeeChat plugin API reference <https://weechat.org/doc/api#_nicklist_nick_set>`_"""
    ...


def bar_item_search(name: str) -> str:
    """`bar_item_search in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_item_search>`_"""
    ...


def bar_item_new(name: str, build_callback: str, build_callback_data: str) -> str:
    """`bar_item_new in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_item_new>`_"""
    ...


def bar_item_update(name: str) -> int:
    """`bar_item_update in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_item_update>`_"""
    ...


def bar_item_remove(item: str) -> int:
    """`bar_item_remove in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_item_remove>`_"""
    ...


def bar_search(name: str) -> str:
    """`bar_search in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_search>`_"""
    ...


def bar_new(name: str, hidden: str, priority: str, type: str, condition: str, position: str,
            filling_top_bottom: str, filling_left_right: str, size: str, size_max: str,
            color_fg: str, color_delim: str, color_bg: str, color_bg_inactive: str,
            separator: str, items: str) -> str:
    """`bar_new in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_new>`_"""
    ...


def bar_set(bar: str, property: str, value: str) -> int:
    """`bar_set in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_set>`_"""
    ...


def bar_update(name: str) -> int:
    """`bar_update in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_update>`_"""
    ...


def bar_remove(bar: str) -> int:
    """`bar_remove in WeeChat plugin API reference <https://weechat.org/doc/api#_bar_remove>`_"""
    ...


def command(buffer: str, command: str) -> int:
    """`command in WeeChat plugin API reference <https://weechat.org/doc/api#_command>`_"""
    ...


def command_options(buffer: str, command: str, options: Dict[str, str]) -> int:
    """`command_options in WeeChat plugin API reference <https://weechat.org/doc/api#_command_options>`_"""
    ...


def completion_new(buffer: str) -> str:
    """`completion_new in WeeChat plugin API reference <https://weechat.org/doc/api#_completion_new>`_"""
    ...


def completion_search(completion: str, data: str, position: int, direction: int) -> int:
    """`completion_search in WeeChat plugin API reference <https://weechat.org/doc/api#_completion_search>`_"""
    ...


def completion_get_string(completion: str, property: str) -> str:
    """`completion_get_string in WeeChat plugin API reference <https://weechat.org/doc/api#_completion_get_string>`_"""
    ...


def completion_list_add(completion: str, word: str, nick_completion: int, where: str) -> int:
    """`completion_list_add in WeeChat plugin API reference <https://weechat.org/doc/api#_completion_list_add>`_"""
    ...


def completion_free(completion: str) -> int:
    """`completion_free in WeeChat plugin API reference <https://weechat.org/doc/api#_completion_free>`_"""
    ...


def info_get(info_name: str, arguments: str) -> str:
    """`info_get in WeeChat plugin API reference <https://weechat.org/doc/api#_info_get>`_"""
    ...


def info_get_hashtable(info_name: str, dict_in: Dict[str, str]) -> Dict[str, str]:
    """`info_get_hashtable in WeeChat plugin API reference <https://weechat.org/doc/api#_info_get_hashtable>`_"""
    ...


def infolist_new() -> str:
    """`infolist_new in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_new>`_"""
    ...


def infolist_new_item(infolist: str) -> str:
    """`infolist_new_item in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_new_item>`_"""
    ...


def infolist_new_var_integer(item: str, name: str, value: int) -> str:
    """`infolist_new_var_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_new_var_integer>`_"""
    ...


def infolist_new_var_string(item: str, name: str, value: str) -> str:
    """`infolist_new_var_string in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_new_var_string>`_"""
    ...


def infolist_new_var_pointer(item: str, name: str, pointer: str) -> str:
    """`infolist_new_var_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_new_var_pointer>`_"""
    ...


def infolist_new_var_time(item: str, name: str, time: int) -> str:
    """`infolist_new_var_time in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_new_var_time>`_"""
    ...


def infolist_get(infolist_name: str, pointer: str, arguments: str) -> str:
    """`infolist_get in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_get>`_"""
    ...


def infolist_next(infolist: str) -> int:
    """`infolist_next in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_next>`_"""
    ...


def infolist_prev(infolist: str) -> int:
    """`infolist_prev in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_prev>`_"""
    ...


def infolist_reset_item_cursor(infolist: str) -> int:
    """`infolist_reset_item_cursor in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_reset_item_cursor>`_"""
    ...


def infolist_search_var(infolist: str, name: str) -> str:
    """`infolist_search_var in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_search_var>`_"""
    ...


def infolist_fields(infolist: str) -> str:
    """`infolist_fields in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_fields>`_"""
    ...


def infolist_integer(infolist: str, var: str) -> int:
    """`infolist_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_integer>`_"""
    ...


def infolist_string(infolist: str, var: str) -> str:
    """`infolist_string in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_string>`_"""
    ...


def infolist_pointer(infolist: str, var: str) -> str:
    """`infolist_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_pointer>`_"""
    ...


def infolist_time(infolist: str, var: str) -> int:
    """`infolist_time in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_time>`_"""
    ...


def infolist_free(infolist: str) -> int:
    """`infolist_free in WeeChat plugin API reference <https://weechat.org/doc/api#_infolist_free>`_"""
    ...


def hdata_get(hdata_name: str) -> str:
    """`hdata_get in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get>`_"""
    ...


def hdata_get_var_offset(hdata: str, name: str) -> int:
    """`hdata_get_var_offset in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get_var_offset>`_"""
    ...


def hdata_get_var_type_string(hdata: str, name: str) -> str:
    """`hdata_get_var_type_string in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get_var_type_string>`_"""
    ...


def hdata_get_var_array_size(hdata: str, pointer: str, name: str) -> int:
    """`hdata_get_var_array_size in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get_var_array_size>`_"""
    ...


def hdata_get_var_array_size_string(hdata: str, pointer: str, name: str) -> str:
    """`hdata_get_var_array_size_string in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get_var_array_size_string>`_"""
    ...


def hdata_get_var_hdata(hdata: str, name: str) -> str:
    """`hdata_get_var_hdata in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get_var_hdata>`_"""
    ...


def hdata_get_list(hdata: str, name: str) -> str:
    """`hdata_get_list in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get_list>`_"""
    ...


def hdata_check_pointer(hdata: str, list: str, pointer: str) -> int:
    """`hdata_check_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_check_pointer>`_"""
    ...


def hdata_move(hdata: str, pointer: str, count: int) -> str:
    """`hdata_move in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_move>`_"""
    ...


def hdata_search(hdata: str, pointer: str, search: str,
                 pointers: Dict[str, str], extra_vars: Dict[str, str], options: Dict[str, str],
                 count: int) -> str:
    """`hdata_search in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_search>`_"""
    ...


def hdata_char(hdata: str, pointer: str, name: str) -> int:
    """`hdata_char in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_char>`_"""
    ...


def hdata_integer(hdata: str, pointer: str, name: str) -> int:
    """`hdata_integer in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_integer>`_"""
    ...


def hdata_long(hdata: str, pointer: str, name: str) -> int:
    """`hdata_long in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_long>`_"""
    ...


def hdata_string(hdata: str, pointer: str, name: str) -> str:
    """`hdata_string in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_string>`_"""
    ...


def hdata_pointer(hdata: str, pointer: str, name: str) -> str:
    """`hdata_pointer in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_pointer>`_"""
    ...


def hdata_time(hdata: str, pointer: str, name: str) -> int:
    """`hdata_time in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_time>`_"""
    ...


def hdata_hashtable(hdata: str, pointer: str, name: str) -> Dict[str, str]:
    """`hdata_hashtable in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_hashtable>`_"""
    ...


def hdata_compare(hdata: str, pointer1: str, pointer2: str, name: str, case_sensitive: int) -> int:
    """`hdata_compare in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_compare>`_"""
    ...


def hdata_update(hdata: str, pointer: str, hashtable: Dict[str, str]) -> int:
    """`hdata_update in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_update>`_"""
    ...


def hdata_get_string(hdata: str, property: str) -> str:
    """`hdata_get_string in WeeChat plugin API reference <https://weechat.org/doc/api#_hdata_get_string>`_"""
    ...


def upgrade_new(filename: str, callback_read: str, callback_read_data: str) -> str:
    """`upgrade_new in WeeChat plugin API reference <https://weechat.org/doc/api#_upgrade_new>`_"""
    ...


def upgrade_write_object(upgrade_file: str, object_id: int, infolist: str) -> int:
    """`upgrade_write_object in WeeChat plugin API reference <https://weechat.org/doc/api#_upgrade_write_object>`_"""
    ...


def upgrade_read(upgrade_file: str) -> int:
    """`upgrade_read in WeeChat plugin API reference <https://weechat.org/doc/api#_upgrade_read>`_"""
    ...


def upgrade_close(upgrade_file: str) -> int:
    """`upgrade_close in WeeChat plugin API reference <https://weechat.org/doc/api#_upgrade_close>`_"""
    ...

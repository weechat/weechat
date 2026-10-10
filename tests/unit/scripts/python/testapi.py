# SPDX-FileCopyrightText: 2017-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""WeeChat scripting API tests.

It cannot be run directly and cannot be loaded in WeeChat.

It is parsed by testapigen.py, using Python AST (Abstract Syntax Trees),
to generate scripts in all supported languages (Python, Perl, Ruby, ...).
The resulting scripts can be loaded in WeeChat to test the scripting API.
"""

# ruff: noqa: ANN001,ANN201,ARG001,COM812,INP001,ISC003,PLR2004,PLR0915

import weechat


def check(result, condition, lineno):
    """Display the result of a test."""
    if result:
        weechat.prnt("", "      TEST OK: " + condition)
    else:
        weechat.prnt(
            "",
            "{SCRIPT_SOURCE}"
            + ":"
            + lineno
            + ":1: "
            + "ERROR: ["
            + "{SCRIPT_NAME}"
            + "] condition is false: "
            + condition,
        )


def test_constants():
    """Test constants."""
    check(weechat.WEECHAT_RC_OK == 0)
    check(weechat.WEECHAT_RC_OK_EAT == 1)
    check(weechat.WEECHAT_RC_ERROR == -1)
    check(weechat.WEECHAT_CONFIG_READ_OK == 0)
    check(weechat.WEECHAT_CONFIG_READ_MEMORY_ERROR == -1)
    check(weechat.WEECHAT_CONFIG_READ_FILE_NOT_FOUND == -2)
    check(weechat.WEECHAT_CONFIG_WRITE_OK == 0)
    check(weechat.WEECHAT_CONFIG_WRITE_ERROR == -1)
    check(weechat.WEECHAT_CONFIG_WRITE_MEMORY_ERROR == -2)
    check(weechat.WEECHAT_CONFIG_OPTION_SET_OK_CHANGED == 2)
    check(weechat.WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE == 1)
    check(weechat.WEECHAT_CONFIG_OPTION_SET_ERROR == 0)
    check(weechat.WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND == -1)
    check(weechat.WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET == 0)
    check(weechat.WEECHAT_CONFIG_OPTION_UNSET_OK_RESET == 1)
    check(weechat.WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED == 2)
    check(weechat.WEECHAT_CONFIG_OPTION_UNSET_ERROR == -1)
    check(weechat.WEECHAT_LIST_POS_SORT == "sort")
    check(weechat.WEECHAT_LIST_POS_BEGINNING == "beginning")
    check(weechat.WEECHAT_LIST_POS_END == "end")
    check(weechat.WEECHAT_HOTLIST_LOW == "0")
    check(weechat.WEECHAT_HOTLIST_MESSAGE == "1")
    check(weechat.WEECHAT_HOTLIST_PRIVATE == "2")
    check(weechat.WEECHAT_HOTLIST_HIGHLIGHT == "3")
    check(weechat.WEECHAT_HOOK_PROCESS_RUNNING == -1)
    check(weechat.WEECHAT_HOOK_PROCESS_ERROR == -2)
    check(weechat.WEECHAT_HOOK_CONNECT_IPV6_DISABLE == 0)
    check(weechat.WEECHAT_HOOK_CONNECT_IPV6_AUTO == 1)
    check(weechat.WEECHAT_HOOK_CONNECT_IPV6_FORCE == 2)
    check(weechat.WEECHAT_HOOK_CONNECT_OK == 0)
    check(weechat.WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND == 1)
    check(weechat.WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND == 2)
    check(weechat.WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED == 3)
    check(weechat.WEECHAT_HOOK_CONNECT_PROXY_ERROR == 4)
    check(weechat.WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR == 5)
    check(weechat.WEECHAT_HOOK_CONNECT_GNUTLS_INIT_ERROR == 6)
    check(weechat.WEECHAT_HOOK_CONNECT_GNUTLS_HANDSHAKE_ERROR == 7)
    check(weechat.WEECHAT_HOOK_CONNECT_MEMORY_ERROR == 8)
    check(weechat.WEECHAT_HOOK_CONNECT_TIMEOUT == 9)
    check(weechat.WEECHAT_HOOK_CONNECT_SOCKET_ERROR == 10)
    check(weechat.WEECHAT_HOOK_SIGNAL_STRING == "string")
    check(weechat.WEECHAT_HOOK_SIGNAL_INT == "int")
    check(weechat.WEECHAT_HOOK_SIGNAL_POINTER == "pointer")


def test_plugins():
    """Test plugins functions."""
    check(weechat.plugin_get_name("") == "core")
    check(weechat.plugin_get_name(weechat.buffer_get_pointer(weechat.buffer_search_main(), "plugin")) == "core")


def test_strings():
    """Test string functions."""
    check(weechat.charset_set("iso-8859-15") == 1)
    check(weechat.charset_set("") == 1)
    check(weechat.iconv_to_internal("iso-8859-15", "abc") == "abc")
    check(weechat.iconv_from_internal("iso-8859-15", "abcd") == "abcd")
    check(weechat.gettext("abcdef") == "abcdef")
    check(weechat.ngettext("file", "files", 1) == "file")
    check(weechat.ngettext("file", "files", 2) == "files")
    check(weechat.strlen_screen("abcd") == 4)
    check(weechat.string_match("abcdef", "abc*", 0) == 1)
    check(weechat.string_match("abcdef", "abc*", 1) == 1)
    check(weechat.string_match("ABCDEF", "abc*", 1) == 0)
    check(weechat.string_match_list("abcdef", "*,!abc*", 0) == 0)
    check(weechat.string_match_list("ABCDEF", "*,!abc*", 1) == 1)
    check(weechat.string_match_list("def", "*,!abc*", 0) == 1)
    check(weechat.string_eval_path_home("test ${abc}", {}, {"abc": "123"}, {}) == "test 123")
    check(weechat.string_mask_to_regex("test*mask") == "test.*mask")
    check(weechat.string_has_highlight("my test string", "test,word2") == 1)
    check(weechat.string_has_highlight_regex("my test string", "test|word2") == 1)
    check(weechat.string_format_size(0) == "0 bytes")
    check(weechat.string_format_size(1) == "1 byte")
    check(weechat.string_format_size(2097152) == "2.10 MB")
    check(weechat.string_format_size(420000000) == "420.00 MB")
    check(weechat.string_parse_size("") == 0)
    check(weechat.string_parse_size("*") == 0)
    check(weechat.string_parse_size("z") == 0)
    check(weechat.string_parse_size("1ba") == 0)
    check(weechat.string_parse_size("1") == 1)
    check(weechat.string_parse_size("12b") == 12)
    check(weechat.string_parse_size("123 b") == 123)
    check(weechat.string_parse_size("120k") == 120000)
    check(weechat.string_parse_size("1500m") == 1500000000)
    check(weechat.string_parse_size("2g") == 2000000000)
    check(weechat.string_color_code_size("") == 0)
    check(weechat.string_color_code_size("test") == 0)
    str_color = weechat.color("yellow,red")
    check(weechat.string_color_code_size(str_color) == 7)
    check(weechat.string_remove_color("test", "?") == "test")
    check(weechat.string_is_command_char("/test") == 1)
    check(weechat.string_is_command_char("test") == 0)
    check(weechat.string_input_for_buffer("test") == "test")
    check(weechat.string_input_for_buffer("/test") == "")
    check(weechat.string_input_for_buffer("//test") == "/test")
    check(weechat.string_eval_expression("100 > 50", {}, {}, {"type": "condition"}) == "1")
    check(weechat.string_eval_expression("-50 < 100", {}, {}, {"type": "condition"}) == "1")
    check(weechat.string_eval_expression("18.2 > 5", {}, {}, {"type": "condition"}) == "1")
    check(weechat.string_eval_expression("0xA3 > 2", {}, {}, {"type": "condition"}) == "1")
    check(weechat.string_eval_expression("${buffer.full_name}", {}, {}, {}) == "core.weechat")


def test_dir():
    """Test directory functions."""
    # The directory "testapi_dir" is removed before each run of the tests.
    path = weechat.info_get("weechat_data_dir", "") + "/testapi_dir"
    # mkdir
    check(weechat.mkdir(path + "/dir1/sub", 0o755) == 0)
    check(weechat.mkdir(path, 0o755) == 1)
    check(weechat.mkdir(path, 0o755) == 1)
    check(weechat.mkdir(path + "/dir1", 0o755) == 1)
    check(weechat.mkdir(path + "/dir1/sub", 0o755) == 1)
    check(weechat.mkdir("/dev/null/testapi_dir", 0o755) == 0)
    # mkdir_parents
    check(weechat.mkdir(path + "/dir2/sub1/sub2/sub3", 0o755) == 0)
    check(weechat.mkdir_parents(path + "/dir2/sub1/sub2", 0o755) == 1)
    check(weechat.mkdir(path + "/dir2/sub1/sub2/sub3", 0o755) == 1)
    check(weechat.mkdir_parents(path + "/dir2/sub1/sub2", 0o755) == 1)
    check(weechat.mkdir_parents("/dev/null/testapi_dir/sub", 0o755) == 0)
    # mkdir_home
    check(weechat.mkdir(path + "/dir3/sub", 0o755) == 0)
    check(weechat.mkdir_home("testapi_dir/dir3", 0o755) == 1)
    check(weechat.mkdir(path + "/dir3/sub", 0o755) == 1)
    check(weechat.mkdir(path + "/dir4/sub", 0o755) == 0)
    check(weechat.mkdir_home("${weechat_data_dir}/testapi_dir/dir4", 0o755) == 1)
    check(weechat.mkdir(path + "/dir4/sub", 0o755) == 1)
    check(weechat.mkdir_home("testapi_dir/dir5/sub", 0o755) == 0)


def test_lists():
    """Test list functions."""
    ptr_list = weechat.list_new()
    check(ptr_list != "")
    check(weechat.list_size(ptr_list) == 0)
    item_def = weechat.list_add(ptr_list, "def", weechat.WEECHAT_LIST_POS_SORT, "")
    check(weechat.list_size(ptr_list) == 1)
    item_abc = weechat.list_add(ptr_list, "abc", weechat.WEECHAT_LIST_POS_SORT, "")
    check(weechat.list_size(ptr_list) == 2)
    check(weechat.list_search(ptr_list, "abc") == item_abc)
    check(weechat.list_search(ptr_list, "def") == item_def)
    check(weechat.list_search(ptr_list, "ghi") == "")
    check(weechat.list_search_pos(ptr_list, "abc") == 0)
    check(weechat.list_search_pos(ptr_list, "def") == 1)
    check(weechat.list_search_pos(ptr_list, "ghi") == -1)
    check(weechat.list_casesearch(ptr_list, "abc") == item_abc)
    check(weechat.list_casesearch(ptr_list, "def") == item_def)
    check(weechat.list_casesearch(ptr_list, "ghi") == "")
    check(weechat.list_casesearch(ptr_list, "ABC") == item_abc)
    check(weechat.list_casesearch(ptr_list, "DEF") == item_def)
    check(weechat.list_casesearch(ptr_list, "GHI") == "")
    check(weechat.list_casesearch_pos(ptr_list, "abc") == 0)
    check(weechat.list_casesearch_pos(ptr_list, "def") == 1)
    check(weechat.list_casesearch_pos(ptr_list, "ghi") == -1)
    check(weechat.list_casesearch_pos(ptr_list, "ABC") == 0)
    check(weechat.list_casesearch_pos(ptr_list, "DEF") == 1)
    check(weechat.list_casesearch_pos(ptr_list, "GHI") == -1)
    check(weechat.list_get(ptr_list, 0) == item_abc)
    check(weechat.list_get(ptr_list, 1) == item_def)
    check(weechat.list_get(ptr_list, 2) == "")
    weechat.list_set(item_def, "def2")
    check(weechat.list_string(item_def) == "def2")
    check(weechat.list_next(item_abc) == item_def)
    check(weechat.list_next(item_def) == "")
    check(weechat.list_prev(item_abc) == "")
    check(weechat.list_prev(item_def) == item_abc)
    weechat.list_remove(ptr_list, item_abc)
    check(weechat.list_size(ptr_list) == 1)
    check(weechat.list_get(ptr_list, 0) == item_def)
    check(weechat.list_get(ptr_list, 1) == "")
    weechat.list_remove_all(ptr_list)
    check(weechat.list_size(ptr_list) == 0)
    weechat.list_free(ptr_list)


def config_reload_cb(data, config_file):
    """Config reload callback."""
    return weechat.WEECHAT_RC_OK


def config_update_cb(data, config_file, version, data_read):
    """Config update callback."""
    return weechat.WEECHAT_RC_OK


def section_read_cb(data, config_file, section, option_name, value):
    """Section read callback."""
    return weechat.WEECHAT_RC_OK


def section_write_cb(data, config_file, section_name):
    """Section write callback."""
    return weechat.WEECHAT_RC_OK


def section_write_default_cb(data, config_file, section_name):
    """Section write default callback."""
    return weechat.WEECHAT_RC_OK


def section_create_option_cb(data, config_file, section, option_name, value):
    """Section create option callback."""
    return weechat.WEECHAT_RC_OK


def section_delete_option_cb(data, config_file, section, option):
    """Section delete option callback."""
    return weechat.WEECHAT_RC_OK


def option_check_value_cb(data, option, value):
    """Option check value callback."""
    return 1


def option_change_cb(data, option):
    """Option change callback."""
    pass  # noqa: PIE790


def option_delete_cb(data, option):
    """Option delete callback."""
    pass  # noqa: PIE790


def config_write_section_cb(data, config_file, section_name):
    """Write the section, with an option and a custom line."""
    check(data == "write_data")
    check(section_name == "section")
    section = weechat.config_search_section(config_file, section_name)
    option = weechat.config_search_option(config_file, section, "option1")
    check(option != "")
    check(weechat.config_write_line(config_file, section_name, "") == 1)
    check(weechat.config_write_option(config_file, option) == 1)
    check(weechat.config_write_line(config_file, "custom", '"custom value"') == 1)
    return weechat.WEECHAT_CONFIG_WRITE_OK


def config_write_read_cb(data, config_file, section, option_name, value):
    """Read an option of the section (it is saved in a local variable of core buffer)."""
    weechat.buffer_set(weechat.buffer_search_main(), "localvar_set_testapi_read_" + option_name, value)
    return weechat.WEECHAT_CONFIG_OPTION_SET_OK_CHANGED


def test_config_write():
    """Test functions config_write_line and config_write_option."""
    config = weechat.config_new("test_config_write_" + "{SCRIPT_LANGUAGE}", "config_reload_cb", "")
    check(config != "")
    section = weechat.config_new_section(
        config,
        "section",
        0,
        0,
        "config_write_read_cb",
        "",
        "config_write_section_cb",
        "write_data",
        "section_write_default_cb",
        "",
        "section_create_option_cb",
        "",
        "section_delete_option_cb",
        "",
    )
    check(section != "")
    option = weechat.config_new_option(
        config,
        section,
        "option1",
        "string",
        "string option",
        "",
        0,
        0,
        "default",
        "value1",
        0,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(option != "")
    # Outside a write callback, the file is not opened: nothing is written.
    weechat.config_write_line(config, "custom", '"value"')
    weechat.config_write_option(config, option)
    # Write the file (the write callback is called once), then read it.
    check(weechat.config_write(config) == weechat.WEECHAT_CONFIG_WRITE_OK)
    check(weechat.config_read(config) == weechat.WEECHAT_CONFIG_READ_OK)
    buffer = weechat.buffer_search_main()
    check(weechat.buffer_get_string(buffer, "localvar_testapi_read_option1") == "value1")
    check(weechat.buffer_get_string(buffer, "localvar_testapi_read_custom") == "custom value")
    weechat.buffer_set(buffer, "localvar_del_testapi_read_option1", "")
    weechat.buffer_set(buffer, "localvar_del_testapi_read_custom", "")
    weechat.config_free(config)


def test_config():
    """Test config functions."""
    # Config
    ptr_config = weechat.config_new("test_config_" + "{SCRIPT_LANGUAGE}", "config_reload_cb", "config_reload_data")
    check(ptr_config != "")
    # Set version.
    weechat.config_set_version(ptr_config, 2, "config_update_cb", "config_update_data")
    # Section
    ptr_section = weechat.config_new_section(
        ptr_config,
        "section1",
        0,
        0,
        "section_read_cb",
        "",
        "section_write_cb",
        "",
        "section_write_default_cb",
        "",
        "section_create_option_cb",
        "",
        "section_delete_option_cb",
        "",
    )
    check(ptr_section != "")
    # Search section.
    ptr_section2 = weechat.config_search_section(ptr_config, "section1")
    check(ptr_section2 == ptr_section)
    # Boolean option
    ptr_opt_bool = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_bool",
        "boolean",
        "bool option",
        "",
        0,
        0,
        "on",
        "on",
        0,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_bool != "")
    check(weechat.config_boolean(ptr_opt_bool) == 1)
    check(weechat.config_option_set(ptr_opt_bool, "off", 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_set(ptr_opt_bool, "off", 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_boolean(ptr_opt_bool) == 0)
    check(weechat.config_boolean_default(ptr_opt_bool) == 1)
    check(weechat.config_option_reset(ptr_opt_bool, 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_reset(ptr_opt_bool, 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_boolean(ptr_opt_bool) == 1)
    # Boolean option with parent option
    ptr_opt_bool_child = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_bool_child << test_config_" + "{SCRIPT_LANGUAGE}" + ".section1.option_bool",
        "boolean",
        "bool option",
        "",
        0,
        0,
        None,
        None,
        1,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_bool_child != "")
    check(weechat.config_boolean(ptr_opt_bool_child) == 0)
    check(weechat.config_boolean_inherited(ptr_opt_bool_child) == 1)
    # Integer option
    ptr_opt_int = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_int",
        "integer",
        "int option",
        "",
        0,
        256,
        "2",
        "2",
        0,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_int != "")
    check(weechat.config_integer(ptr_opt_int) == 2)
    check(weechat.config_option_set(ptr_opt_int, "15", 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_set(ptr_opt_int, "15", 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_integer(ptr_opt_int) == 15)
    check(weechat.config_integer_default(ptr_opt_int) == 2)
    check(weechat.config_option_reset(ptr_opt_int, 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_reset(ptr_opt_int, 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_integer(ptr_opt_int) == 2)
    # Integer option with parent option
    ptr_opt_int_child = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_int_child << test_config_" + "{SCRIPT_LANGUAGE}" + ".section1.option_int",
        "integer",
        "int option",
        "",
        0,
        256,
        None,
        None,
        1,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_int_child != "")
    check(weechat.config_integer(ptr_opt_int_child) == 0)
    check(weechat.config_integer_inherited(ptr_opt_int_child) == 2)
    # Integer option (with string values: enum with WeeChat >= 4.1.0)
    ptr_opt_int_str = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_int_str",
        "integer",
        "int option str",
        "val1|val2|val3",
        0,
        0,
        "val2",
        "val2",
        0,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_int_str != "")
    check(weechat.config_integer(ptr_opt_int_str) == 1)
    check(weechat.config_string(ptr_opt_int_str) == "val2")
    check(weechat.config_option_set(ptr_opt_int_str, "val1", 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_set(ptr_opt_int_str, "val1", 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_integer(ptr_opt_int_str) == 0)
    check(weechat.config_string(ptr_opt_int_str) == "val1")
    check(weechat.config_integer_default(ptr_opt_int_str) == 1)
    check(weechat.config_string_default(ptr_opt_int_str) == "val2")
    check(weechat.config_option_reset(ptr_opt_int_str, 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_reset(ptr_opt_int_str, 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_integer(ptr_opt_int_str) == 1)
    check(weechat.config_string(ptr_opt_int_str) == "val2")
    # Integer option with parent option (with string values: enum with WeeChat >= 4.1.0)
    ptr_opt_int_str_child = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_int_str_child << test_config_" + "{SCRIPT_LANGUAGE}" + ".section1.option_int_str",
        "integer",
        "int option str",
        "val1|val2|val3",
        0,
        0,
        None,
        None,
        1,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_int_str_child != "")
    check(weechat.config_integer(ptr_opt_int_str_child) == 0)
    check(weechat.config_integer_inherited(ptr_opt_int_str_child) == 1)
    # String option
    ptr_opt_str = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_str",
        "string",
        "str option",
        "",
        0,
        0,
        "value",
        "value",
        1,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_str != "")
    check(weechat.config_string(ptr_opt_str) == "value")
    check(weechat.config_option_set(ptr_opt_str, "value2", 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_set(ptr_opt_str, "value2", 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_string(ptr_opt_str) == "value2")
    check(weechat.config_string_default(ptr_opt_str) == "value")
    check(weechat.config_option_reset(ptr_opt_str, 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_reset(ptr_opt_str, 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_string(ptr_opt_str) == "value")
    check(weechat.config_option_is_null(ptr_opt_str) == 0)
    check(weechat.config_option_set_null(ptr_opt_str, 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_set_null(ptr_opt_str, 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_option_is_null(ptr_opt_str) == 1)
    check(weechat.config_string(ptr_opt_str) == "")
    check(weechat.config_option_unset(ptr_opt_str) == 1)  # UNSET_OK_RESET
    check(weechat.config_option_unset(ptr_opt_str) == 0)  # UNSET_OK_NO_RESET
    check(weechat.config_string(ptr_opt_str) == "value")
    check(weechat.config_option_default_is_null(ptr_opt_str) == 0)
    # String option with parent option
    ptr_opt_str_child = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_str_child << test_config_" + "{SCRIPT_LANGUAGE}" + ".section1.option_str",
        "string",
        "str option",
        "",
        0,
        0,
        None,
        None,
        1,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_str_child != "")
    check(weechat.config_string(ptr_opt_str_child) == "")
    check(weechat.config_string_inherited(ptr_opt_str_child) == "value")
    # Color option
    ptr_opt_col = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_col",
        "color",
        "col option",
        "",
        0,
        0,
        "lightgreen",
        "lightgreen",
        0,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_col != "")
    check(weechat.config_color(ptr_opt_col) == "lightgreen")
    check(weechat.config_option_set(ptr_opt_col, "red", 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_set(ptr_opt_col, "red", 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_color(ptr_opt_col) == "red")
    check(weechat.config_color_default(ptr_opt_col) == "lightgreen")
    check(weechat.config_option_reset(ptr_opt_col, 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_reset(ptr_opt_col, 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_color(ptr_opt_col) == "lightgreen")
    # Color option with parent option
    ptr_opt_col_child = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_col_child << test_config_" + "{SCRIPT_LANGUAGE}" + ".section1.option_col",
        "color",
        "col option",
        "",
        0,
        0,
        None,
        None,
        1,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_col_child != "")
    check(weechat.config_color(ptr_opt_col_child) == "")
    check(weechat.config_color_inherited(ptr_opt_col_child) == "lightgreen")
    # Enum option
    ptr_opt_enum = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_enum",
        "enum",
        "enum option",
        "val1|val2|val3",
        0,
        0,
        "val2",
        "val2",
        0,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_enum != "")
    check(weechat.config_enum(ptr_opt_enum) == 1)
    check(weechat.config_integer(ptr_opt_enum) == 1)
    check(weechat.config_string(ptr_opt_enum) == "val2")
    check(weechat.config_option_set(ptr_opt_enum, "val1", 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_set(ptr_opt_enum, "val1", 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_enum(ptr_opt_enum) == 0)
    check(weechat.config_integer(ptr_opt_enum) == 0)
    check(weechat.config_string(ptr_opt_enum) == "val1")
    check(weechat.config_enum_default(ptr_opt_enum) == 1)
    check(weechat.config_integer_default(ptr_opt_enum) == 1)
    check(weechat.config_string_default(ptr_opt_enum) == "val2")
    check(weechat.config_option_reset(ptr_opt_enum, 1) == 2)  # SET_OK_CHANGED
    check(weechat.config_option_reset(ptr_opt_enum, 1) == 1)  # SET_OK_SAME_VALUE
    check(weechat.config_enum(ptr_opt_enum) == 1)
    check(weechat.config_integer(ptr_opt_enum) == 1)
    check(weechat.config_string(ptr_opt_enum) == "val2")
    # Enum option with parent option
    ptr_opt_enum_child = weechat.config_new_option(
        ptr_config,
        ptr_section,
        "option_enum_child << test_config_" + "{SCRIPT_LANGUAGE}" + ".section1.option_enum",
        "enum",
        "enum option",
        "val1|val2|val3",
        0,
        0,
        None,
        None,
        1,
        "option_check_value_cb",
        "",
        "option_change_cb",
        "",
        "option_delete_cb",
        "",
    )
    check(ptr_opt_enum_child != "")
    check(weechat.config_enum(ptr_opt_enum_child) == 0)
    check(weechat.config_enum_inherited(ptr_opt_enum_child) == 1)
    # Search option.
    ptr_opt_bool2 = weechat.config_search_option(ptr_config, ptr_section, "option_bool")
    check(ptr_opt_bool2 == ptr_opt_bool)
    # String to boolean
    check(weechat.config_string_to_boolean("") == 0)
    check(weechat.config_string_to_boolean("off") == 0)
    check(weechat.config_string_to_boolean("0") == 0)
    check(weechat.config_string_to_boolean("on") == 1)
    check(weechat.config_string_to_boolean("1") == 1)
    # Rename option.
    weechat.config_option_rename(ptr_opt_bool, "option_bool_renamed")
    # Get string property of option.
    check(weechat.config_option_get_string(ptr_opt_bool, "type") == "boolean")
    check(weechat.config_option_get_string(ptr_opt_bool, "name") == "option_bool_renamed")
    # Get pointer property of option.
    check(weechat.config_option_get_pointer(ptr_opt_bool, "config_file") == ptr_config)
    check(weechat.config_option_get_pointer(ptr_opt_bool, "section") == ptr_section)
    # Read config (create it because it does not exist yet).
    check(weechat.config_read(ptr_config) == 0)  # CONFIG_READ_OK
    # Write config.
    check(weechat.config_write(ptr_config) == 0)  # CONFIG_WRITE_OK
    # Reload config.
    check(weechat.config_reload(ptr_config) == 0)  # CONFIG_READ_OK
    # Free option.
    weechat.config_option_free(ptr_opt_bool)
    # Free options in section.
    check(weechat.config_search_option(ptr_config, ptr_section, "option_str") != "")
    weechat.config_section_free_options(ptr_section)
    check(weechat.config_search_option(ptr_config, ptr_section, "option_str") == "")
    # Free section.
    weechat.config_section_free(ptr_section)
    # Free config.
    weechat.config_free(ptr_config)
    # Get config option.
    ptr_option = weechat.config_get("weechat.look.item_time_format")
    check(ptr_option != "")
    check(weechat.config_string(ptr_option) == "%H:%M")
    # Get config plugin option.
    check(weechat.config_get_plugin("option") == "")
    check(weechat.config_is_set_plugin("option") == 0)
    check(weechat.config_set_plugin("option", "value") == 1)  # SET_OK_SAME_VALUE
    weechat.config_set_desc_plugin("option", "description of option")
    ptr_option_desc = weechat.config_get("plugins.desc." + "{SCRIPT_LANGUAGE}" + "." + "{SCRIPT_NAME}" + ".option")
    check(ptr_option_desc != "")
    check(weechat.config_string(ptr_option_desc) == "description of option")
    check(weechat.config_get_plugin("option") == "value")
    check(weechat.config_is_set_plugin("option") == 1)
    check(weechat.config_unset_plugin("option") == 2)  # UNSET_OK_REMOVED
    check(weechat.config_unset_plugin("option") == -1)  # UNSET_ERROR


def test_key():
    """Test key functions."""
    check(
        weechat.key_bind(
            "mouse",
            {
                "@chat(plugin.test):button1": "hsignal:test_mouse",
                "@chat(plugin.test):wheelup": "/mycommand up",
                "@chat(plugin.test):wheeldown": "/mycommand down",
                "__quiet": "",
            },
        )
        == 3
    )
    check(weechat.key_unbind("mouse", "quiet:area:chat(plugin.test)") == 3)


def buffer_input_cb(data, buffer, input_data):
    """Buffer input callback."""
    return weechat.WEECHAT_RC_OK


def buffer_close_cb(data, buffer):
    """Buffer close callback."""
    return weechat.WEECHAT_RC_OK


def buffer_line_data(buffer, position):
    """Return pointer to data of the first or last line of a buffer (position is "first_line" or "last_line")."""
    own_lines = weechat.hdata_pointer(weechat.hdata_get("buffer"), buffer, "own_lines")
    line = weechat.hdata_pointer(weechat.hdata_get("lines"), own_lines, position)
    return weechat.hdata_pointer(weechat.hdata_get("line"), line, "data")


def test_display():
    """Test display functions."""
    check(weechat.prefix("action") != "")
    check(weechat.prefix("error") != "")
    check(weechat.prefix("join") != "")
    check(weechat.prefix("network") != "")
    check(weechat.prefix("quit") != "")
    check(weechat.prefix("unknown") == "")
    check(weechat.color("green") != "")
    check(weechat.color("unknown") == "")
    hdata_line_data = weechat.hdata_get("line_data")
    # Print on core buffer.
    buffer = weechat.buffer_search_main()
    weechat.prnt("", "## test print core buffer")
    data = buffer_line_data(buffer, "last_line")
    check(weechat.hdata_string(hdata_line_data, data, "message") == "## test print core buffer")
    check(weechat.hdata_integer(hdata_line_data, data, "tags_count") == 0)
    weechat.prnt_date_tags("", 946681200, "tag1,tag2", "## test print_date_tags core buffer")
    data = buffer_line_data(buffer, "last_line")
    check(weechat.hdata_string(hdata_line_data, data, "message") == "## test print_date_tags core buffer")
    check(weechat.hdata_time(hdata_line_data, data, "date") == 946681200)
    check(weechat.hdata_integer(hdata_line_data, data, "tags_count") == 2)
    check(weechat.hdata_string(hdata_line_data, data, "0|tags_array") == "tag1")
    check(weechat.hdata_string(hdata_line_data, data, "1|tags_array") == "tag2")
    weechat.prnt_datetime_tags(
        "", 2146383600, 123456, "tag1,tag2", "## test print_date_tags core buffer, January, 6th 2038"
    )
    data = buffer_line_data(buffer, "last_line")
    check(
        weechat.hdata_string(hdata_line_data, data, "message")
        == "## test print_date_tags core buffer, January, 6th 2038"
    )
    check(weechat.hdata_time(hdata_line_data, data, "date") == 2146383600)
    check(weechat.hdata_integer(hdata_line_data, data, "date_usec") == 123456)
    check(weechat.hdata_integer(hdata_line_data, data, "tags_count") == 2)
    # Print on buffer with formatted content.
    buffer = weechat.buffer_new("test_formatted", "buffer_input_cb", "", "buffer_close_cb", "")
    check(buffer != "")
    check(weechat.buffer_get_integer(buffer, "type") == 0)
    weechat.prnt(buffer, "## prefix\t## test print formatted buffer")
    data = buffer_line_data(buffer, "last_line")
    check(weechat.hdata_string(hdata_line_data, data, "prefix") == "## prefix")
    check(weechat.hdata_string(hdata_line_data, data, "message") == "## test print formatted buffer")
    weechat.prnt_date_tags(buffer, 946681200, "tag1,tag2", "## test print_date_tags formatted buffer")
    data = buffer_line_data(buffer, "last_line")
    check(weechat.hdata_string(hdata_line_data, data, "message") == "## test print_date_tags formatted buffer")
    check(weechat.hdata_time(hdata_line_data, data, "date") == 946681200)
    check(weechat.hdata_string(hdata_line_data, data, "1|tags_array") == "tag2")
    weechat.buffer_close(buffer)
    # Print on buffer with free content.
    buffer = weechat.buffer_new_props("test_free", {"type": "free"}, "buffer_input_cb", "", "buffer_close_cb", "")
    check(weechat.buffer_get_integer(buffer, "type") == 1)
    check(buffer != "")
    weechat.prnt_y(buffer, 0, "## test print_y free buffer")
    data = buffer_line_data(buffer, "first_line")
    check(weechat.hdata_integer(hdata_line_data, data, "y") == 0)
    check(weechat.hdata_string(hdata_line_data, data, "message") == "## test print_y free buffer")
    # Same line number: the line is replaced.
    weechat.prnt_y_date_tags(buffer, 0, 946681200, "tag1,tag2", "## test print_y_date_tags free buffer")
    data = buffer_line_data(buffer, "first_line")
    check(weechat.hdata_integer(hdata_line_data, data, "y") == 0)
    check(weechat.hdata_string(hdata_line_data, data, "message") == "## test print_y_date_tags free buffer")
    check(weechat.hdata_string(hdata_line_data, data, "0|tags_array") == "tag1")
    weechat.prnt_y_datetime_tags(
        buffer, 1, 2146383600, 123456, "tag1,tag2", "## test print_y_date_tags free buffer, January, 6th 2038"
    )
    data = buffer_line_data(buffer, "last_line")
    check(weechat.hdata_integer(hdata_line_data, data, "y") == 1)
    check(
        weechat.hdata_string(hdata_line_data, data, "message")
        == "## test print_y_date_tags free buffer, January, 6th 2038"
    )
    check(weechat.hdata_string(hdata_line_data, data, "1|tags_array") == "tag2")
    own_lines = weechat.hdata_pointer(weechat.hdata_get("buffer"), buffer, "own_lines")
    check(weechat.hdata_integer(weechat.hdata_get("lines"), own_lines, "lines_count") == 2)
    weechat.buffer_close(buffer)
    # The message must not be used as format (it contains "%s" and "%n").
    check(weechat.log_print("test log_print: %s %n %d") == 1)


def test_theme():
    """Test theme functions."""
    check(weechat.theme_register("", {"weechat.color.chat_delimiters": "red"}) == "")
    theme = weechat.theme_register("testapi_theme", {"weechat.color.chat_delimiters": "red"})
    check(theme != "")
    # Overrides are merged in the contribution of the script.
    check(weechat.theme_register("testapi_theme", {"weechat.color.chat_inactive_window": "blue"}) == theme)
    # Apply the theme (without backup, which would display a message).
    option_backup = weechat.config_get("weechat.look.theme_backup")
    option_theme = weechat.config_get("weechat.look.theme")
    option_delimiters = weechat.config_get("weechat.color.chat_delimiters")
    option_inactive_window = weechat.config_get("weechat.color.chat_inactive_window")
    weechat.config_option_set(option_backup, "off", 1)
    check(weechat.command("", "/theme apply testapi_theme") == weechat.WEECHAT_RC_OK)
    check(weechat.config_string(option_theme) == "testapi_theme")
    check(weechat.config_color(option_delimiters) == "red")
    check(weechat.config_color(option_inactive_window) == "blue")
    # Restore options.
    weechat.config_option_reset(option_delimiters, 1)
    weechat.config_option_reset(option_inactive_window, 1)
    weechat.config_option_reset(option_theme, 1)
    weechat.config_option_reset(option_backup, 1)
    check(weechat.config_color(option_delimiters) == "22")


def completion1_cb(data, completion_item, buf, completion):
    """Completion callback."""
    check(data == "completion_data")
    check(completion_item == "{SCRIPT_NAME}1")
    check(weechat.completion_get_string(completion, "args") == "w")
    weechat.completion_set(completion, "add_space", "0")
    weechat.completion_list_add(completion, "word_completed", 0, weechat.WEECHAT_LIST_POS_END)
    return weechat.WEECHAT_RC_OK


def completion2_cb(data, completion_item, buf, completion):
    """Completion callback."""
    check(data == "completion_data")
    check(completion_item == "{SCRIPT_NAME}2")
    check(weechat.completion_get_string(completion, "args") == "w")
    weechat.completion_list_add(completion, "word_completed", 0, weechat.WEECHAT_LIST_POS_END)
    return weechat.WEECHAT_RC_OK


def completion3_cb(data, completion_item, buf, completion):
    """Completion callback (using old function names)."""
    check(data == "completion_data")
    check(completion_item == "{SCRIPT_NAME}3")
    # The old functions are not documented, so they are not in the Python stub.
    check(weechat.hook_completion_get_string(completion, "args") == "w")  # ty: ignore[unresolved-attribute]
    check(
        weechat.hook_completion_list_add(  # ty: ignore[unresolved-attribute]
            completion, "word_completed", 0, weechat.WEECHAT_LIST_POS_END
        )
        == 1
    )
    return weechat.WEECHAT_RC_OK


def command_cb(data, buf, args):
    """Command callback."""
    check(data == "command_data")
    check(args == "word_completed")
    return weechat.WEECHAT_RC_OK


def command_run_cb(data, buf, command):
    """Command_run callback."""
    check(data == "command_run_data")
    check(command == "/cmd2" + "{SCRIPT_NAME}" + " word_completed")
    return weechat.WEECHAT_RC_OK


def timer_cb(data, remaining_calls):
    """Timer callback."""
    return weechat.WEECHAT_RC_OK


def signal_string_cb(data, signal, signal_data):
    """Signal callback (string)."""
    check(data == "signal_data")
    check(signal == "{SCRIPT_NAME}_signal_string")
    check(signal_data == "test string")
    return weechat.WEECHAT_RC_OK_EAT


def signal_pointer_cb(data, signal, signal_data):
    """Signal callback (pointer)."""
    check(data == "signal_data")
    check(signal == "{SCRIPT_NAME}_signal_pointer")
    check(signal_data == weechat.buffer_search_main())
    return weechat.WEECHAT_RC_OK


def hsignal_cb(data, signal, hashtable):
    """Hsignal callback."""
    check(data == "hsignal_data")
    check(signal == "{SCRIPT_NAME}_hsignal")
    check(hashtable["key1"] == "value1")
    check(hashtable["key2"] == "value2")
    return weechat.WEECHAT_RC_OK_EAT


def modifier_cb(data, modifier, modifier_data, string):
    """Modify the string (modifier callback)."""
    check(data == "modifier_data")
    check(modifier == "{SCRIPT_NAME}_modifier")
    check(modifier_data == "test data")
    check(string == "test string")
    return string + " (modified)"


def info_cb(data, info_name, arguments):
    """Info callback."""
    check(data == "info_data")
    check(info_name == "{SCRIPT_NAME}_info")
    check(arguments == "test args")
    return "info: " + arguments


def info_hashtable_cb(data, info_name, hashtable):
    """Info_hashtable callback."""
    check(data == "info_hashtable_data")
    check(info_name == "{SCRIPT_NAME}_info_hashtable")
    check(hashtable["key_in"] == "value_in")
    return {"key_out": "value_out"}


def config_cb(data, option, value):
    """Handle a change of option."""
    check(data == "config_data")
    check(option == "plugins.var." + "{SCRIPT_LANGUAGE}" + "." + "{SCRIPT_NAME}" + ".test_hook_config")
    check(value == "value2")
    return weechat.WEECHAT_RC_OK


def print_cb(data, buf, date, tags, displayed, highlight, prefix, message):
    """Handle a printed message."""
    check(data == "print_data")
    check(buf != "")
    check(date == "1231231230")
    check(tags == "testapi_print_tag,tag2")
    check(displayed == 1)
    check(highlight == 0)
    check(prefix == "## prefix")
    check(message == "## test hook_print")
    return weechat.WEECHAT_RC_OK


def line_cb(data, line):
    """Update a line before it is displayed."""
    check(data == "line_data")
    check(line["buffer_type"] == "formatted")
    check(line["tags"] == "testapi_line_tag")
    check(line["message"] == "## test hook_line")
    return {"message": "## test hook_line (modified)"}


def focus_cb(data, info):
    """Add data to the focus info."""
    check(data == "focus_data")
    check(info["_chat"] == "1")
    check(info["_window_number"] == "1")
    return {"test_key": "test_value"}


def process_cb(data, command, return_code, out, err):
    """Get the result of the process (called once, when the process has ended)."""
    check(data == "process_data")
    check(command == "sh -c 'echo test; echo error >&2; exit 3'")
    check(return_code == 3)
    check(out == "test\n")
    check(err == "error\n")
    return weechat.WEECHAT_RC_OK


def process_hashtable_cb(data, command, return_code, out, err):
    """Get the result of the process (called once, when the process has ended)."""
    check(data == "process_hashtable_data")
    check(command == "sh")
    check(return_code == 0)
    check(out == "test hashtable\n")
    check(err == "")
    return weechat.WEECHAT_RC_OK


def url_cb(data, url, options, output):
    """Get the result of the URL transfer (called once, when the transfer has ended)."""
    check(data == "url_data")
    check(weechat.string_match(url, "file://*/test_config_write_" + "{SCRIPT_LANGUAGE}" + ".conf", 1) == 1)
    check(output["response_code"] == "0")
    check(weechat.string_match(output["output"], '*custom = "custom value"*', 1) == 1)
    return weechat.WEECHAT_RC_OK


def url_error_cb(data, url, options, output):
    """Get the result of the URL transfer (called once, when the transfer has ended)."""
    check(data == "url_error_data")
    check(output["error"] != "")
    check(output["error_code"] == "2")  # Transfer error (curl error)
    return weechat.WEECHAT_RC_OK


def connect_cb(data, status, gnutls_rc, sock, error, ip_address):
    """Get the result of the connection (called once, when connected)."""
    check(data == "connect_data")
    check(status == weechat.WEECHAT_HOOK_CONNECT_OK)
    check(gnutls_rc == 0)
    check(sock >= 0)
    check(ip_address == "127.0.0.1")
    check(error == "")
    return weechat.WEECHAT_RC_OK


def connect_refused_cb(data, status, gnutls_rc, sock, error, ip_address):
    """Get the result of the connection (called once, when refused)."""
    check(data == "connect_refused_data")
    check(status == weechat.WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED)
    return weechat.WEECHAT_RC_OK


def fd_cb(data, fd):
    """Handle data available on the file descriptor (called once: the hook is removed here)."""
    check(data == "fd_data")
    check(fd == weechat.string_parse_size(weechat.string_eval_expression("${env:WEECHAT_TESTS_FD}", {}, {}, {})))
    # The data is never read, so the hook must be removed to not be called again.
    buffer = weechat.buffer_search_main()
    hook_fd = weechat.buffer_get_string(buffer, "localvar_testapi_hook_fd")
    check(hook_fd != "")
    weechat.unhook(hook_fd)
    weechat.buffer_set(buffer, "localvar_del_testapi_hook_fd", "")
    return weechat.WEECHAT_RC_OK


def test_hooks():
    """Test hook functions."""
    buffer = weechat.buffer_search_main()
    # hook_completion / hook_completion_args / and hook_command
    hook_cmplt1 = weechat.hook_completion("{SCRIPT_NAME}1", "description", "completion1_cb", "completion_data")
    hook_cmd1 = weechat.hook_command(
        "cmd1" + "{SCRIPT_NAME}",
        "description",
        "arguments",
        "description arguments",
        "%(" + "{SCRIPT_NAME}1" + ")",
        "command_cb",
        "command_data",
    )
    hook_cmplt2 = weechat.hook_completion("{SCRIPT_NAME}2", "description", "completion2_cb", "completion_data")
    hook_cmd2 = weechat.hook_command(
        "cmd2" + "{SCRIPT_NAME}",
        "description",
        "arguments",
        "description arguments",
        "%(" + "{SCRIPT_NAME}2" + ")",
        "command_cb",
        "command_data",
    )
    weechat.command("", "/input insert /cmd1" + "{SCRIPT_NAME}" + " w")
    weechat.command("", "/input complete_next")
    buffer_input = weechat.buffer_get_string(buffer, "input")
    check(buffer_input == "/cmd1" + "{SCRIPT_NAME}" + " word_completed")
    weechat.command("", "/input delete_line")
    weechat.command("", "/input insert /cmd2" + "{SCRIPT_NAME}" + " w")
    weechat.command("", "/input complete_next")
    buffer_input = weechat.buffer_get_string(buffer, "input")
    check(buffer_input == "/cmd2" + "{SCRIPT_NAME}" + " word_completed ")
    # hook_command_run
    hook_cmd_run = weechat.hook_command_run("/cmd2" + "{SCRIPT_NAME}" + "*", "command_run_cb", "command_run_data")
    weechat.command("", "/input return")
    weechat.unhook(hook_cmd_run)
    weechat.unhook(hook_cmd1)
    weechat.unhook(hook_cmplt1)
    weechat.unhook(hook_cmd2)
    weechat.unhook(hook_cmplt2)
    # hook_completion with old functions hook_completion_get_string and hook_completion_list_add
    hook_cmplt3 = weechat.hook_completion("{SCRIPT_NAME}3", "description", "completion3_cb", "completion_data")
    hook_cmd3 = weechat.hook_command(
        "cmd3" + "{SCRIPT_NAME}",
        "description",
        "arguments",
        "description arguments",
        "%(" + "{SCRIPT_NAME}3" + ")",
        "command_cb",
        "command_data",
    )
    weechat.command("", "/input insert /cmd3" + "{SCRIPT_NAME}" + " w")
    weechat.command("", "/input complete_next")
    check(weechat.buffer_get_string(buffer, "input") == "/cmd3" + "{SCRIPT_NAME}" + " word_completed ")
    weechat.command("", "/input delete_line")
    weechat.unhook(hook_cmd3)
    weechat.unhook(hook_cmplt3)
    # hook_timer
    hook_timer = weechat.hook_timer(2000111000, 0, 1, "timer_cb", "timer_cb_data")
    ptr_infolist = weechat.infolist_get("hook", hook_timer, "")
    check(ptr_infolist != "")
    check(weechat.infolist_next(ptr_infolist) == 1)
    check(weechat.infolist_long(ptr_infolist, "interval") == 2000111000)
    weechat.infolist_free(ptr_infolist)
    weechat.unhook(hook_timer)
    # hook_signal / hook_signal_send
    signal_string = "{SCRIPT_NAME}_signal_string"
    signal_pointer = "{SCRIPT_NAME}_signal_pointer"
    hook_sig_string = weechat.hook_signal(signal_string, "signal_string_cb", "signal_data")
    check(hook_sig_string != "")
    hook_sig_pointer = weechat.hook_signal(signal_pointer, "signal_pointer_cb", "signal_data")
    check(hook_sig_pointer != "")
    rc = weechat.hook_signal_send(signal_string, weechat.WEECHAT_HOOK_SIGNAL_STRING, "test string")
    check(rc == weechat.WEECHAT_RC_OK_EAT)
    rc = weechat.hook_signal_send(signal_pointer, weechat.WEECHAT_HOOK_SIGNAL_POINTER, buffer)
    check(rc == weechat.WEECHAT_RC_OK)
    weechat.unhook(hook_sig_string)
    weechat.unhook(hook_sig_pointer)
    rc = weechat.hook_signal_send(signal_string, weechat.WEECHAT_HOOK_SIGNAL_STRING, "test string")
    check(rc == weechat.WEECHAT_RC_OK)
    # hook_hsignal / hook_hsignal_send
    hsignal = "{SCRIPT_NAME}_hsignal"
    hook_hsig = weechat.hook_hsignal(hsignal, "hsignal_cb", "hsignal_data")
    check(hook_hsig != "")
    rc = weechat.hook_hsignal_send(hsignal, {"key1": "value1", "key2": "value2"})
    check(rc == weechat.WEECHAT_RC_OK_EAT)
    weechat.unhook(hook_hsig)
    rc = weechat.hook_hsignal_send(hsignal, {"key1": "value1", "key2": "value2"})
    check(rc == weechat.WEECHAT_RC_OK)
    # hook_modifier / hook_modifier_exec
    modifier = "{SCRIPT_NAME}_modifier"
    hook_mod = weechat.hook_modifier(modifier, "modifier_cb", "modifier_data")
    check(hook_mod != "")
    check(weechat.hook_modifier_exec(modifier, "test data", "test string") == "test string (modified)")
    weechat.unhook(hook_mod)
    check(weechat.hook_modifier_exec(modifier, "test data", "test string") == "test string")
    # hook_info / info_get
    info = "{SCRIPT_NAME}_info"
    hook_inf = weechat.hook_info(info, "description", "arguments", "info_cb", "info_data")
    check(hook_inf != "")
    check(weechat.info_get(info, "test args") == "info: test args")
    weechat.unhook(hook_inf)
    check(weechat.info_get(info, "test args") == "")
    # hook_info_hashtable / info_get_hashtable
    info_hashtable = "{SCRIPT_NAME}_info_hashtable"
    hook_inf_hashtable = weechat.hook_info_hashtable(
        info_hashtable,
        "description",
        "arguments",
        "output",
        "info_hashtable_cb",
        "info_hashtable_data",
    )
    check(hook_inf_hashtable != "")
    result = weechat.info_get_hashtable(info_hashtable, {"key_in": "value_in"})
    check(result["key_out"] == "value_out")
    weechat.unhook(hook_inf_hashtable)
    # hook_config
    weechat.config_set_plugin("test_hook_config", "value1")
    hook_cfg = weechat.hook_config("plugins.var.*." + "{SCRIPT_NAME}" + ".test_hook_config", "config_cb", "config_data")
    check(hook_cfg != "")
    weechat.config_set_plugin("test_hook_config", "value2")
    weechat.unhook(hook_cfg)
    weechat.config_set_plugin("test_hook_config", "value3")
    weechat.config_unset_plugin("test_hook_config")
    # hook_print
    buffer_hooks = weechat.buffer_new("test_hooks", "buffer_input_cb", "", "buffer_close_cb", "")
    hook_prt = weechat.hook_print(buffer_hooks, "testapi_print_tag", "", 1, "print_cb", "print_data")
    check(hook_prt != "")
    weechat.prnt_date_tags(buffer_hooks, 1231231230, "tag2", "## test hook_print (not caught)")
    weechat.prnt_date_tags(buffer_hooks, 1231231230, "testapi_print_tag,tag2", "## prefix\t## test hook_print")
    weechat.unhook(hook_prt)
    weechat.prnt_date_tags(buffer_hooks, 1231231230, "testapi_print_tag,tag2", "## prefix\t## test hook_print")
    # hook_line
    hook_ln = weechat.hook_line("formatted", "*", "testapi_line_tag", "line_cb", "line_data")
    check(hook_ln != "")
    weechat.prnt_date_tags(buffer_hooks, 0, "testapi_line_tag", "## test hook_line")
    weechat.unhook(hook_ln)
    lines = weechat.hdata_pointer(weechat.hdata_get("buffer"), buffer_hooks, "own_lines")
    line = weechat.hdata_pointer(weechat.hdata_get("lines"), lines, "last_line")
    line_data = weechat.hdata_pointer(weechat.hdata_get("line"), line, "data")
    check(weechat.hdata_string(weechat.hdata_get("line_data"), line_data, "message") == "## test hook_line (modified)")
    weechat.buffer_close(buffer_hooks)
    # hook_focus
    hook_fcs = weechat.hook_focus("chat", "focus_cb", "focus_data")
    check(hook_fcs != "")
    focus_pos = {
        "x": weechat.string_eval_expression("${window.win_chat_x}", {}, {}, {}),
        "y": weechat.string_eval_expression("${window.win_chat_y}", {}, {}, {}),
    }
    focus_info = weechat.info_get_hashtable("focus_info", focus_pos)
    check(focus_info["_chat"] == "1")
    check(focus_info["test_key"] == "test_value")
    weechat.unhook(hook_fcs)


def test_buffers():
    """Test buffer functions."""
    buffer1 = weechat.buffer_new("test1", "buffer_input_cb", "", "buffer_close_cb", "")
    check(buffer1 != "")
    check(weechat.buffer_get_integer(buffer1, "number") == 2)
    check(weechat.buffer_get_string(buffer1, "short_name") == "test1")
    props = {
        "short_name": "t2",
    }
    buffer2 = weechat.buffer_new_props("test2", props, "buffer_input_cb", "", "buffer_close_cb", "")
    check(buffer2 != "")
    check(weechat.buffer_get_integer(buffer2, "number") == 3)
    check(weechat.buffer_get_string(buffer2, "short_name") == "t2")
    check(weechat.buffer_get_longlong(buffer2, "lines_last_id_assigned") == -1)
    weechat.prnt(buffer2, "## test line 1")
    check(weechat.buffer_get_longlong(buffer2, "lines_last_id_assigned") > 0)
    weechat.buffer_clear(buffer2)
    own_lines = weechat.hdata_pointer(weechat.hdata_get("buffer"), buffer2, "own_lines")
    check(weechat.hdata_integer(weechat.hdata_get("lines"), own_lines, "lines_count") == 0)
    check(weechat.hdata_pointer(weechat.hdata_get("lines"), own_lines, "first_line") == "")
    weechat.buffer_merge(buffer2, buffer1)
    check(weechat.buffer_get_integer(buffer1, "number") == 2)
    check(weechat.buffer_get_integer(buffer2, "number") == 2)
    weechat.buffer_unmerge(buffer2, 3)
    check(weechat.buffer_get_integer(buffer1, "number") == 2)
    check(weechat.buffer_get_integer(buffer2, "number") == 3)
    check(weechat.current_buffer() != "")
    check(weechat.buffer_get_integer(buffer1, "hidden") == 0)
    weechat.buffer_set(buffer1, "hidden", "1")
    check(weechat.buffer_get_integer(buffer1, "hidden") == 1)
    weechat.buffer_set(buffer1, "hidden", "0")
    check(weechat.buffer_get_integer(buffer1, "hidden") == 0)
    weechat.buffer_set(buffer1, "localvar_set_var1", "value1")
    check(weechat.buffer_string_replace_local_var(buffer1, "test $var1") == "test value1")
    buffer = weechat.buffer_search_main()
    buffer_id = weechat.buffer_get_string(buffer, "id")
    check(weechat.buffer_search("xxx", "yyy") == "")
    check(weechat.buffer_search("==", "xxx") == "")
    check(weechat.buffer_search("==id", "0") == "")
    check(weechat.buffer_search("core", "weechat") == buffer)
    check(weechat.buffer_search("==", "core.weechat") == buffer)
    check(weechat.buffer_search("==id", buffer_id) == buffer)
    check(weechat.buffer_match_list(buffer, "") == 0)
    check(weechat.buffer_match_list(buffer, "*") == 1)
    check(weechat.buffer_match_list(buffer, "core.weechat") == 1)
    check(weechat.buffer_match_list(buffer, "*,!core.weechat") == 0)
    weechat.buffer_close(buffer1)
    weechat.buffer_close(buffer2)


def test_nicklist():
    """Test nicklist functions."""
    buffer = weechat.buffer_new("test_nicklist", "buffer_input_cb", "", "buffer_close_cb", "")
    check(buffer != "")
    check(weechat.buffer_get_integer(buffer, "nicklist_count") == 0)
    # groups
    group1 = weechat.nicklist_add_group(buffer, "", "01|group1", "red", 1)
    check(group1 != "")
    check(weechat.nicklist_add_group(buffer, "", "01|group1", "red", 1) == "")
    group2 = weechat.nicklist_add_group(buffer, group1, "group2", "blue", 0)
    check(group2 != "")
    check(weechat.buffer_get_integer(buffer, "nicklist_groups_count") == 2)
    check(weechat.nicklist_search_group(buffer, "", "xxx") == "")
    check(weechat.nicklist_search_group(buffer, "", "01|group1") == group1)
    check(weechat.nicklist_search_group(buffer, "", "group1") == group1)
    check(weechat.nicklist_search_group(buffer, "", "group2") == group2)
    check(weechat.nicklist_search_group(buffer, group1, "group2") == group2)
    check(weechat.nicklist_group_get_integer(buffer, group1, "visible") == 1)
    check(weechat.nicklist_group_get_integer(buffer, group1, "level") == 1)
    check(weechat.nicklist_group_get_integer(buffer, group2, "visible") == 0)
    check(weechat.nicklist_group_get_integer(buffer, group2, "level") == 2)
    check(weechat.nicklist_group_get_integer(buffer, group1, "xxx") == 0)
    check(weechat.nicklist_group_get_string(buffer, group1, "name") == "01|group1")
    check(weechat.nicklist_group_get_string(buffer, group1, "color") == "red")
    check(weechat.nicklist_group_get_string(buffer, group1, "xxx") == "")
    check(weechat.nicklist_group_get_pointer(buffer, group2, "parent") == group1)
    check(weechat.nicklist_group_get_pointer(buffer, group1, "parent") != "")
    check(weechat.nicklist_group_get_pointer(buffer, group1, "xxx") == "")
    weechat.nicklist_group_set(buffer, group2, "color", "green")
    check(weechat.nicklist_group_get_string(buffer, group2, "color") == "green")
    weechat.nicklist_group_set(buffer, group2, "visible", "1")
    check(weechat.nicklist_group_get_integer(buffer, group2, "visible") == 1)
    # nicks
    nick1 = weechat.nicklist_add_nick(buffer, group1, "nick1", "cyan", "@", "lightgreen", 1)
    check(nick1 != "")
    check(weechat.nicklist_add_nick(buffer, group1, "nick1", "cyan", "@", "lightgreen", 1) == "")
    nick2 = weechat.nicklist_add_nick(buffer, group2, "nick2", "magenta", "+", "yellow", 0)
    check(nick2 != "")
    nick3 = weechat.nicklist_add_nick(buffer, "", "nick3", "brown", "", "", 1)
    check(nick3 != "")
    check(weechat.buffer_get_integer(buffer, "nicklist_nicks_count") == 3)
    check(weechat.nicklist_search_nick(buffer, "", "xxx") == "")
    check(weechat.nicklist_search_nick(buffer, "", "nick1") == nick1)
    check(weechat.nicklist_search_nick(buffer, "", "nick2") == nick2)
    check(weechat.nicklist_search_nick(buffer, group2, "nick2") == nick2)
    check(weechat.nicklist_search_nick(buffer, group2, "nick1") == "")
    check(weechat.nicklist_search_nick(buffer, "", "nick3") == nick3)
    check(weechat.nicklist_nick_get_integer(buffer, nick1, "visible") == 1)
    check(weechat.nicklist_nick_get_integer(buffer, nick2, "visible") == 0)
    check(weechat.nicklist_nick_get_integer(buffer, nick1, "xxx") == 0)
    check(weechat.nicklist_nick_get_string(buffer, nick1, "name") == "nick1")
    check(weechat.nicklist_nick_get_string(buffer, nick1, "color") == "cyan")
    check(weechat.nicklist_nick_get_string(buffer, nick1, "prefix") == "@")
    check(weechat.nicklist_nick_get_string(buffer, nick1, "prefix_color") == "lightgreen")
    check(weechat.nicklist_nick_get_string(buffer, nick1, "xxx") == "")
    check(weechat.nicklist_nick_get_pointer(buffer, nick1, "group") == group1)
    check(weechat.nicklist_nick_get_pointer(buffer, nick2, "group") == group2)
    check(weechat.nicklist_nick_get_pointer(buffer, nick3, "group") != "")
    check(weechat.nicklist_nick_get_pointer(buffer, nick1, "xxx") == "")
    weechat.nicklist_nick_set(buffer, nick2, "color", "white")
    check(weechat.nicklist_nick_get_string(buffer, nick2, "color") == "white")
    weechat.nicklist_nick_set(buffer, nick2, "prefix", "%")
    check(weechat.nicklist_nick_get_string(buffer, nick2, "prefix") == "%")
    weechat.nicklist_nick_set(buffer, nick2, "prefix_color", "lightred")
    check(weechat.nicklist_nick_get_string(buffer, nick2, "prefix_color") == "lightred")
    weechat.nicklist_nick_set(buffer, nick2, "visible", "1")
    check(weechat.nicklist_nick_get_integer(buffer, nick2, "visible") == 1)
    # remove
    weechat.nicklist_remove_nick(buffer, nick1)
    check(weechat.nicklist_search_nick(buffer, "", "nick1") == "")
    check(weechat.buffer_get_integer(buffer, "nicklist_nicks_count") == 2)
    weechat.nicklist_remove_group(buffer, group2)
    check(weechat.nicklist_search_group(buffer, "", "group2") == "")
    check(weechat.nicklist_search_nick(buffer, "", "nick2") == "")
    check(weechat.buffer_get_integer(buffer, "nicklist_groups_count") == 1)
    check(weechat.buffer_get_integer(buffer, "nicklist_nicks_count") == 1)
    weechat.nicklist_remove_all(buffer)
    check(weechat.nicklist_search_group(buffer, "", "group1") == "")
    check(weechat.nicklist_search_nick(buffer, "", "nick3") == "")
    check(weechat.buffer_get_integer(buffer, "nicklist_count") == 0)
    weechat.buffer_close(buffer)


def test_lines():
    """Test line functions."""
    buffer = weechat.buffer_search_main()
    check(weechat.line_search_by_id(buffer, -1) == "")
    check(weechat.line_search_by_id(buffer, 1234567) == "")
    lines = weechat.hdata_pointer(weechat.hdata_get("buffer"), buffer, "own_lines")
    line = weechat.hdata_pointer(weechat.hdata_get("lines"), lines, "last_line")
    line_data = weechat.hdata_pointer(weechat.hdata_get("line"), line, "data")
    line_id = weechat.hdata_longlong(weechat.hdata_get("line_data"), line_data, "id")
    check(line_id > 0)
    check(weechat.line_search_by_id(buffer, line_id) == line)


def bar_item_cb(data, item, window):
    """Bar item callback."""
    return "item"


def bar_item_extra_cb(data, item, window, buf, extra_info):
    """Bar item callback (with buffer and extra info)."""
    return "item_extra"


def test_bars():
    """Test bar and bar item functions."""
    # bar items
    check(weechat.bar_item_search("test_item") == "")
    item1 = weechat.bar_item_new("test_item", "bar_item_cb", "bar_item_data")
    check(item1 != "")
    check(weechat.bar_item_new("test_item", "bar_item_cb", "bar_item_data") == "")
    item2 = weechat.bar_item_new("(extra)test_item_extra", "bar_item_extra_cb", "bar_item_data")
    check(item2 != "")
    check(weechat.bar_item_search("test_item") == item1)
    check(weechat.bar_item_search("test_item_extra") == item2)
    check(weechat.bar_item_search("(extra)test_item_extra") == "")
    weechat.bar_item_update("test_item")
    weechat.bar_item_update("test_item_extra")
    # bars
    check(weechat.bar_search("test_bar") == "")
    check(
        weechat.bar_new(
            "test_bar",
            "off",
            "100",
            "xxx",
            "",
            "top",
            "horizontal",
            "vertical",
            "0",
            "5",
            "default",
            "cyan",
            "blue",
            "darkgray",
            "off",
            "test_item,test_item_extra",
        )
        == ""
    )
    check(
        weechat.bar_new(
            "test_bar",
            "off",
            "100",
            "window",
            "",
            "xxx",
            "horizontal",
            "vertical",
            "0",
            "5",
            "default",
            "cyan",
            "blue",
            "darkgray",
            "off",
            "test_item,test_item_extra",
        )
        == ""
    )
    bar = weechat.bar_new(
        "test_bar",
        "off",
        "100",
        "window",
        "",
        "top",
        "horizontal",
        "vertical",
        "0",
        "5",
        "default",
        "cyan",
        "blue",
        "darkgray",
        "off",
        "test_item,test_item_extra",
    )
    check(bar != "")
    check(weechat.bar_search("test_bar") == bar)
    check(weechat.config_string(weechat.config_get("weechat.bar.test_bar.type")) == "window")
    check(weechat.config_string(weechat.config_get("weechat.bar.test_bar.position")) == "top")
    check(weechat.config_integer(weechat.config_get("weechat.bar.test_bar.size_max")) == 5)
    check(weechat.config_string(weechat.config_get("weechat.bar.test_bar.items")) == "test_item,test_item_extra")
    bar2 = weechat.bar_new(
        "test_bar",
        "off",
        "100",
        "window",
        "",
        "bottom",
        "horizontal",
        "vertical",
        "0",
        "5",
        "default",
        "cyan",
        "blue",
        "darkgray",
        "off",
        "test_item,test_item_extra",
    )
    check(bar2 == bar)
    check(weechat.config_string(weechat.config_get("weechat.bar.test_bar.position")) == "top")
    check(weechat.config_string_default(weechat.config_get("weechat.bar.test_bar.position")) == "bottom")
    check(weechat.bar_set(bar, "xxx", "value") == 0)
    check(weechat.bar_set(bar, "position", "left") == 1)
    check(weechat.config_string(weechat.config_get("weechat.bar.test_bar.position")) == "left")
    check(weechat.bar_set(bar, "size", "2") == 1)
    check(weechat.config_integer(weechat.config_get("weechat.bar.test_bar.size")) == 2)
    check(weechat.bar_set(bar, "hidden", "on") == 1)
    check(weechat.config_boolean(weechat.config_get("weechat.bar.test_bar.hidden")) == 1)
    check(weechat.bar_set(bar, "items", "test_item") == 1)
    check(weechat.config_string(weechat.config_get("weechat.bar.test_bar.items")) == "test_item")
    weechat.bar_update("test_bar")
    weechat.bar_remove(bar)
    check(weechat.bar_search("test_bar") == "")
    check(weechat.config_get("weechat.bar.test_bar.position") == "")
    weechat.bar_item_remove(item1)
    weechat.bar_item_remove(item2)
    check(weechat.bar_item_search("test_item") == "")
    check(weechat.bar_item_search("test_item_extra") == "")


def test_completion():
    """Test completion functions."""
    buffer = weechat.buffer_search_main()
    hdata = weechat.hdata_get("completion")
    completion = weechat.completion_new(buffer)
    check(completion != "")
    check(weechat.hdata_pointer(hdata, completion, "buffer") == buffer)
    check(weechat.completion_search(completion, "/help filt", -1, 1) == 0)
    # command
    check(weechat.completion_search(completion, "/filt", 5, 1) == 1)
    check(weechat.completion_get_string(completion, "base_command") == "")
    check(weechat.completion_get_string(completion, "base_word") == "filt")
    check(weechat.hdata_string(hdata, completion, "word_found") == "filter")
    check(weechat.hdata_integer(hdata, completion, "position_replace") == 1)
    # command argument
    check(weechat.completion_search(completion, "/help filt", 10, 1) == 1)
    check(weechat.completion_get_string(completion, "base_command") == "help")
    check(weechat.completion_get_string(completion, "base_word") == "filt")
    check(weechat.completion_get_string(completion, "args") == "filt")
    check(weechat.completion_get_string(completion, "xxx") == "")
    check(weechat.hdata_string(hdata, completion, "word_found") == "filter")
    check(weechat.hdata_integer(hdata, completion, "position_replace") == 6)
    weechat.completion_free(completion)


def test_windows():
    """Test window functions."""
    window = weechat.current_window()
    check(window != "")
    buffer = weechat.buffer_search_main()
    check(weechat.window_search_with_buffer(buffer) != "")
    buffer1 = weechat.buffer_new("test1", "buffer_input_cb", "", "buffer_close_cb", "")
    check(buffer1 != "")
    check(weechat.window_search_with_buffer(buffer1) == "")
    weechat.buffer_close(buffer1)
    check(weechat.window_search_with_buffer(buffer) == window)
    check(weechat.window_get_integer(window, "number") == 1)
    check(weechat.window_get_integer(window, "win_x") == 0)
    check(weechat.window_get_integer(window, "win_y") == 0)
    check(weechat.window_get_integer(window, "win_width") > 0)
    check(weechat.window_get_integer(window, "win_height") > 0)
    check(weechat.window_get_integer(window, "win_width_pct") == 100)
    check(weechat.window_get_integer(window, "win_height_pct") == 100)
    check(weechat.window_get_integer(window, "scrolling") == 0)
    check(weechat.window_get_integer(window, "xxx") == 0)
    check(weechat.window_get_string(window, "xxx") == "")
    check(weechat.window_get_pointer(window, "current") == window)
    check(weechat.window_get_pointer("", "current") == window)
    check(weechat.window_get_pointer(window, "buffer") == buffer)
    check(weechat.window_get_pointer("", "buffer") == "")
    check(weechat.window_get_pointer(window, "xxx") == "")
    # Empty title: no-op (a title would be sent to the terminal running the tests).
    weechat.window_set_title("")


def test_command():
    """Test command functions."""
    check(weechat.command("", "/mute") == 0)
    check(weechat.command_options("", "/mute", {"commands": "*,!print"}) == 0)
    check(weechat.command_options("", "/mute", {"commands": "*,!mute"}) == -1)


def infolist_cb(data, infolist_name, pointer, arguments):
    """Infolist callback."""
    infolist = weechat.infolist_new()
    check(infolist != "")
    item = weechat.infolist_new_item(infolist)
    check(item != "")
    check(weechat.infolist_new_var_integer(item, "integer", 123) != "")
    check(weechat.infolist_new_var_string(item, "string", "test string") != "")
    check(weechat.infolist_new_var_pointer(item, "pointer", "0xabcdef") != "")
    # Tue Jan 06 2009 08:40:30 GMT+0000
    check(weechat.infolist_new_var_time(item, "time1", 1231231230) != "")
    # Wed Jan 06 2038 09:40:00 GMT+0000
    check(weechat.infolist_new_var_time(item, "time2", 2146383600) != "")
    check(weechat.infolist_new_var_long(item, "long", 2123456789) != "")
    check(weechat.infolist_new_var_longlong(item, "longlong", 9123456789123456789) != "")
    return infolist


def test_infolist():
    """Test infolist functions."""
    hook_infolist = weechat.hook_infolist("infolist_test_script", "description", "", "", "infolist_cb", "")
    check(weechat.infolist_get("infolist_does_not_exist", "", "") == "")
    ptr_infolist = weechat.infolist_get("infolist_test_script", "", "")
    check(ptr_infolist != "")
    check(weechat.infolist_next(ptr_infolist) == 1)
    check(weechat.infolist_integer(ptr_infolist, "integer") == 123)
    check(weechat.infolist_string(ptr_infolist, "string") == "test string")
    check(weechat.infolist_pointer(ptr_infolist, "pointer") == "0xabcdef")
    check(weechat.infolist_time(ptr_infolist, "time1") == 1231231230)
    check(weechat.infolist_time(ptr_infolist, "time2") == 2146383600)
    check(weechat.infolist_long(ptr_infolist, "long") == 2123456789)
    check(weechat.infolist_longlong(ptr_infolist, "longlong") == 9123456789123456789)
    check(weechat.infolist_fields(ptr_infolist) == "i:integer,s:string,p:pointer,t:time1,t:time2,l:long,L:longlong")
    check(weechat.infolist_next(ptr_infolist) == 0)
    weechat.infolist_free(ptr_infolist)
    weechat.unhook(hook_infolist)
    # Infolist with two items.
    infolist = weechat.infolist_new()
    item = weechat.infolist_new_item(infolist)
    weechat.infolist_new_var_integer(item, "integer", 1)
    weechat.infolist_new_var_string(item, "string", "item1")
    item = weechat.infolist_new_item(infolist)
    weechat.infolist_new_var_integer(item, "integer", 2)
    weechat.infolist_new_var_string(item, "string", "item2")
    check(weechat.infolist_search_var(infolist, "integer") == "")
    check(weechat.infolist_prev(infolist) == 1)
    check(weechat.infolist_string(infolist, "string") == "item2")
    check(weechat.infolist_prev(infolist) == 1)
    check(weechat.infolist_string(infolist, "string") == "item1")
    check(weechat.infolist_prev(infolist) == 0)
    check(weechat.infolist_next(infolist) == 1)
    check(weechat.infolist_integer(infolist, "integer") == 1)
    check(weechat.infolist_search_var(infolist, "integer") != "")
    check(weechat.infolist_search_var(infolist, "string") != "")
    check(weechat.infolist_search_var(infolist, "xxx") == "")
    check(weechat.infolist_next(infolist) == 1)
    check(weechat.infolist_integer(infolist, "integer") == 2)
    weechat.infolist_reset_item_cursor(infolist)
    check(weechat.infolist_search_var(infolist, "integer") == "")
    check(weechat.infolist_next(infolist) == 1)
    check(weechat.infolist_integer(infolist, "integer") == 1)
    weechat.infolist_reset_item_cursor(infolist)
    check(weechat.infolist_prev(infolist) == 1)
    check(weechat.infolist_integer(infolist, "integer") == 2)
    weechat.infolist_free(infolist)


def upgrade_read_cb(data, upgrade_file, object_id, infolist):
    """Read an object from the upgrade file."""
    check(data == "upgrade_read_data")
    check(upgrade_file != "")
    check(object_id == 1)
    check(weechat.infolist_next(infolist) == 1)
    check(weechat.infolist_integer(infolist, "integer") == 123)
    check(weechat.infolist_string(infolist, "string") == "test string")
    check(weechat.infolist_time(infolist, "time") == 1231231230)
    check(weechat.infolist_next(infolist) == 0)
    return weechat.WEECHAT_RC_OK


def test_upgrade():
    """Test upgrade functions."""
    check(weechat.upgrade_new("testapi_upgrade_missing", "upgrade_read_cb", "upgrade_read_data") == "")
    # Write the upgrade file.
    upgrade_file = weechat.upgrade_new("testapi_upgrade", "", "")
    check(upgrade_file != "")
    infolist = weechat.infolist_new()
    item = weechat.infolist_new_item(infolist)
    weechat.infolist_new_var_integer(item, "integer", 123)
    weechat.infolist_new_var_string(item, "string", "test string")
    weechat.infolist_new_var_time(item, "time", 1231231230)
    check(weechat.upgrade_write_object("", 1, infolist) == 0)
    check(weechat.upgrade_write_object(upgrade_file, 1, infolist) == 1)
    weechat.infolist_free(infolist)
    check(weechat.upgrade_read("") == 0)
    weechat.upgrade_close(upgrade_file)
    # Read the upgrade file (the callback is called once, for the object).
    upgrade_file = weechat.upgrade_new("testapi_upgrade", "upgrade_read_cb", "upgrade_read_data")
    check(upgrade_file != "")
    check(weechat.upgrade_read(upgrade_file) == 1)
    weechat.upgrade_close(upgrade_file)


def test_hdata():
    """Test hdata functions."""
    buffer = weechat.buffer_search_main()
    # Get hdata.
    hdata_buffer = weechat.hdata_get("buffer")
    check(hdata_buffer != "")
    hdata_lines = weechat.hdata_get("lines")
    check(hdata_lines != "")
    hdata_line = weechat.hdata_get("line")
    check(hdata_line != "")
    hdata_line_data = weechat.hdata_get("line_data")
    check(hdata_line_data != "")
    hdata_key = weechat.hdata_get("key")
    check(hdata_key != "")
    hdata_hotlist = weechat.hdata_get("hotlist")
    check(hdata_hotlist != "")
    hdata_irc_server = weechat.hdata_get("irc_server")
    check(hdata_irc_server != "")
    # Create a test buffer with 3 messages.
    buffer2 = weechat.buffer_new("test", "buffer_input_cb", "", "buffer_close_cb", "")
    weechat.prnt_date_tags(buffer2, 2146383600, "tag1,tag2", "prefix1\t## msg1")
    weechat.prnt_date_tags(buffer2, 2146383601, "tag3,tag4", "prefix2\t## msg2")
    weechat.prnt_date_tags(buffer2, 2146383602, "tag5,tag6", "prefix3\t## msg3")
    own_lines = weechat.hdata_pointer(hdata_buffer, buffer2, "own_lines")
    line1 = weechat.hdata_pointer(hdata_lines, own_lines, "first_line")
    line1_data = weechat.hdata_pointer(hdata_line, line1, "data")
    line2 = weechat.hdata_pointer(hdata_line, line1, "next_line")
    line3 = weechat.hdata_pointer(hdata_line, line2, "next_line")
    # hdata_get_var_offset
    check(weechat.hdata_get_var_offset(hdata_buffer, "id") == 0)
    check(weechat.hdata_get_var_offset(hdata_buffer, "plugin") > 0)
    # hdata_get_var_type_string
    check(weechat.hdata_get_var_type_string(hdata_buffer, "plugin") == "pointer")
    check(weechat.hdata_get_var_type_string(hdata_buffer, "number") == "integer")
    check(weechat.hdata_get_var_type_string(hdata_buffer, "name") == "string")
    check(weechat.hdata_get_var_type_string(hdata_buffer, "local_variables") == "hashtable")
    check(weechat.hdata_get_var_type_string(hdata_line_data, "displayed") == "char")
    check(weechat.hdata_get_var_type_string(hdata_line_data, "prefix") == "shared_string")
    check(weechat.hdata_get_var_type_string(hdata_line_data, "date") == "time")
    check(weechat.hdata_get_var_type_string(hdata_hotlist, "time") == "time")
    check(weechat.hdata_get_var_type_string(hdata_hotlist, "time_usec") == "long")
    check(weechat.hdata_get_var_type_string(hdata_irc_server, "sasl_scram_salted_pwd") == "other")
    # hdata_get_var_array_size
    check(weechat.hdata_get_var_array_size(hdata_buffer, buffer2, "name") == -1)
    check(weechat.hdata_get_var_array_size(hdata_buffer, buffer2, "highlight_tags_array") >= 0)
    # hdata_get_var_array_size_string
    check(weechat.hdata_get_var_array_size_string(hdata_buffer, buffer2, "name") == "")
    check(
        weechat.hdata_get_var_array_size_string(hdata_buffer, buffer2, "highlight_tags_array") == "highlight_tags_count"
    )
    # hdata_get_var_hdata
    check(weechat.hdata_get_var_hdata(hdata_buffer, "plugin") == "plugin")
    check(weechat.hdata_get_var_hdata(hdata_buffer, "own_lines") == "lines")
    check(weechat.hdata_get_var_hdata(hdata_buffer, "name") == "")
    # hdata_get_list
    check(weechat.hdata_get_list(hdata_buffer, "gui_buffers") == buffer)
    # hdata_check_pointer
    check(weechat.hdata_check_pointer(hdata_buffer, buffer, buffer) == 1)
    check(weechat.hdata_check_pointer(hdata_buffer, buffer, buffer2) == 1)
    check(weechat.hdata_check_pointer(hdata_buffer, buffer, own_lines) == 0)
    # hdata_move
    check(weechat.hdata_move(hdata_line, line1, 1) == line2)
    check(weechat.hdata_move(hdata_line, line1, 2) == line3)
    check(weechat.hdata_move(hdata_line, line3, -1) == line2)
    check(weechat.hdata_move(hdata_line, line3, -2) == line1)
    check(weechat.hdata_move(hdata_line, line1, -1) == "")
    # hdata_search
    check(weechat.hdata_search(hdata_buffer, buffer, "${name} == test", {}, {}, {}, 1) == buffer2)
    check(weechat.hdata_search(hdata_buffer, buffer, "${name} == xxx", {}, {}, {}, 1) == "")
    # hdata_char
    check(weechat.hdata_char(hdata_line_data, line1_data, "displayed") == 1)
    # hdata_integer
    check(weechat.hdata_integer(hdata_buffer, buffer2, "number") == 2)
    # hdata_long
    weechat.buffer_set(buffer, "hotlist", weechat.WEECHAT_HOTLIST_MESSAGE)
    gui_hotlist = weechat.hdata_get_list(hdata_hotlist, "gui_hotlist")
    check(weechat.hdata_long(hdata_hotlist, gui_hotlist, "creation_time.tv_usec") >= 0)
    # hdata_longlong
    check(weechat.hdata_longlong(hdata_buffer, buffer2, "id") > 1708874542000000)
    # hdata_string
    check(weechat.hdata_string(hdata_buffer, buffer2, "name") == "test")
    # hdata_pointer
    check(weechat.hdata_pointer(hdata_buffer, buffer2, "own_lines") == own_lines)
    # hdata_time
    check(weechat.hdata_time(hdata_line_data, line1_data, "date") > 1659430030)
    # hdata_hashtable
    local_vars = weechat.hdata_hashtable(hdata_buffer, buffer2, "local_variables")
    value = local_vars["name"]
    check(value == "test")
    # hdata_compare
    check(weechat.hdata_compare(hdata_buffer, buffer, buffer2, "name", 0) > 0)
    check(weechat.hdata_compare(hdata_buffer, buffer2, buffer, "name", 0) < 0)
    check(weechat.hdata_compare(hdata_buffer, buffer, buffer, "name", 0) == 0)
    # hdata_update
    check(weechat.hdata_time(hdata_line_data, line1_data, "date") == 2146383600)
    check(weechat.hdata_string(hdata_line_data, line1_data, "prefix") == "prefix1")
    check(weechat.hdata_string(hdata_line_data, line1_data, "message") == "## msg1")
    update = {
        "date": "2146383605",
        "prefix": "new_prefix1",
        "message": "new_message1",
    }
    check(weechat.hdata_update(hdata_line_data, line1_data, update) == 3)
    check(weechat.hdata_time(hdata_line_data, line1_data, "date") == 2146383605)
    check(weechat.hdata_string(hdata_line_data, line1_data, "prefix") == "new_prefix1")
    check(weechat.hdata_string(hdata_line_data, line1_data, "message") == "new_message1")
    # hdata_get_string
    check(weechat.hdata_get_string(hdata_line, "var_prev") == "prev_line")
    check(weechat.hdata_get_string(hdata_line, "var_next") == "next_line")
    # Destroy test buffer.
    weechat.buffer_close(buffer2)


def unhook_all_signal_cb(data, signal, signal_data):
    """Eat the signal (callback used to check if the hook exists)."""
    return weechat.WEECHAT_RC_OK_EAT


def test_unhook_all():
    """Test functions hook_set (subplugin) and unhook_all."""
    # Get the hook of the command running the tests.
    infolist = weechat.infolist_get("hook", "", "command," + "{SCRIPT_NAME}")
    check(weechat.infolist_next(infolist) == 1)
    hook_cmd = weechat.infolist_pointer(infolist, "pointer")
    check(hook_cmd != "")
    check(weechat.infolist_string(infolist, "subplugin") == "{SCRIPT_NAME}")
    weechat.infolist_free(infolist)
    # hook_set: move the command hook to another subplugin, so that it is not
    # removed by unhook_all (it is running).
    weechat.hook_set(hook_cmd, "subplugin", "testapi_subplugin")
    infolist = weechat.infolist_get("hook", "", "command," + "{SCRIPT_NAME}")
    check(weechat.infolist_next(infolist) == 1)
    check(weechat.infolist_string(infolist, "subplugin") == "testapi_subplugin")
    weechat.infolist_free(infolist)
    # unhook_all
    signal = "{SCRIPT_NAME}_unhook_all"
    hook_sig = weechat.hook_signal(signal, "unhook_all_signal_cb", "")
    check(hook_sig != "")
    check(weechat.hook_signal_send(signal, weechat.WEECHAT_HOOK_SIGNAL_STRING, "") == weechat.WEECHAT_RC_OK_EAT)
    weechat.unhook_all()
    check(weechat.hook_signal_send(signal, weechat.WEECHAT_HOOK_SIGNAL_STRING, "") == weechat.WEECHAT_RC_OK)
    # The command hook (other subplugin) is still there.
    infolist = weechat.infolist_get("hook", "", "command," + "{SCRIPT_NAME}")
    check(weechat.infolist_next(infolist) == 1)
    check(weechat.infolist_pointer(infolist, "pointer") == hook_cmd)
    weechat.infolist_free(infolist)
    # Restore the subplugin, so that the hook is removed when the script is unloaded.
    weechat.hook_set(hook_cmd, "subplugin", "{SCRIPT_NAME}")


def test_hooks_process():
    """Test functions hook_process and hook_process_hashtable.

    The callbacks are called later, by the main loop, when the processes have
    ended (the hooks are then automatically removed), so this function must be
    called after test_unhook_all (which would remove the hooks).
    """
    hook_proc = weechat.hook_process("sh -c 'echo test; echo error >&2; exit 3'", 10000, "process_cb", "process_data")
    check(hook_proc != "")
    hook_proc_hashtable = weechat.hook_process_hashtable(
        "sh",
        {"arg1": "-c", "arg2": "echo test hashtable"},
        10000,
        "process_hashtable_cb",
        "process_hashtable_data",
    )
    check(hook_proc_hashtable != "")


def test_hooks_url():
    """Test function hook_url.

    The callbacks are called later, by the main loop, when the transfers have
    ended (the hooks are then automatically removed), so this function must be
    called after test_unhook_all (which would remove the hooks).

    The URL is a local file written by test_config_write.
    """
    config_dir = weechat.info_get("weechat_config_dir", "")
    url = "file://" + config_dir + "/test_config_write_" + "{SCRIPT_LANGUAGE}" + ".conf"
    hook_url1 = weechat.hook_url(url, {}, 10000, "url_cb", "url_data")
    check(hook_url1 != "")
    url_missing = "file://" + config_dir + "/test_url_missing.conf"
    hook_url2 = weechat.hook_url(url_missing, {}, 10000, "url_error_cb", "url_error_data")
    check(hook_url2 != "")


def test_hooks_connect():
    """Test function hook_connect.

    The callbacks are called later, by the main loop, when the connections
    are done (the hooks are then automatically removed), so this function must
    be called after test_unhook_all (which would remove the hooks).

    The ports (on 127.0.0.1) are given by the C++ test in environment
    variables: one is listening (connection OK), the other is not (connection
    refused); function string_parse_size is used to convert them to integers.
    """
    port = weechat.string_parse_size(weechat.string_eval_expression("${env:WEECHAT_TESTS_CONNECT_PORT}", {}, {}, {}))
    check(port > 0)
    hook_conn1 = weechat.hook_connect(
        "", "127.0.0.1", port, weechat.WEECHAT_HOOK_CONNECT_IPV6_DISABLE, 0, "", "connect_cb", "connect_data"
    )
    check(hook_conn1 != "")
    port_refused = weechat.string_parse_size(
        weechat.string_eval_expression("${env:WEECHAT_TESTS_CONNECT_PORT_REFUSED}", {}, {}, {})
    )
    check(port_refused > 0)
    hook_conn2 = weechat.hook_connect(
        "",
        "127.0.0.1",
        port_refused,
        weechat.WEECHAT_HOOK_CONNECT_IPV6_DISABLE,
        0,
        "",
        "connect_refused_cb",
        "connect_refused_data",
    )
    check(hook_conn2 != "")


def test_hooks_fd():
    """Test function hook_fd.

    The callback is called later, by the main loop, so this function must be
    called after test_unhook_all (which would remove the hook).

    The file descriptor is the read end of a pipe given by the C++ test in an
    environment variable (with data that is never read, so it is always ready
    for reading); the hook is saved in a local variable of core buffer, so
    that the callback can remove it.
    """
    fd = weechat.string_parse_size(weechat.string_eval_expression("${env:WEECHAT_TESTS_FD}", {}, {}, {}))
    check(fd > 0)
    hook_fd = weechat.hook_fd(fd, 1, 0, 0, "fd_cb", "fd_data")
    check(hook_fd != "")
    weechat.buffer_set(weechat.buffer_search_main(), "localvar_set_testapi_hook_fd", hook_fd)


def cmd_test_cb(data, buf, args):
    """Run all the tests."""
    weechat.prnt("", ">>>")
    weechat.prnt("", ">>> ------------------------------")
    weechat.prnt("", ">>> Testing " + "{SCRIPT_LANGUAGE}" + " API")
    weechat.prnt("", "  > TESTS: " + "{SCRIPT_TESTS}")
    test_constants()
    test_plugins()
    test_strings()
    test_dir()
    test_lists()
    test_config()
    test_config_write()
    test_key()
    test_display()
    test_theme()
    test_hooks()
    test_buffers()
    test_nicklist()
    test_lines()
    test_bars()
    test_completion()
    test_windows()
    test_command()
    test_infolist()
    test_upgrade()
    test_hdata()
    test_unhook_all()
    test_hooks_process()
    test_hooks_url()
    test_hooks_connect()
    test_hooks_fd()
    weechat.prnt("", "  > TESTS END")
    return weechat.WEECHAT_RC_OK


def weechat_init():
    """Initialize script."""
    weechat.register(
        "{SCRIPT_NAME}", "{SCRIPT_AUTHOR}", "{SCRIPT_VERSION}", "{SCRIPT_LICENSE}", "{SCRIPT_DESCRIPTION}", "", ""
    )
    weechat.hook_command("{SCRIPT_NAME}", "", "", "", "", "cmd_test_cb", "")

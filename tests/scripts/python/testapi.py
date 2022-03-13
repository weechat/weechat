# -*- coding: utf-8 -*-
#
# Copyright (C) 2017-2022 Sébastien Helleu <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

"""
This script contains WeeChat scripting API tests
(it can not be run directly and can not be loaded in WeeChat).

It is parsed by testapigen.py, using Python AST (Abstract Syntax Trees),
to generate scripts in all supported languages (Python, Perl, Ruby, ...).
The resulting scripts can be loaded in WeeChat to test the scripting API.
"""

# pylint: disable=line-too-long,no-value-for-parameter

import weechat  # pylint: disable=import-error


def check(result, condition, lineno):
    """Display the result of a test."""
    if result:
        weechat.prnt('', '      TEST OK: ' + condition)
    else:
        weechat.prnt('',
                     'SCRIPT_SOURCE' + ':' + lineno + ':1: ' +
                     'ERROR: [' + 'SCRIPT_NAME' + '] condition is false: ' +
                     condition)


def test_plugins():
    """Test plugins functions."""
    check(weechat.plugin_get_name('') == 'core')
    check(weechat.plugin_get_name(weechat.buffer_get_pointer(weechat.buffer_search_main(), 'plugin')) == 'core')


def test_strings():
    """Test string functions."""
    check(weechat.charset_set('iso-8859-15') == 1)
    check(weechat.charset_set('') == 1)
    check(weechat.iconv_to_internal('iso-8859-15', 'abc') == 'abc')
    check(weechat.iconv_from_internal('iso-8859-15', 'abcd') == 'abcd')
    check(weechat.gettext('abcdef') == 'abcdef')
    check(weechat.ngettext('file', 'files', 1) == 'file')
    check(weechat.ngettext('file', 'files', 2) == 'files')
    check(weechat.strlen_screen('abcd') == 4)
    check(weechat.string_match('abcdef', 'abc*', 0) == 1)
    check(weechat.string_match('abcdef', 'abc*', 1) == 1)
    check(weechat.string_match('ABCDEF', 'abc*', 1) == 0)
    check(weechat.string_match_list('abcdef', '*,!abc*', 0) == 0)
    check(weechat.string_match_list('ABCDEF', '*,!abc*', 1) == 1)
    check(weechat.string_match_list('def', '*,!abc*', 0) == 1)
    check(weechat.string_eval_path_home('test ${abc}', {}, {'abc': '123'}, {}) == 'test 123')
    check(weechat.string_mask_to_regex('test*mask') == 'test.*mask')
    check(weechat.string_has_highlight('my test string', 'test,word2') == 1)
    check(weechat.string_has_highlight_regex('my test string', 'test|word2') == 1)
    check(weechat.string_format_size(0) == '0 bytes')
    check(weechat.string_format_size(1) == '1 byte')
    check(weechat.string_format_size(2097152) == '2.10 MB')
    check(weechat.string_format_size(420000000) == '420.00 MB')
    check(weechat.string_color_code_size('') == 0)
    check(weechat.string_color_code_size('test') == 0)
    str_color = weechat.color('yellow,red')
    check(weechat.string_color_code_size(str_color) == 7)
    check(weechat.string_remove_color('test', '?') == 'test')
    check(weechat.string_is_command_char('/test') == 1)
    check(weechat.string_is_command_char('test') == 0)
    check(weechat.string_input_for_buffer('test') == 'test')
    check(weechat.string_input_for_buffer('/test') == '')
    check(weechat.string_input_for_buffer('//test') == '/test')
    check(weechat.string_eval_expression("100 > 50", {}, {}, {"type": "condition"}) == '1')
    check(weechat.string_eval_expression("-50 < 100", {}, {}, {"type": "condition"}) == '1')
    check(weechat.string_eval_expression("18.2 > 5", {}, {}, {"type": "condition"}) == '1')
    check(weechat.string_eval_expression("0xA3 > 2", {}, {}, {"type": "condition"}) == '1')
    check(weechat.string_eval_expression("${buffer.full_name}", {}, {}, {}) == 'core.weechat')


def test_lists():
    """Test list functions."""
    ptr_list = weechat.list_new()
    check(ptr_list != '')
    check(weechat.list_size(ptr_list) == 0)
    item_def = weechat.list_add(ptr_list, 'def', weechat.WEECHAT_LIST_POS_SORT, '')
    check(weechat.list_size(ptr_list) == 1)
    item_abc = weechat.list_add(ptr_list, 'abc', weechat.WEECHAT_LIST_POS_SORT, '')
    check(weechat.list_size(ptr_list) == 2)
    check(weechat.list_search(ptr_list, 'abc') == item_abc)
    check(weechat.list_search(ptr_list, 'def') == item_def)
    check(weechat.list_search(ptr_list, 'ghi') == '')
    check(weechat.list_search_pos(ptr_list, 'abc') == 0)
    check(weechat.list_search_pos(ptr_list, 'def') == 1)
    check(weechat.list_search_pos(ptr_list, 'ghi') == -1)
    check(weechat.list_casesearch(ptr_list, 'abc') == item_abc)
    check(weechat.list_casesearch(ptr_list, 'def') == item_def)
    check(weechat.list_casesearch(ptr_list, 'ghi') == '')
    check(weechat.list_casesearch(ptr_list, 'ABC') == item_abc)
    check(weechat.list_casesearch(ptr_list, 'DEF') == item_def)
    check(weechat.list_casesearch(ptr_list, 'GHI') == '')
    check(weechat.list_casesearch_pos(ptr_list, 'abc') == 0)
    check(weechat.list_casesearch_pos(ptr_list, 'def') == 1)
    check(weechat.list_casesearch_pos(ptr_list, 'ghi') == -1)
    check(weechat.list_casesearch_pos(ptr_list, 'ABC') == 0)
    check(weechat.list_casesearch_pos(ptr_list, 'DEF') == 1)
    check(weechat.list_casesearch_pos(ptr_list, 'GHI') == -1)
    check(weechat.list_get(ptr_list, 0) == item_abc)
    check(weechat.list_get(ptr_list, 1) == item_def)
    check(weechat.list_get(ptr_list, 2) == '')
    weechat.list_set(item_def, 'def2')
    check(weechat.list_string(item_def) == 'def2')
    check(weechat.list_next(item_abc) == item_def)
    check(weechat.list_next(item_def) == '')
    check(weechat.list_prev(item_abc) == '')
    check(weechat.list_prev(item_def) == item_abc)
    weechat.list_remove(ptr_list, item_abc)
    check(weechat.list_size(ptr_list) == 1)
    check(weechat.list_get(ptr_list, 0) == item_def)
    check(weechat.list_get(ptr_list, 1) == '')
    weechat.list_remove_all(ptr_list)
    check(weechat.list_size(ptr_list) == 0)
    weechat.list_free(ptr_list)


def test_key():
    """Test key functions."""
    check(
        weechat.key_bind(
            'mouse',
            {
                '@chat(plugin.test):button1': 'hsignal:test_mouse',
                '@chat(plugin.test):wheelup': '/mycommand up',
                '@chat(plugin.test):wheeldown': '/mycommand down',
                '__quiet': '',
            }
        ) == 3)
    check(weechat.key_unbind('mouse', 'quiet:area:chat(plugin.test)') == 3)


def buffer_input_cb(data, buffer, input_data):
    """Buffer input callback."""
    return weechat.WEECHAT_RC_OK


def buffer_close_cb(data, buffer):
    """Buffer close callback."""
    return weechat.WEECHAT_RC_OK


def test_display():
    """Test display functions."""
    check(weechat.prefix('action') != '')
    check(weechat.prefix('error') != '')
    check(weechat.prefix('join') != '')
    check(weechat.prefix('network') != '')
    check(weechat.prefix('quit') != '')
    check(weechat.prefix('unknown') == '')
    check(weechat.color('green') != '')
    check(weechat.color('unknown') == '')
    weechat.prnt('', '## test print core buffer')
    weechat.prnt_date_tags('', 946681200, 'tag1,tag2', '## test print_date_tags core buffer')
    buffer = weechat.buffer_new('test_formatted', 'buffer_input_cb', '', 'buffer_close_cb', '')
    check(buffer != '')
    check(weechat.buffer_get_integer(buffer, 'type') == 0)
    weechat.prnt(buffer, '## test print formatted buffer')
    weechat.prnt_date_tags(buffer, 946681200, 'tag1,tag2', '## test print_date_tags formatted buffer')
    weechat.buffer_close(buffer)
    buffer = weechat.buffer_new_props('test_free', {'type': 'free'}, 'buffer_input_cb', '', 'buffer_close_cb', '')
    check(weechat.buffer_get_integer(buffer, 'type') == 1)
    check(buffer != '')
    weechat.prnt_y(buffer, 0, '## test print_y free buffer')
    weechat.prnt_y_date_tags(buffer, 0, 946681200, 'tag1,tag2', '## test print_y_date_tags free buffer')
    weechat.buffer_close(buffer)


def completion_cb(data, completion_item, buf, completion):
    """Completion callback."""
    check(data == 'completion_data')
    check(completion_item == 'SCRIPT_NAME')
    check(weechat.completion_get_string(completion, 'args') == 'w')
    weechat.completion_list_add(completion, 'word_completed',
                                0, weechat.WEECHAT_LIST_POS_END)
    return weechat.WEECHAT_RC_OK


def command_cb(data, buf, args):
    """Command callback."""
    check(data == 'command_data')
    check(args == 'word_completed')
    return weechat.WEECHAT_RC_OK


def command_run_cb(data, buf, command):
    """Command_run callback."""
    check(data == 'command_run_data')
    check(command == '/cmd' + 'SCRIPT_NAME' + ' word_completed')
    return weechat.WEECHAT_RC_OK


def test_hooks():
    """Test function hook_command."""
    # hook_completion / hook_completion_args / and hook_command
    hook_cmplt = weechat.hook_completion('SCRIPT_NAME', 'description',
                                         'completion_cb', 'completion_data')
    hook_cmd = weechat.hook_command('cmd' + 'SCRIPT_NAME', 'description',
                                    'arguments', 'description arguments',
                                    '%(' + 'SCRIPT_NAME' + ')',
                                    'command_cb', 'command_data')
    weechat.command('', '/input insert /cmd' + 'SCRIPT_NAME' + ' w')
    weechat.command('', '/input complete_next')
    # hook_command_run
    hook_cmd_run = weechat.hook_command_run('/cmd' + 'SCRIPT_NAME' + '*',
                                            'command_run_cb', 'command_run_data')
    weechat.command('', '/input return')
    weechat.unhook(hook_cmd_run)
    weechat.unhook(hook_cmd)
    weechat.unhook(hook_cmplt)


def test_command():
    """Test command functions."""
    check(weechat.command('', '/mute') == 0)
    check(weechat.command_options('', '/mute', {'commands': '*,!print'}) == 0)
    check(weechat.command_options('', '/mute', {'commands': '*,!mute'}) == -1)


def infolist_cb(data, infolist_name, pointer, arguments):
    """Infolist callback."""
    infolist = weechat.infolist_new()
    check(infolist != '')
    item = weechat.infolist_new_item(infolist)
    check(item != '')
    check(weechat.infolist_new_var_integer(item, 'integer', 123) != '')
    check(weechat.infolist_new_var_string(item, 'string', 'test string') != '')
    check(weechat.infolist_new_var_pointer(item, 'pointer', '0xabcdef') != '')
    check(weechat.infolist_new_var_time(item, 'time', 1231231230) != '')
    return infolist


def test_infolist():
    """Test infolist functions."""
    hook_infolist = weechat.hook_infolist('infolist_test_script',
                                          'description', '', '',
                                          'infolist_cb', '')
    check(weechat.infolist_get('infolist_does_not_exist', '', '') == '')
    ptr_infolist = weechat.infolist_get('infolist_test_script', '', '')
    check(ptr_infolist != '')
    check(weechat.infolist_next(ptr_infolist) == 1)
    check(weechat.infolist_integer(ptr_infolist, 'integer') == 123)
    check(weechat.infolist_string(ptr_infolist, 'string') == 'test string')
    check(weechat.infolist_pointer(ptr_infolist, 'pointer') == '0xabcdef')
    check(weechat.infolist_time(ptr_infolist, 'time') == 1231231230)
    check(weechat.infolist_fields(ptr_infolist) == 'i:integer,s:string,p:pointer,t:time')
    check(weechat.infolist_next(ptr_infolist) == 0)
    weechat.infolist_free(ptr_infolist)
    weechat.unhook(hook_infolist)


def cmd_test_cb(data, buf, args):
    """Run all the tests."""
    weechat.prnt('', '>>>')
    weechat.prnt('', '>>> ------------------------------')
    weechat.prnt('', '>>> Testing ' + 'SCRIPT_LANGUAGE' + ' API')
    weechat.prnt('', '  > TESTS: ' + 'SCRIPT_TESTS')
    test_plugins()
    test_strings()
    test_lists()
    test_key()
    test_display()
    test_hooks()
    test_command()
    test_infolist()
    weechat.prnt('', '  > TESTS END')
    return weechat.WEECHAT_RC_OK


def weechat_init():
    """Main function."""
    weechat.register('SCRIPT_NAME', 'SCRIPT_AUTHOR', 'SCRIPT_VERSION',
                     'SCRIPT_LICENSE', 'SCRIPT_DESCRIPTION', '', '')
    weechat.hook_command('SCRIPT_NAME', '', '', '', '', 'cmd_test_cb', '')

# -*- coding: utf-8 -*-
#
# Copyright (C) 2017 Sébastien Helleu <flashcode@flashtux.org>
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    check(weechat.string_eval_path_home('test ${abc}', {}, {'abc': '123'}, {}) == 'test 123')
    check(weechat.string_mask_to_regex('test*mask') == 'test.*mask')
    check(weechat.string_has_highlight('my test string', 'test,word2') == 1)
    check(weechat.string_has_highlight_regex('my test string', 'test|word2') == 1)
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


def cmd_test_cb(data, buf, args):
    """Run all the tests."""
    weechat.prnt('', '>>>')
    weechat.prnt('', '>>> ------------------------------')
    weechat.prnt('', '>>> Testing ' + 'SCRIPT_LANGUAGE' + ' API')
    weechat.prnt('', '  > TESTS: ' + 'SCRIPT_TESTS')
    test_plugins()
    test_strings()
    test_lists()
    weechat.prnt('', '  > TESTS END')
    return weechat.WEECHAT_RC_OK


def weechat_init():
    """Main function."""
    weechat.register('SCRIPT_NAME', 'SCRIPT_AUTHOR', 'SCRIPT_VERSION',
                     'SCRIPT_LICENSE', 'SCRIPT_DESCRIPTION', '', '')
    weechat.hook_command('SCRIPT_NAME', '', '', '', '', 'cmd_test_cb', '')

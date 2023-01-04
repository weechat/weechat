#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (C) 2017-2023 Sébastien Helleu <flashcode@flashtux.org>
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
Scripts generator for WeeChat: build source of scripts in all languages to
test the scripting API.

This script can be run in WeeChat or as a standalone script
(during automatic tests, it is loaded as a WeeChat script).

It uses the following scripts:
- unparse.py: convert Python code to other languages (including Python itself)
- testapi.py: the WeeChat scripting API tests
"""

# pylint: disable=wrong-import-order,wrong-import-position
# pylint: disable=useless-object-inheritance
# pylint: disable=consider-using-f-string
# pylint: disable=super-with-arguments
# pylint: disable=consider-using-with
# pylint: disable=unspecified-encoding

from __future__ import print_function
import argparse
import ast
from datetime import datetime
import inspect
from io import StringIO
import os
import sys
import traceback

sys.dont_write_bytecode = True

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.append(SCRIPT_DIR)
from unparse import (  # noqa: E402
    UnparsePython,
    UnparsePerl,
    UnparseRuby,
    UnparseLua,
    UnparseTcl,
    UnparseGuile,
    UnparseJavascript,
    UnparsePhp,
)

RUNNING_IN_WEECHAT = True
try:
    import weechat
except ImportError:
    RUNNING_IN_WEECHAT = False


SCRIPT_NAME = 'testapigen'
SCRIPT_AUTHOR = 'Sébastien Helleu <flashcode@flashtux.org>'
SCRIPT_VERSION = '0.1'
SCRIPT_LICENSE = 'GPL3'
SCRIPT_DESC = 'Generate scripting API test scripts'

SCRIPT_COMMAND = 'testapigen'


class WeechatScript(object):  # pylint: disable=too-many-instance-attributes
    """
    A generic WeeChat script.

    This class must NOT be instanciated directly, use subclasses instead:
    PythonScript, PerlScript, ...
    """

    def __init__(self, unparse_class, tree, source_script, output_dir,
                 language, extension, comment_char='#',
                 weechat_module='weechat'):
        # pylint: disable=too-many-arguments
        self.unparse_class = unparse_class
        self.tree = tree
        self.source_script = os.path.realpath(source_script)
        self.output_dir = os.path.realpath(output_dir)
        self.language = language
        self.extension = extension
        self.script_name = 'weechat_testapi.%s' % extension
        self.script_path = os.path.join(self.output_dir, self.script_name)
        self.comment_char = comment_char
        self.weechat_module = weechat_module
        self.update_tree()

    def comment(self, string):
        """Get a commented line."""
        return '%s %s' % (self.comment_char, string)

    def update_tree(self):
        """Make changes in AST tree."""
        functions = {
            'prnt': 'print',
            'prnt_date_tags': 'print_date_tags',
            'prnt_y': 'print_y',
            'prnt_y_date_tags': 'print_y_date_tags',
        }
        tests_count = 0
        for node in ast.walk(self.tree):
            # rename some API functions
            if self.language != 'python' and \
                    isinstance(node, ast.Call) and \
                    isinstance(node.func, ast.Attribute) and \
                    node.func.value.id == 'weechat':
                node.func.attr = functions.get(node.func.attr, node.func.attr)
            # count number of tests
            if isinstance(node, ast.Call) and \
                    isinstance(node.func, ast.Name) and \
                    node.func.id == 'check':
                tests_count += 1

        # replace script variables in string values
        variables = {
            '{SCRIPT_SOURCE}': self.source_script,
            '{SCRIPT_NAME}': self.script_name,
            '{SCRIPT_PATH}': self.script_path,
            '{SCRIPT_AUTHOR}': 'Sebastien Helleu',
            '{SCRIPT_VERSION}': '1.0',
            '{SCRIPT_LICENSE}': 'GPL3',
            '{SCRIPT_DESCRIPTION}': ('%s scripting API test' %
                                     self.language.capitalize()),
            '{SCRIPT_LANGUAGE}': self.language,
            '{SCRIPT_TESTS}': str(tests_count),
        }
        # replace variables
        for node in ast.walk(self.tree):
            if isinstance(node, ast.Str) and node.s in variables:
                node.s = variables[node.s]

    def write_header(self, output):
        """Generate script header (just comments by default)."""
        comments = (
            '',
            '%s -- WeeChat %s scripting API testing' % (
                self.script_name, self.language.capitalize()),
            '',
            'WeeChat script automatically generated by testapigen.py.',
            'DO NOT EDIT BY HAND!',
            '',
            'Date: %s' % datetime.now(),
            '',
        )
        for line in comments:
            output.write(self.comment(line).rstrip() + '\n')

    def write(self):
        """Write script on disk."""
        print('Writing script %s... ' % self.script_path, end='')
        with open(self.script_path, 'w') as output:
            self.write_header(output)
            self.unparse_class(output).add(self.tree)
            output.write('\n')
            self.write_footer(output)
        print('OK')

    def write_footer(self, output):
        """Write footer (nothing by default)."""
        pass  # pylint: disable=unnecessary-pass


class WeechatPythonScript(WeechatScript):
    """A WeeChat script written in Python."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatPythonScript, self).__init__(
            UnparsePython, tree, source_script, output_dir, 'python', 'py')

    def write_header(self, output):
        output.write('# -*- coding: utf-8 -*-\n')
        super(WeechatPythonScript, self).write_header(output)
        output.write('\n'
                     'import weechat')

    def write_footer(self, output):
        output.write('\n'
                     '\n'
                     'if __name__ == "__main__":\n'
                     '    weechat_init()\n')


class WeechatPerlScript(WeechatScript):
    """A WeeChat script written in Perl."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatPerlScript, self).__init__(
            UnparsePerl, tree, source_script, output_dir, 'perl', 'pl')

    def write_footer(self, output):
        output.write('\n'
                     'weechat_init();\n')


class WeechatRubyScript(WeechatScript):
    """A WeeChat script written in Ruby."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatRubyScript, self).__init__(
            UnparseRuby, tree, source_script, output_dir, 'ruby', 'rb')

    def update_tree(self):
        super(WeechatRubyScript, self).update_tree()
        for node in ast.walk(self.tree):
            if isinstance(node, ast.Attribute) and \
                    node.value.id == 'weechat':
                node.value.id = 'Weechat'
            if isinstance(node, ast.Call) \
                    and isinstance(node.func, ast.Attribute) \
                    and node.func.attr == 'config_new_option':
                node.args = node.args[:11] + [ast.List(node.args[11:])]


class WeechatLuaScript(WeechatScript):
    """A WeeChat script written in Lua."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatLuaScript, self).__init__(
            UnparseLua, tree, source_script, output_dir, 'lua', 'lua',
            comment_char='--')

    def write_footer(self, output):
        output.write('\n'
                     'weechat_init()\n')


class WeechatTclScript(WeechatScript):
    """A WeeChat script written in Tcl."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatTclScript, self).__init__(
            UnparseTcl, tree, source_script, output_dir, 'tcl', 'tcl')

    def write_footer(self, output):
        output.write('\n'
                     'weechat_init\n')


class WeechatGuileScript(WeechatScript):
    """A WeeChat script written in Guile (Scheme)."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatGuileScript, self).__init__(
            UnparseGuile, tree, source_script, output_dir, 'guile', 'scm',
            comment_char=';')

    def update_tree(self):
        super(WeechatGuileScript, self).update_tree()
        functions_with_list = (
            'config_new_section',
            'config_new_option',
        )
        for node in ast.walk(self.tree):
            if isinstance(node, ast.Call) \
                    and isinstance(node.func, ast.Attribute) \
                    and node.func.attr in functions_with_list:
                node.args = [ast.Call('list', node.args)]

    def write_footer(self, output):
        output.write('\n'
                     '(weechat_init)\n')


class WeechatJavascriptScript(WeechatScript):
    """A WeeChat script written in Javascript."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatJavascriptScript, self).__init__(
            UnparseJavascript, tree, source_script, output_dir,
            'javascript', 'js', comment_char='//')

    def write_footer(self, output):
        output.write('\n'
                     'weechat_init()\n')


class WeechatPhpScript(WeechatScript):
    """A WeeChat script written in PHP."""

    def __init__(self, tree, source_script, output_dir):
        super(WeechatPhpScript, self).__init__(
            UnparsePhp, tree, source_script, output_dir, 'php', 'php',
            comment_char='//')

    def write_header(self, output):
        output.write('<?php\n')
        super(WeechatPhpScript, self).write_header(output)

    def write_footer(self, output):
        output.write('\n'
                     'weechat_init();\n')

# ============================================================================


def update_nodes(tree):
    """
    Update the tests AST tree (in-place):
      1. add a print message in each test_* function
      2. add arguments in calls to check() function
    """
    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef) and \
                node.name.startswith('test_'):
            # add a print at the beginning of each test function
            node.body.insert(
                0, ast.parse('weechat.prnt("", "  > %s");' % node.name))
        elif isinstance(node, ast.Call) and \
                isinstance(node.func, ast.Name) and \
                node.func.id == 'check':
            # add two arguments in the call to "check" function:
            #   1. the string representation of the test
            #   2. the line number in source (as string)
            # for example if this test is on line 50:
            #   check(weechat.test() == 123)
            # it becomes:
            #   check(weechat.test() == 123, 'weechat.test() == 123', '50')
            output = StringIO()
            unparsed = UnparsePython(output=output)
            unparsed.add(node.args[0])
            node.args.append(ast.Str(output.getvalue()))
            node.args.append(ast.Str(str(node.func.lineno)))


def get_tests(path):
    """Parse the source with tests and return the AST node."""
    test_script = open(path).read()
    tests = ast.parse(test_script)
    update_nodes(tests)
    return tests


def generate_scripts(source_script, output_dir):
    """Generate scripts in all languages to test the API."""
    ret_code = 0
    error = None
    try:
        for name, obj in inspect.getmembers(sys.modules[__name__]):
            if inspect.isclass(obj) and name != 'WeechatScript' and \
                    name.startswith('Weechat') and name.endswith('Script'):
                tests = get_tests(source_script)
                obj(tests, source_script, output_dir).write()
    except Exception as exc:  # pylint: disable=broad-except
        ret_code = 1
        error = 'ERROR: %s\n\n%s' % (str(exc), traceback.format_exc())
    return ret_code, error


def testapigen_cmd_cb(data, buf, args):
    """Callback for WeeChat command /testapigen."""

    def print_error(msg):
        """Print an error message on core buffer."""
        weechat.prnt('', '%s%s' % (weechat.prefix('error'), msg))

    try:
        source_script, output_dir = args.split()
    except ValueError:
        print_error('ERROR: invalid arguments for /testapigen')
        return weechat.WEECHAT_RC_OK
    if not weechat.mkdir_parents(output_dir, 0o755):
        print_error('ERROR: invalid directory: %s' % output_dir)
        return weechat.WEECHAT_RC_OK
    ret_code, error = generate_scripts(source_script, output_dir)
    if error:
        print_error(error)
    return weechat.WEECHAT_RC_OK if ret_code == 0 else weechat.WEECHAT_RC_ERROR


def get_parser_args():
    """Get parser arguments."""
    parser = argparse.ArgumentParser(
        description=('Generate WeeChat scripts in all languages '
                     'to test the API.'))
    parser.add_argument(
        'script',
        help='the path to Python script with tests')
    parser.add_argument(
        '-o', '--output-dir',
        default='.',
        help='output directory (defaults to current directory)')
    return parser.parse_args()


def main():
    """Main function (when script is not loaded in WeeChat)."""
    args = get_parser_args()
    ret_code, error = generate_scripts(args.script, args.output_dir)
    if error:
        print(error)
    sys.exit(ret_code)


if __name__ == '__main__':
    if RUNNING_IN_WEECHAT:
        weechat.register(SCRIPT_NAME, SCRIPT_AUTHOR, SCRIPT_VERSION,
                         SCRIPT_LICENSE, SCRIPT_DESC, '', '')
        weechat.hook_command(
            SCRIPT_COMMAND,
            'Generate scripting API test scripts',
            'source_script output_dir',
            'source_script: path to source script (testapi.py)\n'
            '   output_dir: output directory for scripts',
            '',
            'testapigen_cmd_cb', '')
    else:
        main()

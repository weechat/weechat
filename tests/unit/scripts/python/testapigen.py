#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2017-2025 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later
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

"""Test script generator for WeeChat.

Build source of scripts in all languages to test the scripting API.

This script can be run in WeeChat or as a standalone script
(during automatic tests, it is loaded as a WeeChat script).

It uses the following scripts:
- unparse.py: convert Python code to other languages (including Python itself)
- testapi.py: the WeeChat scripting API tests
"""

# ruff: noqa: T201,UP006,UP007,UP035

import argparse
import ast
import datetime
import inspect
import io
import os
import sys
import traceback
from pathlib import Path
from typing import Tuple, Type, Union

sys.dont_write_bytecode = True

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.append(str(SCRIPT_DIR))
from unparse import (  # noqa: E402
    Unparse,
    UnparseGuile,
    UnparseJavaScript,
    UnparseLua,
    UnparsePerl,
    UnparsePhp,
    UnparsePython,
    UnparseRuby,
    UnparseTcl,
)

RUNNING_IN_WEECHAT = True
try:
    import weechat
except ImportError:
    RUNNING_IN_WEECHAT = False


SCRIPT_NAME = "testapigen"
SCRIPT_AUTHOR = "Sébastien Helleu <flashcode@flashtux.org>"
SCRIPT_VERSION = "0.1"
SCRIPT_LICENSE = "GPL3"
SCRIPT_DESC = "Generate scripting API test scripts"

SCRIPT_COMMAND = "testapigen"


class WeechatScript:
    """A generic WeeChat script.

    This class must NOT be instantiated directly, use subclasses instead:
    PythonScript, PerlScript, ...
    """

    def __init__(
        self,
        unparse_class: Type[Unparse],
        tree: ast.AST,
        source_script: str,
        output_dir: str,
        language: str,
        extension: str,
        comment_char: str = "#",
        weechat_module: str = "weechat",
    ) -> None:
        """Initialize a WeeChat script generator."""
        self.unparse_class = unparse_class
        self.tree = tree
        self.source_script = os.path.realpath(source_script)
        self.output_dir = os.path.realpath(output_dir)
        self.language = language
        self.extension = extension
        self.script_name = f"weechat_testapi.{extension}"
        self.script_path = Path(self.output_dir) / self.script_name
        self.comment_char = comment_char
        self.weechat_module = weechat_module
        self.update_tree()

    def comment(self, string: str) -> str:
        """Get a commented line."""
        return f"{self.comment_char} {string}"

    def update_tree(self) -> None:
        """Make changes in AST tree."""
        functions = {
            "prnt": "print",
            "prnt_date_tags": "print_date_tags",
            "prnt_datetime_tags": "print_datetime_tags",
            "prnt_y": "print_y",
            "prnt_y_date_tags": "print_y_date_tags",
            "prnt_y_datetime_tags": "print_y_datetime_tags",
        }
        tests_count = 0
        for node in ast.walk(self.tree):
            # rename some API functions
            if (
                self.language != "python"
                and isinstance(node, ast.Call)
                and isinstance(node.func, ast.Attribute)
                and node.func.value.id == "weechat"
            ):
                node.func.attr = functions.get(node.func.attr, node.func.attr)
            # count number of tests
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "check":
                tests_count += 1

        # replace script variables in string values
        variables = {
            "{SCRIPT_SOURCE}": self.source_script,
            "{SCRIPT_NAME}": self.script_name,
            "{SCRIPT_PATH}": str(self.script_path),
            "{SCRIPT_AUTHOR}": "Sebastien Helleu",
            "{SCRIPT_VERSION}": "1.0",
            "{SCRIPT_LICENSE}": "GPL3",
            "{SCRIPT_DESCRIPTION}": f"{self.language.capitalize()} scripting API test",
            "{SCRIPT_LANGUAGE}": self.language,
            "{SCRIPT_TESTS}": str(tests_count),
        }
        # replace variables
        for node in ast.walk(self.tree):
            if isinstance(node, ast.Constant) and node.value in variables:
                node.value = variables[node.value]

    def write_header(self, output: io.TextIOBase) -> None:
        """Generate script header (just comments by default)."""
        comments = (
            "",
            f"{self.script_name} -- WeeChat {self.language.capitalize()} scripting API testing",
            "",
            "WeeChat script automatically generated by testapigen.py.",
            "DO NOT EDIT BY HAND!",
            "",
            f"Date: {datetime.datetime.now(tz=datetime.timezone.utc)}",
            "",
        )
        output.writelines(f"{self.comment(line)}\n" for line in comments)

    def write(self) -> None:
        """Write script on disk."""
        print(f"Writing script {self.script_path}... ", end="")
        with self.script_path.open("w") as output:
            self.write_header(output)
            self.unparse_class(output).add(self.tree)
            output.write("\n")
            self.write_footer(output)
        print("OK")

    def write_footer(self, output: io.TextIOBase) -> None:
        """Write footer (nothing by default)."""


class WeechatPythonScript(WeechatScript):
    """A WeeChat script written in Python."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize Python script."""
        super().__init__(UnparsePython, tree, source_script, output_dir, "python", "py")

    def write_header(self, output: io.TextIOBase) -> None:
        """Write header of Python script."""
        super().write_header(output)
        output.write("\nimport weechat")

    def write_footer(self, output: io.TextIOBase) -> None:
        """Write footer of Python script."""
        super().write_footer(output)
        output.write('\n\nif __name__ == "__main__":\n    weechat_init()\n')


class WeechatPerlScript(WeechatScript):
    """A WeeChat script written in Perl."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize Perl script."""
        super().__init__(UnparsePerl, tree, source_script, output_dir, "perl", "pl")

    def write_footer(self, output: io.TextIOBase) -> None:
        """Write footer of Perl script."""
        super().write_footer(output)
        output.write("\nweechat_init();\n")


class WeechatRubyScript(WeechatScript):
    """A WeeChat script written in Ruby."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize Ruby script."""
        super().__init__(UnparseRuby, tree, source_script, output_dir, "ruby", "rb")

    def update_tree(self) -> None:
        """Make changes in AST tree of Ruby script."""
        super().update_tree()
        for node in ast.walk(self.tree):
            if isinstance(node, ast.Attribute) and node.value.id == "weechat":
                node.value.id = "Weechat"
            if (
                isinstance(node, ast.Call)
                and isinstance(node.func, ast.Attribute)
                and node.func.attr == "config_new_option"
            ):
                node.args = [*node.args[:11], ast.List(node.args[11:])]


class WeechatLuaScript(WeechatScript):
    """A WeeChat script written in Lua."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize Lua script."""
        super().__init__(UnparseLua, tree, source_script, output_dir, "lua", "lua", comment_char="--")

    def write_footer(self, output: io.TextIOBase) -> None:
        """Write footer of Lua script."""
        super().write_footer(output)
        output.write("\nweechat_init()\n")


class WeechatTclScript(WeechatScript):
    """A WeeChat script written in Tcl."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize Tcl script."""
        super().__init__(UnparseTcl, tree, source_script, output_dir, "tcl", "tcl")

    def write_footer(self, output: io.TextIOBase) -> None:
        """Write footer of Tcl script."""
        super().write_footer(output)
        output.write("\nweechat_init\n")


class WeechatGuileScript(WeechatScript):
    """A WeeChat script written in Guile (Scheme)."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize Guile script."""
        super().__init__(UnparseGuile, tree, source_script, output_dir, "guile", "scm", comment_char=";")

    def update_tree(self) -> None:
        """Make changes in AST tree of Guile script."""
        super().update_tree()
        functions_with_list = (
            "config_new_section",
            "config_new_option",
        )
        for node in ast.walk(self.tree):
            if (
                isinstance(node, ast.Call)
                and isinstance(node.func, ast.Attribute)
                and node.func.attr in functions_with_list
            ):
                node.args = [ast.Call("list", node.args)]

    def write_footer(self, output: io.TextIOBase) -> None:
        """Write footer of Guile script."""
        super().write_footer(output)
        output.write("\n(weechat_init)\n")


class WeechatJavaScriptScript(WeechatScript):
    """A WeeChat script written in JavaScript."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize JavaScript script."""
        super().__init__(UnparseJavaScript, tree, source_script, output_dir, "javascript", "js", comment_char="//")

    def write_footer(self, output: io.TextIOBase) -> None:
        """Writer footer of JavaScript script."""
        super().write_footer(output)
        output.write("\nweechat_init()\n")


class WeechatPhpScript(WeechatScript):
    """A WeeChat script written in PHP."""

    def __init__(self, tree: ast.AST, source_script: str, output_dir: str) -> None:
        """Initialize PHP script."""
        super().__init__(UnparsePhp, tree, source_script, output_dir, "php", "php", comment_char="//")

    def write_header(self, output: io.TextIOBase) -> None:
        """Writer header of PHP script."""
        output.write("<?php\n")
        super().write_header(output)

    def write_footer(self, output: io.TextIOBase) -> None:
        """Write footer of PHP script."""
        super().write_footer(output)
        output.write("\nweechat_init();\n")


# ============================================================================


def update_nodes(tree: ast.AST) -> None:
    """Update the tests AST tree (in-place).

    Actions performed:
      1. add a print message in each test_* function
      2. add arguments in calls to check() function
    """
    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef) and node.name.startswith("test_"):
            # add a print at the beginning of each test function
            node.body.insert(0, ast.parse(f'weechat.prnt("", "  > {node.name}");'))
        elif isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "check":
            # add two arguments in the call to "check" function:
            #   1. the string representation of the test
            #   2. the line number in source (as string)
            # for example if this test is on line 50:
            #   >>> check(weechat.test() == 123)
            # it becomes:
            #   >>> check(weechat.test() == 123, 'weechat.test() == 123', '50')
            output = io.StringIO()
            unparsed = UnparsePython(output=output)
            unparsed.add(node.args[0])
            node.args.append(ast.Constant(output.getvalue()))
            node.args.append(ast.Constant(str(node.func.lineno)))


def get_tests(path: str) -> ast.AST:
    """Parse the source with tests and return the AST node."""
    with Path(path).open() as f:
        test_script = f.read()
        tests = ast.parse(test_script)
        update_nodes(tests)
        return tests


def generate_scripts(source_script: str, output_dir: str) -> Tuple[int, Union[str, None]]:
    """Generate scripts in all languages to test the API."""
    ret_code = 0
    error = None
    try:
        for name, obj in inspect.getmembers(sys.modules[__name__]):
            if (
                inspect.isclass(obj)
                and name != "WeechatScript"
                and name.startswith("Weechat")
                and name.endswith("Script")
            ):
                tests = get_tests(source_script)
                obj(tests, source_script, output_dir).write()
    except Exception as exc:  # noqa: BLE001
        ret_code = 1
        error = f"ERROR: {exc}\n\n{traceback.format_exc()}"
    return ret_code, error


def testapigen_cmd_cb(data: str, buf: str, args: str) -> int:  # noqa: ARG001
    """Callback for WeeChat command /testapigen."""

    def print_error(msg: str) -> None:
        """Print an error message on core buffer."""
        weechat.prnt("", f"{weechat.prefix('error')}{msg}")

    try:
        source_script, output_dir = args.split()
    except ValueError:
        print_error("ERROR: invalid arguments for /testapigen")
        return weechat.WEECHAT_RC_OK
    if not weechat.mkdir_parents(output_dir, 0o755):
        print_error("ERROR: invalid directory: {output_dir}")
        return weechat.WEECHAT_RC_OK
    ret_code, error = generate_scripts(source_script, output_dir)
    if error:
        print_error(error)
    return weechat.WEECHAT_RC_OK if ret_code == 0 else weechat.WEECHAT_RC_ERROR


def get_parser_args() -> argparse.Namespace:
    """Get parser arguments."""
    parser = argparse.ArgumentParser(description=("Generate WeeChat scripts in all languages to test the API."))
    parser.add_argument("script", help="the path to Python script with tests")
    parser.add_argument("-o", "--output-dir", default=".", help="output directory (defaults to current directory)")
    return parser.parse_args()


def main() -> None:
    """Generate all scripts (when script is not loaded in WeeChat)."""
    args = get_parser_args()
    ret_code, error = generate_scripts(args.script, args.output_dir)
    if error:
        print(error)
    sys.exit(ret_code)


if __name__ == "__main__":
    if RUNNING_IN_WEECHAT:
        weechat.register(SCRIPT_NAME, SCRIPT_AUTHOR, SCRIPT_VERSION, SCRIPT_LICENSE, SCRIPT_DESC, "", "")
        weechat.hook_command(
            SCRIPT_COMMAND,
            "Generate scripting API test scripts",
            "source_script output_dir",
            "source_script: path to source script (testapi.py)\n   output_dir: output directory for scripts",
            "",
            "testapigen_cmd_cb",
            "",
        )
    else:
        main()

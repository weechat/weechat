#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright (C) 2017-2025 Sébastien Helleu <flashcode@flashtux.org>
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
Unparse AST tree to generate scripts in all supported languages
(Python, Perl, Ruby, ...).
"""

# pylint: disable=too-many-lines,unnecessary-pass,useless-object-inheritance
# pylint: disable=consider-using-f-string
# pylint: disable=super-with-arguments

from __future__ import print_function
import argparse
import ast
import inspect
import os
import select
from io import StringIO
import sys

sys.dont_write_bytecode = True


class UnparsePython(object):
    """
    Unparse AST to generate Python script code.

    This class is inspired from _Unparser class in cpython:
    https://github.com/python/cpython/blob/main/Lib/ast.py

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def __init__(self, output=sys.stdout):
        self.output = output
        self.indent_string = ' ' * 4
        self._indent_level = 0
        self._prefix = []  # not used in Python, only in other languages
        self.binop = {
            'Add': '+',
            'Sub': '-',
            'Mult': '*',
            'MatMult': '@',
            'Div': '/',
            'Mod': '%',
            'LShift': '<<',
            'RShift': '>>',
            'BitOr': '|',
            'BitXor': '^',
            'BitAnd': '&',
            'FloorDiv': '//',
            'Pow': '**',
        }
        self.unaryop = {
            'Invert': '~',
            'Not': 'not ',
            'UAdd': '+',
            'USub': '-',
        }
        self.cmpop = {
            'Eq': '==',
            'NotEq': '!=',
            'Lt': '<',
            'LtE': '<=',
            'Gt': '>',
            'GtE': '>=',
            'Is': 'is',
            'IsNot': 'is not',
            'In': 'in',
            'NotIn': 'not in',
        }

    def fill(self, string=''):
        """Add a new line and an indented string."""
        self.add('\n%s%s' % (self.indent_string * self._indent_level, string))

    def indent(self):
        """Indent code."""
        self._indent_level += 1

    def unindent(self):
        """Unindent code."""
        self._indent_level -= 1

    def prefix(self, prefix):
        """Add or remove a prefix from list."""
        if prefix:
            self._prefix.append(prefix)
        else:
            self._prefix.pop()

    def add(self, *args):
        """Add string/node(s) to the output file."""
        for arg in args:
            if callable(arg):
                arg()
            elif isinstance(arg, tuple):
                arg[0](*arg[1:])
            elif isinstance(arg, list):
                for item in arg:
                    self.add(item)
            elif isinstance(arg, ast.AST):
                method = getattr(
                    self, '_ast_%s' % arg.__class__.__name__.lower(),
                    None)
                if method is None:
                    raise NotImplementedError(arg)
                method(arg)
            elif isinstance(arg, str):
                self.output.write(arg)

    @staticmethod
    def make_list(values, sep=', '):
        """Add multiple values using a custom method and separator."""
        result = []
        for value in values:
            if result:
                result.append(sep)
            result.append(value)
        return result

    @staticmethod
    def is_bool(node):
        """Check if the node is a boolean."""
        return isinstance(node, ast.Name) and node.id in ('False', 'True')

    @staticmethod
    def is_number(node):
        """Check if the node is a number."""
        return ((isinstance(node, ast.Constant) and isinstance(node.value, int)) or
                (isinstance(node, ast.UnaryOp) and
                 isinstance(node.op, (ast.UAdd, ast.USub))))

    def _ast_alias(self, node):
        """Add an AST alias in output."""
        # ignore alias
        pass

    def _ast_arg(self, node):
        """Add an AST arg in output."""
        self.add('%s%s' % (self._prefix[-1] if self._prefix else '',
                           node.arg))

    def _ast_assign(self, node):
        """Add an AST Assign in output."""
        self.add(
            self.fill,
            [[target, ' = '] for target in node.targets],
            node.value,
        )

    def _ast_attribute(self, node):
        """Add an AST Attribute in output."""
        self.add(node.value, '.', node.attr)

    def _ast_binop(self, node):
        """Add an AST BinOp in output."""
        self.add(
            node.left,
            ' %s ' % self.binop[node.op.__class__.__name__],
            node.right,
        )

    def _ast_call(self, node):
        """Add an AST Call in output."""
        self.add(
            node.func,
            '(',
            self.make_list(node.args),
            ')',
        )

    def _ast_compare(self, node):
        """Add an AST Compare in output."""
        self.add(node.left)
        for operator, comparator in zip(node.ops, node.comparators):
            self.add(
                ' %s ' % self.cmpop[operator.__class__.__name__],
                comparator,
            )

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        self.add(repr(node.value))

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            '{',
            self.make_list([[key, ': ', value]
                            for key, value in zip(node.keys, node.values)]),
            '}',
        )

    def _ast_expr(self, node):
        """Add an AST Expr in output."""
        if not isinstance(node.value, ast.Constant):  # ignore docstrings
            self.add(
                self.fill,
                node.value,
            )

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            self.fill if self._indent_level == 0 else None,
            'def %s(' % node.name,
            self.make_list(node.args.args),
            '):',
            self.indent,
            node.body,
            self.unindent,
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            'if ',
            node.test,
            ':',
            self.indent,
            node.body,
            self.unindent,
        )
        if node.orelse:
            self.add(
                self.fill,
                'else:',
                self.indent,
                node.orelse,
                self.unindent,
            )

    def _ast_index(self, node):
        """Add an AST Subscript in output."""
        # note: deprecated since Python 3.9
        self.add(node.value)

    def _ast_import(self, node):
        """Add an AST Import in output."""
        # ignore import
        pass

    def _ast_list(self, node):
        """Add an AST List in output."""
        self.add(
            '[',
            self.make_list(node.elts),
            ']',
        )

    def _ast_module(self, node):
        """Add an AST Module in output."""
        self.add(node.body)

    def _ast_name(self, node):
        """Add an AST Name in output."""
        self.add('%s%s' % (self._prefix[-1] if self._prefix else '',
                           node.id))

    def _ast_num(self, node):
        """Add an AST Num in output."""
        self.add(repr(node.n))

    def _ast_pass(self, node):  # pylint: disable=unused-argument
        """Add an AST Pass in output."""
        self.fill('pass')

    def _ast_return(self, node):
        """Add an AST Return in output."""
        self.fill('return')
        if node.value:
            self.add(' ', node.value)

    def _ast_str(self, node):
        """Add an AST Str in output."""
        self._ast_constant(node)

    def _ast_subscript(self, node):
        """Add an AST Subscript in output."""
        self.add(
            node.value,
            '[',
            node.slice,
            ']',
        )

    def _ast_tuple(self, node):
        """Add an AST Tuple in output."""
        self.add(
            '(',
            self.make_list(node.elts),
            ',' if len(node.elts) == 1 else None,
            ')',
        )

    def _ast_unaryop(self, node):
        """Add an AST UnaryOp in output."""
        self.add(
            self.unaryop[node.op.__class__.__name__],
            node.operand,
        )


class UnparsePerl(UnparsePython):
    """
    Unparse AST to generate Perl script code.

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def _ast_assign(self, node):
        """Add an AST Assign in output."""
        self.add(
            self.fill,
            (self.prefix, '$'),
            [[target, ' = '] for target in node.targets],
            (self.prefix, None),
            node.value,
            ';',
        )

    def _ast_attribute(self, node):
        """Add an AST Attribute in output."""
        saved_prefix = self._prefix
        self._prefix = []
        self.add(node.value, '::', node.attr)
        self._prefix = saved_prefix

    def _ast_binop(self, node):
        """Add an AST BinOp in output."""
        if isinstance(node.op, ast.Add) and \
                (not self.is_number(node.left) or
                 not self.is_number(node.right)):
            str_op = '.'
        else:
            str_op = self.binop[node.op.__class__.__name__]
        self.add(
            (self.prefix, '$'),
            node.left,
            ' %s ' % str_op,
            node.right,
            (self.prefix, None),
        )

    def _ast_call(self, node):
        """Add an AST Call in output."""
        self.add(
            node.func,
            '(',
            (self.prefix, '$'),
            self.make_list(node.args),
            (self.prefix, None),
            ')',
        )

    def _ast_compare(self, node):
        """Add an AST Compare in output."""
        self.add(node.left)
        for operator, comparator in zip(node.ops, node.comparators):
            if isinstance(operator, (ast.Eq, ast.NotEq)) and \
                    not self.is_number(node.left) and \
                    not self.is_bool(node.left) and \
                    not self.is_number(comparator) and \
                    not self.is_bool(comparator):
                custom_cmpop = {
                    'Eq': 'eq',
                    'NotEq': 'ne',
                }
            else:
                custom_cmpop = self.cmpop
            self.add(
                ' %s ' % custom_cmpop[operator.__class__.__name__],
                comparator,
            )

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        if isinstance(node.value, str):
            self.add('"%s"' % node.value.replace('$', '\\$').replace('@', '\\@'))
        elif node.value is None:
            self.add('undef')
        else:
            self.add(repr(node.value))

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            '{',
            self.make_list([[key, ' => ', value]
                            for key, value in zip(node.keys, node.values)]),
            '}',
        )

    def _ast_expr(self, node):
        """Add an AST Expr in output."""
        if not isinstance(node.value, ast.Constant):  # ignore docstrings
            self.add(
                self.fill,
                node.value,
                ';',
            )

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            'sub %s' % node.name,
            self.fill,
            '{',
            self.indent,
        )
        if node.args.args:
            self.add(
                self.fill,
                'my (',
                (self.prefix, '$'),
                self.make_list(node.args.args),
                (self.prefix, None),
                ') = @_;',
            )
        self.add(
            node.body,
            self.unindent,
            self.fill,
            '}',
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            'if (',
            (self.prefix, '$'),
            node.test,
            (self.prefix, None),
            ')',
            self.fill,
            '{',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            '}',
        )
        if node.orelse:
            self.add(
                self.fill,
                'else',
                self.fill,
                '{',
                self.indent,
                node.orelse,
                self.unindent,
                self.fill,
                '}',
            )

    def _ast_list(self, node):
        """Add an AST List in output."""
        self.add(
            '(',
            self.make_list(node.elts),
            ')',
        )

    def _ast_pass(self, node):
        """Add an AST Pass in output."""
        pass

    def _ast_return(self, node):
        """Add an AST Return in output."""
        self.fill('return')
        if node.value:
            self.add(
                ' ',
                (self.prefix,
                 '%' if isinstance(node.value, ast.Dict) else '$'),
                node.value,
                (self.prefix, None),
                ';',
            )

    def _ast_str(self, node):
        """Add an AST Str in output."""
        self._ast_constant(node)

    def _ast_subscript(self, node):
        """Add an AST Subscript in output."""
        self.add(
            (self.prefix, '$'),
            node.value,
            (self.prefix, None),
            '->{',
            node.slice,
            '}',
        )


class UnparseRuby(UnparsePython):
    """
    Unparse AST to generate Ruby script code.

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def _ast_attribute(self, node):
        """Add an AST Attribute in output."""
        self.add(
            node.value,
            '::' if node.attr.startswith('WEECHAT_') else '.',
            node.attr,
        )

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        if isinstance(node.value, str):
            self.add('"%s"' % node.value.replace('#{', '\\#{'))
        elif node.value is None:
            self.add('nil')
        else:
            self.add(repr(node.value))

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            'Hash[',
            self.make_list([[key, ' => ', value]
                            for key, value in zip(node.keys, node.values)]),
            ']',
        )

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            'def %s' % node.name,
        )
        if node.args.args:
            self.add(
                '(',
                self.make_list(node.args.args),
                ')',
            )
        self.add(
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            'end',
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            'if ',
            node.test,
            self.indent,
            node.body,
            self.unindent,
        )
        if node.orelse:
            self.add(
                self.fill,
                'else',
                self.indent,
                node.orelse,
                self.unindent,
                self.fill,
                'end',
            )

    def _ast_list(self, node):
        """Add an AST List in output."""
        self.add(
            'Array[',
            self.make_list(node.elts),
            ']',
        )

    def _ast_pass(self, node):
        """Add an AST Pass in output."""
        pass

    def _ast_str(self, node):
        """Add an AST Str in output."""
        self._ast_constant(node)


class UnparseLua(UnparsePython):
    """
    Unparse AST to generate Lua script code.

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def __init__(self, *args, **kwargs):
        super(UnparseLua, self).__init__(*args, **kwargs)
        self.cmpop = {
            'Eq': '==',
            'NotEq': '~=',
            'Lt': '<',
            'LtE': '<=',
            'Gt': '>',
            'GtE': '>=',
        }

    def _ast_binop(self, node):
        """Add an AST BinOp in output."""
        if isinstance(node.op, ast.Add) and \
                (not self.is_number(node.left) or
                 not self.is_number(node.right)):
            str_op = '..'
        else:
            str_op = self.binop[node.op.__class__.__name__]
        self.add(
            node.left,
            ' %s ' % str_op,
            node.right,
        )

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        if node.value is None:
            self.add('nil')
        else:
            self.add(repr(node.value))

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            '{',
            self.make_list([
                ['[', key, ']', '=', value]
                for key, value in zip(node.keys, node.values)]),
            '}',
        )

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            'function %s' % node.name,
        )
        self.add(
            '(',
            self.make_list(node.args.args),
            ')',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            'end',
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            'if ',
            node.test,
            ' then',
            self.indent,
            node.body,
            self.unindent,
        )
        if node.orelse:
            self.add(
                self.fill,
                'else',
                self.indent,
                node.orelse,
                self.unindent,
                self.fill,
                'end',
            )

    def _ast_list(self, node):
        """Add an AST List in output."""
        self.add(
            '{',
            self.make_list(node.elts),
            '}',
        )

    def _ast_pass(self, node):
        """Add an AST Pass in output."""
        pass


class UnparseTcl(UnparsePython):
    """
    Unparse AST to generate Tcl script code.

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def __init__(self, *args, **kwargs):
        super(UnparseTcl, self).__init__(*args, **kwargs)
        self._call = 0

    def _ast_assign(self, node):
        """Add an AST Assign in output."""
        exclude_types = (ast.Dict, ast.List, ast.Constant, ast.Subscript)
        self.add(
            self.fill,
            'set ',
            node.targets[0],
            ' ',
            '[' if not isinstance(node.value, exclude_types) else '',
            node.value,
            ']' if not isinstance(node.value, exclude_types) else '',
        )

    def _ast_attribute(self, node):
        """Add an AST Attribute in output."""
        saved_prefix = self._prefix
        self._prefix = []
        if node.attr.startswith('WEECHAT_'):
            self.add('$::')
        self.add(node.value, '::', node.attr)
        self._prefix = saved_prefix

    def _ast_binop(self, node):
        """Add an AST BinOp in output."""
        self.add(
            '[join [list ',
            (self.prefix, '$'),
            node.left,
            ' ',
            node.right,
            (self.prefix, None),
            '] ""]',
        )

    def _ast_call(self, node):
        """Add an AST Call in output."""
        if self._call:
            self.add('[')
        self._call += 1
        self.add(
            node.func,
            ' ' if node.args else None,
            (self.prefix, '$'),
            self.make_list(node.args, sep=' '),
            (self.prefix, None),
        )
        self._call -= 1
        if self._call:
            self.add(']')

    def _ast_compare(self, node):
        """Add an AST Compare in output."""
        self.prefix('$')
        if self._call:
            self.add('[expr {')
        self.add(node.left)
        for operator, comparator in zip(node.ops, node.comparators):
            if isinstance(operator, (ast.Eq, ast.NotEq)) and \
                    not self.is_number(node.left) and \
                    not self.is_bool(node.left) and \
                    not self.is_number(comparator) and \
                    not self.is_bool(comparator):
                custom_cmpop = {
                    'Eq': 'eq',
                    'NotEq': 'ne',
                }
            else:
                custom_cmpop = self.cmpop
            self.add(
                ' %s ' % custom_cmpop[operator.__class__.__name__],
                comparator,
            )
        if self._call:
            self.add('}]')
        self.prefix(None)

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        if isinstance(node.value, str):
            self.add('"%s"' % node.value.replace('$', '\\$'))
        elif node.value is None:
            self.add('$::weechat::WEECHAT_NULL')
        else:
            self.add(repr(node.value))

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            '[dict create ',
            self.make_list([[key, ' ', value]
                            for key, value in zip(node.keys, node.values)],
                           sep=' '),
            ']',
        )

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            'proc %s {' % node.name,
            (self.make_list(node.args.args, sep=' ')
             if node.args.args else None),
            '} {',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            '}',
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            'if {',
            (self.prefix, '$'),
            node.test,
            (self.prefix, None),
            '} {',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            '}',
        )
        if node.orelse:
            self.add(
                ' else {',
                self.indent,
                node.orelse,
                self.unindent,
                self.fill,
                '}',
            )

    def _ast_list(self, node):
        """Add an AST List in output."""
        self.add(
            '[',
            self.make_list(node.elts, sep=' '),
            ']',
        )

    def _ast_pass(self, node):
        """Add an AST Pass in output."""
        pass

    def _ast_return(self, node):
        """Add an AST Return in output."""
        self.fill('return')
        if node.value:
            self.add(
                ' ',
                (self.prefix, '$'),
                node.value,
                (self.prefix, None),
            )

    def _ast_str(self, node):
        """Add an AST Str in output."""
        self._ast_constant(node)

    def _ast_subscript(self, node):
        """Add an AST Subscript in output."""
        self.add(
            '[dict get ',
            (self.prefix, '$'),
            node.value,
            (self.prefix, None),
            ' ',
            node.slice,
            ']',
        )


class UnparseGuile(UnparsePython):
    """
    Unparse AST to generate Guile script code.

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def __init__(self, *args, **kwargs):
        super(UnparseGuile, self).__init__(*args, **kwargs)
        self.cmpop = {
            'Eq': '=',
            'NotEq': '<>',
            'Lt': '<',
            'LtE': '<=',
            'Gt': '>',
            'GtE': '>=',
        }
        self._call = 0
        self._let = 0

    def _ast_assign(self, node):
        """Add an AST Assign in output."""
        self.add(
            self.fill,
            '(let ((',
            node.targets[0],
            ' ',
            node.value,
            '))',
            self.indent,
            self.fill,
            '(begin',
            self.indent,
        )
        self._let += 1

    def _ast_attribute(self, node):
        """Add an AST Attribute in output."""
        self.add(node.value, ':', node.attr)

    def _ast_binop(self, node):
        """Add an AST BinOp in output."""
        if isinstance(node.op, ast.Add) and \
                (isinstance(node.left, (ast.Name, ast.Constant)) or
                 isinstance(node.right, (ast.Name, ast.Constant))):
            self.add(
                '(string-append ',
                node.left,
                ' ',
                node.right,
                ')',
            )
        else:
            self.add(
                node.left,
                ' %s ' % self.binop[node.op.__class__.__name__],
                node.right,
            )

    def _ast_call(self, node):
        """Add an AST Call in output."""
        self._call += 1
        self.add(
            '(',
            node.func,
            ' ' if node.args else None,
            self.make_list(node.args, sep=' '),
            ')',
        )
        self._call -= 1

    def _ast_compare(self, node):
        """Add an AST Compare in output."""
        for operator, comparator in zip(node.ops, node.comparators):
            if isinstance(operator, (ast.Eq, ast.NotEq)) and \
                    not self.is_number(node.left) and \
                    not self.is_bool(node.left) and \
                    not self.is_number(comparator) and \
                    not self.is_bool(comparator):
                prefix = 'string'
            else:
                prefix = ''
            self.add(
                '(%s%s ' % (prefix, self.cmpop[operator.__class__.__name__]),
                node.left,
                ' ',
                comparator,
                ')',
            )

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        if isinstance(node.value, str):
            self.add('"%s"' % node.value)
        elif node.value is None:
            self.add('#nil')
        else:
            self.add(repr(node.value))

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            '\'(',
            self.make_list([['(', key, ' ', value, ')']
                            for key, value in zip(node.keys, node.values)],
                           sep=' '),
            ')',
        )

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            '(define (%s' % node.name,
            ' ' if node.args.args else None,
            (self.make_list(node.args.args, sep=' ')
             if node.args.args else None),
            ')',
            self.indent,
            node.body,
        )
        while self._let > 0:
            self.add(
                self.unindent,
                self.fill,
                ')',
                self.unindent,
                self.fill,
                ')',
            )
            self._let -= 1
        self.add(
            self.unindent,
            self.fill,
            ')',
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            '(if '
            '' if isinstance(node.test, ast.Name) else '(',
            node.test,
            '' if isinstance(node.test, ast.Name) else ')',
            self.indent,
            self.fill,
            '(begin',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            ')',
            self.unindent,
        )
        if node.orelse:
            self.add(
                self.indent,
                self.fill,
                '(begin',
                self.indent,
                node.orelse,
                self.unindent,
                self.fill,
                ')',
                self.unindent,
            )
        self.add(self.fill, ')')

    def _ast_list(self, node):
        """Add an AST List in output."""
        self.add(
            '\'(',
            self.make_list(node.elts, sep=' '),
            ')',
        )

    def _ast_pass(self, node):
        """Add an AST Pass in output."""
        self.add('#t')

    def _ast_return(self, node):
        """Add an AST Return in output."""
        if node.value:
            self.add(self.fill, node.value)

    def _ast_str(self, node):
        """Add an AST Str in output."""
        self._ast_constant(node)

    def _ast_subscript(self, node):
        """Add an AST Subscript in output."""
        self.add(
            '(assoc-ref ',
            node.value,
            ' ',
            node.slice,
            ')',
        )


class UnparseJavaScript(UnparsePython):
    """
    Unparse AST to generate JavaScript script code.

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            '{',
            self.make_list([[key, ': ', value]
                            for key, value in zip(node.keys, node.values)]),
            '}',
        )

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        if node.value is None:
            self.add('null')
        else:
            self.add(repr(node.value))

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            'function %s(' % node.name,
            self.make_list(node.args.args),
            ') {',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            '}',
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            'if (',
            node.test,
            ') {',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            '}',
        )
        if node.orelse:
            self.add(
                ' else {',
                self.indent,
                node.orelse,
                self.unindent,
                self.fill,
                '}',
            )

    def _ast_pass(self, node):
        """Add an AST Pass in output."""
        pass


class UnparsePhp(UnparsePython):
    """
    Unparse AST to generate PHP script code.

    Note: only part of AST types are supported (just the types used by
    the script to test WeeChat scripting API).
    """

    __lineno__ = inspect.currentframe().f_lineno

    def _ast_assign(self, node):
        """Add an AST Assign in output."""
        self.add(
            self.fill,
            (self.prefix, '$'),
            [[target, ' = '] for target in node.targets],
            (self.prefix, None),
            node.value,
            ';',
        )

    def _ast_attribute(self, node):
        """Add an AST Attribute in output."""
        saved_prefix = self._prefix
        self._prefix = []
        if not node.attr.startswith('WEECHAT_'):
            self.add(node.value, '_')
        self.add(node.attr)
        self._prefix = saved_prefix

    def _ast_binop(self, node):
        """Add an AST BinOp in output."""
        if isinstance(node.op, ast.Add) and \
                (isinstance(node.left, (ast.Name, ast.Constant)) or
                 isinstance(node.right, (ast.Name, ast.Constant))):
            str_op = '.'
        else:
            str_op = self.binop[node.op.__class__.__name__]
        self.add(
            (self.prefix, '$'),
            node.left,
            ' %s ' % str_op,
            node.right,
            (self.prefix, None),
        )

    def _ast_call(self, node):
        """Add an AST Call in output."""
        self.add(
            node.func,
            '(',
            (self.prefix, '$'),
            self.make_list(node.args),
            (self.prefix, None),
            ')',
        )

    def _ast_constant(self, node):
        """Add an AST Constant in output."""
        if isinstance(node.value, str):
            self.add('"%s"' % node.value.replace('$', '\\$'))
        elif node.value is None:
            self.add('NULL')
        else:
            self.add(repr(node.value))

    def _ast_dict(self, node):
        """Add an AST Dict in output."""
        self.add(
            'array(',
            self.make_list([[key, ' => ', value]
                            for key, value in zip(node.keys, node.values)]),
            ')',
        )

    def _ast_expr(self, node):
        """Add an AST Expr in output."""
        if not isinstance(node.value, ast.Constant):  # ignore docstrings
            self.add(
                self.fill,
                node.value,
                ';',
            )

    def _ast_functiondef(self, node):
        """Add an AST FunctionDef in output."""
        self.add(
            self.fill,
            self.fill,
            'function %s(' % node.name,
            (self.prefix, '$'),
            self.make_list(node.args.args),
            (self.prefix, None),
            ')',
            self.fill,
            '{',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            '}',
        )

    def _ast_list(self, node):
        """Add an AST List in output."""
        self.add(
            'array(',
            self.make_list(node.elts),
            ')',
        )

    def _ast_if(self, node):
        """Add an AST If in output."""
        self.add(
            self.fill,
            'if (',
            (self.prefix, '$'),
            node.test,
            (self.prefix, None),
            ')',
            self.fill,
            '{',
            self.indent,
            node.body,
            self.unindent,
            self.fill,
            '}',
        )
        if node.orelse:
            self.add(
                self.fill,
                'else',
                self.fill,
                '{',
                self.indent,
                node.orelse,
                self.unindent,
                self.fill,
                '}',
            )

    def _ast_pass(self, node):
        """Add an AST Pass in output."""
        pass

    def _ast_return(self, node):
        """Add an AST Return in output."""
        self.fill('return')
        if node.value:
            self.add(
                ' ',
                (self.prefix, '$'),
                node.value,
                (self.prefix, None),
                ';',
            )

    def _ast_str(self, node):
        """Add an AST Str in output."""
        self._ast_constant(node)

    def _ast_subscript(self, node):
        """Add an AST Subscript in output."""
        self.add(
            '$',
            node.value,
            '[',
            node.slice,
            ']',
        )


def get_languages():
    """Return a list of supported languages: ['python', 'perl', ...]."""

    members = [
        member
        for member in inspect.getmembers(sys.modules[__name__],
                                         predicate=inspect.isclass)
        if inspect.isclass(member[1]) and member[0].startswith('Unparse')
    ]

    languages = [
        name[7:].lower()
        for name, _ in sorted(members,
                              key=lambda member: member[1].__lineno__)
    ]

    return languages


LANGUAGES = get_languages()


def get_parser():
    """Get parser arguments."""
    all_languages = LANGUAGES + ['all']
    default_language = LANGUAGES[0]
    parser = argparse.ArgumentParser(
        description=('Unparse Python code from stdin and generate code in '
                     'another language (to stdout).\n\n'
                     'The code is read from stdin and generated code is '
                     'written on stdout.'))
    parser.add_argument(
        '-l', '--language',
        default=default_language,
        choices=all_languages,
        help='output language (default: %s)' % default_language)
    return parser


def get_stdin():
    """
    Return data from standard input.

    If there is nothing in stdin, wait for data until ctrl-d (EOF)
    is received.
    """
    data = ''
    inr = select.select([sys.stdin], [], [], 0)[0]
    if not inr:
        print('Enter the code to convert (Enter + ctrl-d to end)')
    while True:
        inr = select.select([sys.stdin], [], [], 0.1)[0]
        if not inr:
            continue
        new_data = os.read(sys.stdin.fileno(), 4096)
        if not new_data:  # EOF?
            break
        data += new_data.decode('utf-8')
    return data


def convert_to_language(code, language, prefix=''):
    """Convert Python code to a language."""
    class_name = 'Unparse%s' % language.capitalize()
    unparse_class = getattr(sys.modules[__name__], class_name)
    if prefix:
        print(prefix)
    output = StringIO()
    unparse_class(output=output).add(ast.parse(code))
    print(output.getvalue().lstrip())


def convert(code, language):
    """Convert Python code to one or all languages."""
    if language == 'all':
        for lang in LANGUAGES:
            convert_to_language(code, lang, '\n%s:' % lang)
    else:
        convert_to_language(code, language)


def main():
    """Main function."""
    parser = get_parser()
    args = parser.parse_args()

    code = get_stdin()
    if not code:
        print('ERROR: missing input')
        print()
        parser.print_help()
        sys.exit(1)

    convert(code, args.language)

    sys.exit(0)


if __name__ == '__main__':
    main()

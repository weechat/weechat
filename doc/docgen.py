#!/usr/bin/env python3
#
# Copyright (C) 2008-2023 Sébastien Helleu <flashcode@flashtux.org>
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
Documentation generator for WeeChat: build include files with:

- commands
- config options
- default aliases
- IRC colors
- infos
- infos hashtable
- infolists
- hdata
- completions
- URL options
- plugins priority.

Instructions to build config files yourself in WeeChat directories
(replace "path" with the path to the docgen.py script in WeeChat repository):

  weechat -t -r "/python load /path/docgen.py;/docgen;/quit"

There's one output file per language (where xx is language):

  /path/xx/includes/autogen.xx.adoc

This script requires Python 3.6+.
"""

SCRIPT_NAME = 'docgen'
SCRIPT_AUTHOR = 'Sébastien Helleu <flashcode@flashtux.org>'
SCRIPT_VERSION = '0.3'
SCRIPT_LICENSE = 'GPL3'
SCRIPT_DESC = 'Documentation generator for WeeChat'

SCRIPT_COMMAND = 'docgen'

IMPORT_OK = True

# pylint: disable=wrong-import-position
try:
    from collections import defaultdict
    from operator import itemgetter
    import gettext
    import hashlib
    import inspect
    import os
    import re
except ImportError as exc:
    print(f'Missing package(s) for {SCRIPT_NAME}: {exc}')
    IMPORT_OK = False

try:
    import weechat  # pylint: disable=import-error
except ImportError:
    print('This script must be run under WeeChat.')
    print('Get WeeChat now at: https://weechat.org/')
    IMPORT_OK = False

# list of locales for which we want to build doc files to include
LOCALE_LIST = (
    'de_DE',
    'en_US',
    'fr_FR',
    'it_IT',
    'ja_JP',
    'pl_PL',
    'sr_RS',
)

# all commands/options/.. of following plugins will produce a file
# non-listed plugins will be ignored
# value: "c" = plugin may have many commands
#        "o" = write config options for plugin
# if plugin is listed without "c", that means plugin has only one command
# /name (where "name" is name of plugin)
# Note: we consider core is a plugin called "weechat"
PLUGIN_LIST = {
    'sec': 'o',
    'weechat': 'co',
    'alias': '',
    'buflist': 'co',
    'charset': 'o',
    'exec': 'o',
    'fifo': 'o',
    'fset': 'o',
    'irc': 'co',
    'logger': 'o',
    'relay': 'o',
    'script': 'o',
    'perl': 'o',
    'python': 'o',
    'ruby': 'o',
    'lua': 'o',
    'tcl': 'o',
    'guile': 'o',
    'javascript': 'o',
    'php': 'o',
    'spell': 'o',
    'trigger': 'o',
    'xfer': 'co',
    'typing': 'o',
}

# options to ignore
IGNORE_OPTIONS = (
    r'charset\.decode\..*',
    r'charset\.encode\..*',
    r'irc\.msgbuffer\..*',
    r'irc\.ctcp\..*',
    r'irc\.ignore\..*',
    r'irc\.server\..*',
    r'logger\.level\..*',
    r'logger\.mask\..*',
    r'relay\.port\..*',
    r'spell\.dict\..*',
    r'spell\.option\..*',
    r'trigger\.trigger\..*',
    r'weechat\.palette\..*',
    r'weechat\.proxy\..*',
    r'weechat\.bar\..*',
    r'weechat\.debug\..*',
    r'weechat\.notify\..*',
)

# completions to ignore
IGNORE_COMPLETIONS_ITEMS = (
    'docgen.*',
)


def translate(string):
    """Translate a string."""
    return _(string) if string else string


def escape(string):
    """Escape a string."""
    return string.replace('|', '\\|')


def sha256_file(filename, default=None):
    """Return SHA256 checksum of a file."""
    try:
        with open(filename, 'rb') as _file:
            checksum = hashlib.sha256(_file.read()).hexdigest()
    except IOError:
        checksum = default
    return checksum


class WeechatDoc():  # pylint: disable=too-few-public-methods
    """A class to read documentation from WeeChat API."""

    def __init__(self):
        pass

    def read_doc(self):
        """Get documentation from WeeChat API."""
        functions = sorted([
            func[0]
            for func in inspect.getmembers(self, predicate=inspect.isfunction)
            if func[0].startswith('_read_')
        ])
        return {
            function[6:]: getattr(self, function)()
            for function in functions
        }

    @staticmethod
    def _read_user_commands():
        """
        Get list of WeeChat/plugins commands as dictionary with 3 indexes:
        plugin, command, xxx.
        """
        commands = defaultdict(lambda: defaultdict(defaultdict))
        infolist = weechat.infolist_get('hook', '', 'command')
        while weechat.infolist_next(infolist):
            plugin = (weechat.infolist_string(infolist, 'plugin_name')
                      or 'weechat')
            if plugin in PLUGIN_LIST:
                command = weechat.infolist_string(infolist, 'command')
                if command == plugin or 'c' in PLUGIN_LIST[plugin]:
                    for key in ('description', 'args', 'args_description',
                                'completion'):
                        commands[plugin][command][key] = \
                            weechat.infolist_string(infolist, key)
        weechat.infolist_free(infolist)
        return commands

    @staticmethod
    def _read_user_options():
        """
        Get list of WeeChat/plugins config options as dictionary with
        4 indexes: config, section, option, xxx.
        """
        options = \
            defaultdict(lambda: defaultdict(lambda: defaultdict(defaultdict)))
        infolist = weechat.infolist_get('option', '', '')
        while weechat.infolist_next(infolist):
            full_name = weechat.infolist_string(infolist, 'full_name')
            if not re.search('|'.join(IGNORE_OPTIONS), full_name):
                config = weechat.infolist_string(infolist, 'config_name')
                if config in PLUGIN_LIST and 'o' in PLUGIN_LIST[config]:
                    section = weechat.infolist_string(infolist, 'section_name')
                    option = weechat.infolist_string(infolist, 'option_name')
                    for key in ('type', 'string_values', 'default_value',
                                'description'):
                        options[config][section][option][key] = \
                            weechat.infolist_string(infolist, key)
                    for key in ('min', 'max', 'null_value_allowed'):
                        options[config][section][option][key] = \
                            weechat.infolist_integer(infolist, key)
        weechat.infolist_free(infolist)
        return options

    @staticmethod
    def _read_api_infos():
        """
        Get list of WeeChat/plugins infos as dictionary with 3 indexes:
        plugin, name, xxx.
        """
        infos = defaultdict(lambda: defaultdict(defaultdict))
        infolist = weechat.infolist_get('hook', '', 'info')
        while weechat.infolist_next(infolist):
            info_name = weechat.infolist_string(infolist, 'info_name')
            plugin = (weechat.infolist_string(infolist, 'plugin_name')
                      or 'weechat')
            for key in ('description', 'args_description'):
                infos[plugin][info_name][key] = \
                    weechat.infolist_string(infolist, key)
        weechat.infolist_free(infolist)
        return infos

    @staticmethod
    def _read_api_infos_hashtable():
        """
        Get list of WeeChat/plugins infos (hashtable) as dictionary with
        3 indexes: plugin, name, xxx.
        """
        infos_hashtable = defaultdict(lambda: defaultdict(defaultdict))
        infolist = weechat.infolist_get('hook', '', 'info_hashtable')
        while weechat.infolist_next(infolist):
            info_name = weechat.infolist_string(infolist, 'info_name')
            plugin = (weechat.infolist_string(infolist, 'plugin_name')
                      or 'weechat')
            for key in ('description', 'args_description',
                        'output_description'):
                infos_hashtable[plugin][info_name][key] = \
                    weechat.infolist_string(infolist, key)
        weechat.infolist_free(infolist)
        return infos_hashtable

    @staticmethod
    def _read_api_infolists():
        """
        Get list of WeeChat/plugins infolists as dictionary with 3 indexes:
        plugin, name, xxx.
        """
        infolists = defaultdict(lambda: defaultdict(defaultdict))
        infolist = weechat.infolist_get('hook', '', 'infolist')
        while weechat.infolist_next(infolist):
            infolist_name = weechat.infolist_string(infolist, 'infolist_name')
            plugin = (weechat.infolist_string(infolist, 'plugin_name')
                      or 'weechat')
            for key in ('description', 'pointer_description',
                        'args_description'):
                infolists[plugin][infolist_name][key] = \
                    weechat.infolist_string(infolist, key)
        weechat.infolist_free(infolist)
        return infolists

    @staticmethod
    def _read_api_hdata():  # pylint: disable=too-many-locals
        """
        Get list of WeeChat/plugins hdata as dictionary with 3 indexes:
        plugin, name, xxx.
        """
        hdata = defaultdict(lambda: defaultdict(defaultdict))
        infolist = weechat.infolist_get('hook', '', 'hdata')
        while weechat.infolist_next(infolist):
            hdata_name = weechat.infolist_string(infolist, 'hdata_name')
            plugin = (weechat.infolist_string(infolist, 'plugin_name')
                      or 'weechat')
            hdata[plugin][hdata_name]['description'] = \
                weechat.infolist_string(infolist, 'description')
            variables = ''
            vars_update = ''
            lists = ''
            ptr_hdata = weechat.hdata_get(hdata_name)
            if ptr_hdata:
                hdata2 = []
                string = weechat.hdata_get_string(ptr_hdata, 'var_keys_values')
                if string:
                    for item in string.split(','):
                        key = item.split(':')[0]
                        var_offset = weechat.hdata_get_var_offset(
                            ptr_hdata,
                            key,
                        )
                        var_array_size = \
                            weechat.hdata_get_var_array_size_string(
                                ptr_hdata,
                                '',
                                key,
                            )
                        if var_array_size:
                            var_array_size = \
                                f', array_size: "{var_array_size}"'
                        var_hdata = weechat.hdata_get_var_hdata(ptr_hdata, key)
                        if var_hdata:
                            var_hdata = f', hdata: "{var_hdata}"'
                        type_string = weechat.hdata_get_var_type_string(
                            ptr_hdata,
                            key,
                        )
                        hdata2.append({
                            'offset': var_offset,
                            'text': f'_{key}_ ({type_string})',
                            'textlong': (f'_{key}_   ({type_string}'
                                         f'{var_array_size}{var_hdata})'),
                            'update': weechat.hdata_update(
                                ptr_hdata, '', {'__update_allowed': key}),
                        })
                    hdata2 = sorted(hdata2, key=itemgetter('offset'))
                    for item in hdata2:
                        variables += f'{item["textlong"]} +\n'
                        if item['update']:
                            vars_update += f'    {item["text"]} +\n'
                    if weechat.hdata_update(ptr_hdata, '',
                                            {'__create_allowed': ''}):
                        vars_update += '    _{hdata_update_create}_ +\n'
                    if weechat.hdata_update(ptr_hdata, '',
                                            {'__delete_allowed': ''}):
                        vars_update += '    _{hdata_update_delete}_ +\n'
                hdata[plugin][hdata_name]['vars'] = variables
                hdata[plugin][hdata_name]['vars_update'] = vars_update.rstrip()

                string = weechat.hdata_get_string(ptr_hdata, 'list_keys')
                if string:
                    list_lists = string.split(',')
                    lists_std = [lst for lst in list_lists
                                 if not lst.startswith('last_')]
                    lists_last = [lst for lst in list_lists
                                  if lst.startswith('last_')]
                    for item in sorted(lists_std) + sorted(lists_last):
                        lists += f'_{item}_ +\n'
                hdata[plugin][hdata_name]['lists'] = lists
        weechat.infolist_free(infolist)
        return hdata

    @staticmethod
    def _read_api_completions():
        """
        Get list of WeeChat/plugins completions as dictionary with 3 indexes:
        plugin, item, xxx.
        """
        completions = defaultdict(lambda: defaultdict(defaultdict))
        infolist = weechat.infolist_get('hook', '', 'completion')
        while weechat.infolist_next(infolist):
            completion_item = weechat.infolist_string(infolist,
                                                      'completion_item')
            if not re.search('|'.join(IGNORE_COMPLETIONS_ITEMS),
                             completion_item):
                plugin = (weechat.infolist_string(infolist, 'plugin_name')
                          or 'weechat')
                completions[plugin][completion_item]['description'] = \
                    weechat.infolist_string(infolist, 'description')
        weechat.infolist_free(infolist)
        return completions

    @staticmethod
    def _read_api_url_options():
        """
        Get list of URL options as list of dictionaries.
        """
        url_options = []
        infolist = weechat.infolist_get('url_options', '', '')
        while weechat.infolist_next(infolist):
            url_options.append({
                'name': weechat.infolist_string(infolist, 'name').lower(),
                'option': weechat.infolist_integer(infolist, 'option'),
                'type': weechat.infolist_string(infolist, 'type'),
                'constants': weechat.infolist_string(
                    infolist, 'constants').lower().replace(',', ', ')
            })
        weechat.infolist_free(infolist)
        return url_options

    @staticmethod
    def _read_user_default_aliases():
        """
        Get list of default aliases as list of dictionaries.
        """
        default_aliases = []
        infolist = weechat.infolist_get('alias_default', '', '')
        while weechat.infolist_next(infolist):
            default_aliases.append({
                'name': '/' + weechat.infolist_string(infolist, 'name'),
                'command': '/' + weechat.infolist_string(infolist, 'command'),
                'completion': weechat.infolist_string(infolist, 'completion'),
            })
        weechat.infolist_free(infolist)
        return default_aliases

    @staticmethod
    def _read_user_irc_colors():
        """
        Get list of IRC colors as list of dictionaries.
        """
        irc_colors = []
        infolist = weechat.infolist_get('irc_color_weechat', '', '')
        while weechat.infolist_next(infolist):
            irc_colors.append({
                'color_irc': weechat.infolist_string(infolist, 'color_irc'),
                'color_weechat': weechat.infolist_string(infolist,
                                                         'color_weechat'),
            })
        weechat.infolist_free(infolist)
        return irc_colors

    @staticmethod
    def _read_api_plugins_priority():
        """
        Get priority of default WeeChat plugins as a dictionary.
        """
        plugins_priority = {}
        infolist = weechat.infolist_get('plugin', '', '')
        while weechat.infolist_next(infolist):
            name = weechat.infolist_string(infolist, 'name')
            priority = weechat.infolist_integer(infolist, 'priority')
            if priority in plugins_priority:
                plugins_priority[priority].append(name)
            else:
                plugins_priority[priority] = [name]
        weechat.infolist_free(infolist)
        return plugins_priority


class AutogenDoc():
    """A class to write auto-generated doc files."""

    def __init__(self, weechat_doc, doc_directory, locale):
        """Initialize auto-generated doc file."""
        self.doc_directory = doc_directory
        self.locale = locale
        self.count_files = 0
        self.count_updated = 0
        self.filename = None
        self.filename_tmp = None
        self._file = None
        self.install_translations()
        self.write_autogen_files(weechat_doc)

    def install_translations(self):
        """Install translations."""
        trans = gettext.translation(
            'weechat',
            weechat.info_get('weechat_localedir', ''),
            languages=[f'{self.locale}.UTF-8'],
            fallback=True,
        )
        trans.install()

    def open_file(self, name):
        """Open temporary auto-generated file."""
        self.filename = os.path.join(
            self.doc_directory,
            self.locale[:2],
            'includes',
            f'autogen_{name}.{self.locale[:2]}.adoc',
        )
        self.filename_tmp = f'{self.filename}.tmp'
        # pylint: disable=consider-using-with
        self._file = open(self.filename_tmp, 'w', encoding='utf-8')

    def write_autogen_files(self, weechat_doc):
        """Write auto-generated files."""
        for name, doc in weechat_doc.items():
            self.open_file(name)
            self.write_autogen_file(name, doc)
            self.update_autogen_file()

    def write_autogen_file(self, name, doc):
        """Write auto-generated file."""
        self.write('//')
        self.write('// This file is auto-generated by script docgen.py.')
        self.write('// DO NOT EDIT BY HAND!')
        self.write('//')
        getattr(self, f'_write_{name}')(doc)

    def write(self, *args):
        """Write a line in auto-generated doc file."""
        if args:
            if len(args) > 1:
                self._file.write(args[0] % args[1:])
            else:
                self._file.write(args[0])
        self._file.write('\n')

    def update_autogen_file(self):
        """Update doc file if needed (if content has changed)."""
        self.count_files += 1
        # close temp file
        self._file.close()
        sha_old = sha256_file(self.filename, 'old')
        sha_new = sha256_file(self.filename_tmp, 'new')
        # compare checksums
        if sha_old != sha_new:
            # update doc file
            if os.path.exists(self.filename):
                os.unlink(self.filename)
            os.rename(self.filename_tmp, self.filename)
            self.count_updated += 1
        else:
            os.unlink(self.filename_tmp)

    def __str__(self):
        """Get status string."""
        if self.count_updated > 0:
            color_count = weechat.color('yellow')
            color_updated = weechat.color('green')
            color_reset = weechat.color('reset')
            str_updated = (f', {color_count}{self.count_updated} '
                           f'{color_updated}updated{color_reset}')
        else:
            str_updated = ''
        return f'{self.locale}: {self.count_files} files{str_updated}'

    def _write_user_commands(self, commands):
        """Write commands."""
        for plugin in commands:
            self.write()
            self.write(f'// tag::{plugin}_commands[]')
            for i, command in enumerate(sorted(commands[plugin])):
                if i > 0:
                    self.write()
                _cmd = commands[plugin][command]
                args = translate(_cmd['args'])
                args_formats = args.split(' || ')
                desc = translate(_cmd['description'])
                args_desc = translate(_cmd['args_description'])
                self.write(f'[[command_{plugin}_{command}]]')
                self.write(f'* `+{command}+`: {desc}\n')
                self.write('----')
                prefix = '/' + command + '  '
                if args_formats != ['']:
                    for fmt in args_formats:
                        self.write(prefix + fmt)
                        prefix = ' ' * len(prefix)
                if args_desc:
                    self.write()
                    self.write(args_desc)
                self.write('----')
            self.write(f'// end::{plugin}_commands[]')

    # pylint: disable=too-many-locals,too-many-branches
    def _write_user_options(self, options):
        """Write config options."""
        for config in options:
            self.write()
            self.write(f'// tag::{config}_options[]')
            i = 0
            for section in sorted(options[config]):
                for option in sorted(options[config][section]):
                    if i > 0:
                        self.write()
                    i += 1
                    _opt = options[config][section][option]
                    opt_type = _opt['type']
                    string_values = _opt['string_values']
                    default_value = _opt['default_value']
                    opt_min = _opt['min']
                    opt_max = _opt['max']
                    null_value_allowed = _opt['null_value_allowed']
                    desc = translate(_opt['description'])
                    type_nls = translate(opt_type)
                    values = ''
                    if opt_type == 'boolean':
                        values = 'on, off'
                    elif opt_type == 'integer':
                        if string_values:
                            values = string_values.replace('|', ', ')
                        else:
                            values = f'{opt_min} .. {opt_max}'
                    elif opt_type == 'string':
                        if opt_max <= 0:
                            values = _('any string')
                        elif opt_max == 1:
                            values = _('any char')
                        elif opt_max > 1:
                            values = (_('any string')
                                      + '(' + _('max chars') + ': '
                                      + opt_max + ')')
                        else:
                            values = _('any string')
                        default_value = ('"%s"' %
                                         default_value.replace('"', '\\"'))
                    elif opt_type == 'color':
                        values = _(
                            'a WeeChat color name (default, black, '
                            '(dark)gray, white, (light)red, '
                            '(light)green, brown, yellow, (light)blue, '
                            '(light)magenta, (light)cyan), a terminal '
                            'color number or an alias; attributes are '
                            'allowed before color (for text color '
                            'only, not background): '
                            '\"%\" for blink, '
                            '\".\" for \"dim\" (half bright), '
                            '\"*\" for bold, '
                            '\"!\" for reverse, '
                            '\"/\" for italic, '
                            '\"_\" for underline'
                        )
                    self.write(f'* [[option_{config}.{section}.{option}]] '
                               f'*{config}.{section}.{option}*')
                    self.write('** %s: pass:none[%s]',
                               _('description'), desc.replace(']', '\\]'))
                    self.write('** %s: %s', _('type'), type_nls)
                    self.write('** %s: %s', _('values'), values)
                    self.write('** %s: `+%s+`',
                               _('default value'), default_value)
                    if null_value_allowed:
                        self.write('** %s',
                                   _('undefined value allowed (null)'))
            self.write(f'// end::{config}_options[]')

    def _write_user_default_aliases(self, default_aliases):
        """Write default aliases."""
        self.write()
        self.write('// tag::default_aliases[]')
        self.write('[width="100%",cols="2m,5m,5",options="header"]')
        self.write('|===')
        self.write('| %s | %s | %s\n',
                   _('Alias'), _('Command'), _('Completion'))
        for alias in default_aliases:
            self.write('| %s | %s | %s',
                       escape(alias['name']),
                       escape(alias['command']),
                       escape(alias['completion'] or '-'))
        self.write('|===')
        self.write('// end::default_aliases[]')

    def _write_user_irc_colors(self, irc_colors):
        """Write IRC colors."""
        self.write()
        self.write('// tag::irc_colors[]')
        self.write('[width="50%",cols="^2m,3",options="header"]')
        self.write('|===')
        self.write('| %s | %s\n', _('IRC color'), _('WeeChat color'))
        for color in irc_colors:
            self.write('| %s | %s',
                       escape(color['color_irc']),
                       escape(color['color_weechat']))
        self.write('|===')
        self.write('// end::irc_colors[]')

    def _write_api_infos(self, infos):
        """Write infos."""
        self.write()
        self.write('// tag::infos[]')
        self.write('[width="100%",cols="^1,^2,6,6",options="header"]')
        self.write('|===')
        self.write('| %s | %s | %s | %s\n',
                   _('Plugin'), _('Name'), _('Description'), _('Arguments'))
        for plugin in sorted(infos):
            for info in sorted(infos[plugin]):
                _inf = infos[plugin][info]
                desc = translate(_inf['description'])
                args_desc = translate(_inf['args_description']) or '-'
                self.write('| %s | %s | %s | %s\n',
                           escape(plugin), escape(info), escape(desc),
                           escape(args_desc))
        self.write('|===')
        self.write('// end::infos[]')

    def _write_api_infos_hashtable(self, infos_hashtable):
        """Write infos hashtable."""
        self.write()
        self.write('// tag::infos_hashtable[]')
        self.write('[width="100%",cols="^1,^2,6,6,8",options="header"]')
        self.write('|===')
        self.write('| %s | %s | %s | %s | %s\n',
                   _('Plugin'), _('Name'), _('Description'),
                   _('Hashtable (input)'), _('Hashtable (output)'))
        for plugin in sorted(infos_hashtable):
            for info in sorted(infos_hashtable[plugin]):
                _inh = infos_hashtable[plugin][info]
                desc = translate(_inh['description'])
                args_desc = translate(_inh['args_description']) or '-'
                output_desc = translate(_inh['output_description']) or '-'
                self.write('| %s | %s | %s | %s | %s\n',
                           escape(plugin), escape(info), escape(desc),
                           escape(args_desc), escape(output_desc))
        self.write('|===')
        self.write('// end::infos_hashtable[]')

    def _write_api_infolists(self, infolists):
        """Write infolists."""
        self.write()
        self.write('// tag::infolists[]')
        self.write('[width="100%",cols="^1,^2,5,5,5",options="header"]')
        self.write('|===')
        self.write('| %s | %s | %s | %s | %s\n',
                   _('Plugin'), _('Name'), _('Description'), _('Pointer'),
                   _('Arguments'))
        for plugin in sorted(infolists):
            for infolist in sorted(infolists[plugin]):
                _inl = infolists[plugin][infolist]
                desc = translate(_inl['description'])
                pointer_desc = translate(_inl['pointer_description']) or '-'
                args_desc = translate(_inl['args_description']) or '-'
                self.write('| %s | %s | %s | %s | %s\n',
                           escape(plugin), escape(infolist), escape(desc),
                           escape(pointer_desc), escape(args_desc))
        self.write('|===')
        self.write('// end::infolists[]')

    def _write_api_hdata(self, hdata):
        """Write hdata."""
        self.write()
        self.write('// tag::hdata[]')
        self.write(':hdata_update_create: __create')
        self.write(':hdata_update_delete: __delete')
        self.write('[width="100%",cols="^1,^2,2,2,5",options="header"]')
        self.write('|===')
        self.write('| %s | %s | %s | %s | %s\n',
                   _('Plugin'), _('Name'), _('Description'), _('Lists'),
                   _('Variables'))
        for plugin in sorted(hdata):
            for hdata_name in sorted(hdata[plugin]):
                _hda = hdata[plugin][hdata_name]
                anchor = f'hdata_{hdata_name}'
                desc = translate(_hda['description'])
                variables = _hda['vars']
                vars_update = _hda['vars_update']
                lists = _hda['lists']
                self.write(f'| {escape(plugin)}')
                self.write(f'| [[{escape(anchor)}]]<<{escape(anchor)},'
                           f'{escape(hdata_name)}>>')
                self.write(f'| {escape(desc)}')
                str_lists = escape(lists) if lists else '-'
                self.write(f'| {str_lists}')
                self.write(f'| {escape(variables)}')
                if vars_update:
                    self.write('*%s* +\n%s',
                               _('Update allowed:'), escape(vars_update))
                self.write()
        self.write('|===')
        self.write('// end::hdata[]')

    def _write_api_completions(self, completions):
        """Write completions."""
        self.write()
        self.write('// tag::completions[]')
        self.write('[width="100%",cols="^1,^2,7",options="header"]')
        self.write('|===')
        self.write('| %s | %s | %s\n',
                   _('Plugin'), _('Name'), _('Description'))
        for plugin in sorted(completions):
            for completion_item in sorted(completions[plugin]):
                _cmp = completions[plugin][completion_item]
                desc = translate(_cmp['description'])
                self.write('| %s | %s | %s\n',
                           escape(plugin), escape(completion_item),
                           escape(desc))
        self.write('|===')
        self.write('// end::completions[]')

    def _write_api_url_options(self, url_options):
        """Write URL options."""
        self.write()
        self.write('// tag::url_options[]')
        self.write('[width="100%",cols="2,^1,7",options="header"]')
        self.write('|===')
        self.write('| %s | %s ^(1)^ | %s ^(2)^\n',
                   _('Option'), _('Type'), _('Constants'))
        for option in url_options:
            constants = option['constants']
            if constants:
                constants = ' ' + constants
            self.write('| %s | %s |%s\n',
                       escape(option['name']), escape(option['type']),
                       escape(constants))
        self.write('|===')
        self.write('// end::url_options[]')

    def _write_api_plugins_priority(self, plugins_priority):
        """Write plugins priority."""
        self.write()
        self.write('// tag::plugins_priority[]')
        for priority in sorted(plugins_priority, reverse=True):
            plugins = ', '.join(sorted(plugins_priority[priority]))
            self.write('. %s (%s)', escape(plugins), priority)
        self.write('// end::plugins_priority[]')


def docgen_cmd_cb(data, buf, args):
    """Callback for /docgen command."""
    doc_directory = data
    locales = args.split(' ') if args else sorted(LOCALE_LIST)

    weechat_doc = WeechatDoc().read_doc()

    weechat.prnt('', '-' * 75)

    for locale in locales:
        autogen = AutogenDoc(weechat_doc, doc_directory, locale)
        weechat.prnt('', f'docgen: {autogen}')

    weechat.prnt('', '-' * 75)

    return weechat.WEECHAT_RC_OK


def docgen_completion_cb(data, completion_item, buf, completion):
    """Callback for completion."""
    for locale in LOCALE_LIST:
        weechat.completion_list_add(completion, locale, 0,
                                    weechat.WEECHAT_LIST_POS_SORT)
    return weechat.WEECHAT_RC_OK


if __name__ == '__main__' and IMPORT_OK:
    if weechat.register(SCRIPT_NAME, SCRIPT_AUTHOR, SCRIPT_VERSION,
                        SCRIPT_LICENSE, SCRIPT_DESC, '', ''):
        weechat.hook_command(
            SCRIPT_COMMAND,
            'Documentation generator.',
            '[locales]',
            'locales: list of locales to build (by default build all locales)',
            '%(docgen_locales)|%*',
            'docgen_cmd_cb',
            os.path.dirname(__file__),
        )
        weechat.hook_completion(
            'docgen_locales',
            'locales for docgen',
            'docgen_completion_cb',
            '',
        )

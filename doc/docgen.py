# -*- coding: utf-8 -*-
#
# Copyright (C) 2008-2012 Sébastien Helleu <flashcode@flashtux.org>
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

#
# Documentation generator for WeeChat: build include files with commands,
# options, infos, infolists, hdata and completions for WeeChat core and plugins.
#
# Instructions to build config files yourself in WeeChat directories (replace
# all paths with your path to WeeChat):
#     1.  run WeeChat and load this script, with following command:
#           /python load ~/src/weechat/doc/docgen.py
#     2.  change path to build in your doc/ directory:
#           /set plugins.var.python.docgen.path "~/src/weechat/doc"
#     3.  run docgen command:
#           /docgen
# (it is recommended to load only this script when building doc)
# Files should be in ~/src/weechat/doc/xx/autogen/ (where xx is language)
#

SCRIPT_NAME    = 'docgen'
SCRIPT_AUTHOR  = 'Sébastien Helleu <flashcode@flashtux.org>'
SCRIPT_VERSION = '0.1'
SCRIPT_LICENSE = 'GPL3'
SCRIPT_DESC    = 'Documentation generator for WeeChat'

SCRIPT_COMMAND = 'docgen'

import_ok = True

try:
    import weechat
except ImportError:
    print('This script must be run under WeeChat.')
    print('Get WeeChat now at: http://www.weechat.org/')
    import_ok = False

try:
    import os, gettext, re, hashlib
    from collections import defaultdict
    from operator import itemgetter
except ImportError as message:
    print('Missing package(s) for %s: %s' % (SCRIPT_NAME, message))
    import_ok = False

# default path where doc files will be written (should be doc/ in sources
# package tree)
# path must have subdirectories with languages and autogen directory:
#      path
#       |-- en
#       |   |-- autogen
#       |-- fr
#       |   |-- autogen
#       ...
DEFAULT_PATH = '~/src/weechat/doc'

# list of locales for which we want to build doc files to include
locale_list = ('en_US', 'fr_FR', 'it_IT', 'de_DE')

# all commands/options/.. of following plugins will produce a file
# non-listed plugins will be ignored
# value: "c" = plugin may have many commands
#        "o" = write config options for plugin
# if plugin is listed without "c", that means plugin has only one command
# /name (where "name" is name of plugin)
# Note: we consider core is a plugin called "weechat"
plugin_list = { 'weechat'  : 'co',
                'alias'    : '',
                'aspell'   : 'o',
                'charset'  : 'co',
                'demo'     : 'co',
                'fifo'     : 'co',
                'irc'      : 'co',
                'logger'   : 'co',
                'relay'    : 'co',
                'rmodifier': 'co',
                'perl'     : '',
                'python'   : '',
                'ruby'     : '',
                'lua'      : '',
                'tcl'      : '',
                'guile'    : '',
                'xfer'     : 'co' }

# options to ignore
ignore_options = ( 'aspell\.dict\..*',
                   'aspell\.option\..*',
                   'charset\.decode\..*',
                   'charset\.encode\..*',
                   'irc\.msgbuffer\..*',
                   'irc\.ctcp\..*',
                   'irc\.ignore\..*',
                   'irc\.server\..*',
                   'jabber\.server\..*',
                   'logger\.level\..*',
                   'logger\.mask\..*',
                   'relay\.port\..*',
                   'rmodifier\.modifier\..*',
                   'weechat\.palette\..*',
                   'weechat\.proxy\..*',
                   'weechat\.bar\..*',
                   'weechat\.debug\..*',
                   'weechat\.notify\..*' )

# completions to ignore
ignore_completions_items = ( 'docgen.*',
                             'jabber.*',
                             'weeget.*' )

def get_commands():
    """Get list of commands in a dict with 3 indexes: plugin, command, xxx."""
    global plugin_list
    commands = defaultdict(lambda: defaultdict(defaultdict))
    infolist = weechat.infolist_get('hook', '', 'command')
    while weechat.infolist_next(infolist):
        plugin = weechat.infolist_string(infolist, 'plugin_name') or 'weechat'
        if plugin in plugin_list:
            command = weechat.infolist_string(infolist, 'command')
            if command == plugin or 'c' in plugin_list[plugin]:
                for key in ('description', 'args', 'args_description', 'completion'):
                    commands[plugin][command][key] = weechat.infolist_string(infolist, key)
    weechat.infolist_free(infolist)
    return commands

def get_options():
    """Get list of config options in a dict with 4 indexes: config, section, option, xxx."""
    global plugin_list, ignore_options
    options = defaultdict(lambda: defaultdict(lambda: defaultdict(defaultdict)))
    infolist = weechat.infolist_get('option', '', '')
    while weechat.infolist_next(infolist):
        full_name = weechat.infolist_string(infolist, 'full_name')
        if not re.search('|'.join(ignore_options), full_name):
            config = weechat.infolist_string(infolist, 'config_name')
            if config in plugin_list and 'o' in plugin_list[config]:
                section = weechat.infolist_string(infolist, 'section_name')
                option = weechat.infolist_string(infolist, 'option_name')
                for key in ('type', 'string_values', 'default_value', 'description'):
                    options[config][section][option][key] = weechat.infolist_string(infolist, key)
                for key in ('min', 'max', 'null_value_allowed'):
                    options[config][section][option][key] = weechat.infolist_integer(infolist, key)
    weechat.infolist_free(infolist)
    return options

def get_infos():
    """Get list of infos hooked by plugins in a dict with 3 indexes: plugin, name, xxx."""
    infos = defaultdict(lambda: defaultdict(defaultdict))
    infolist = weechat.infolist_get('hook', '', 'info')
    while weechat.infolist_next(infolist):
        info_name = weechat.infolist_string(infolist, 'info_name')
        plugin = weechat.infolist_string(infolist, 'plugin_name') or 'weechat'
        for key in ('description', 'args_description'):
            infos[plugin][info_name][key] = weechat.infolist_string(infolist, key)
    weechat.infolist_free(infolist)
    return infos

def get_infos_hashtable():
    """Get list of infos (hashtable) hooked by plugins in a dict with 3 indexes: plugin, name, xxx."""
    infos_hashtable = defaultdict(lambda: defaultdict(defaultdict))
    infolist = weechat.infolist_get('hook', '', 'info_hashtable')
    while weechat.infolist_next(infolist):
        info_name = weechat.infolist_string(infolist, 'info_name')
        plugin = weechat.infolist_string(infolist, 'plugin_name') or 'weechat'
        for key in ('description', 'args_description', 'output_description'):
            infos_hashtable[plugin][info_name][key] = weechat.infolist_string(infolist, key)
    weechat.infolist_free(infolist)
    return infos_hashtable

def get_infolists():
    """Get list of infolists hooked by plugins in a dict with 3 indexes: plugin, name, xxx."""
    infolists = defaultdict(lambda: defaultdict(defaultdict))
    infolist = weechat.infolist_get('hook', '', 'infolist')
    while weechat.infolist_next(infolist):
        infolist_name = weechat.infolist_string(infolist, 'infolist_name')
        plugin = weechat.infolist_string(infolist, 'plugin_name') or 'weechat'
        for key in ('description', 'pointer_description', 'args_description'):
            infolists[plugin][infolist_name][key] = weechat.infolist_string(infolist, key)
    weechat.infolist_free(infolist)
    return infolists

def get_hdata():
    """Get list of hdata hooked by plugins in a dict with 3 indexes: plugin, name, xxx."""
    hdata = defaultdict(lambda: defaultdict(defaultdict))
    infolist = weechat.infolist_get('hook', '', 'hdata')
    while weechat.infolist_next(infolist):
        hdata_name = weechat.infolist_string(infolist, 'hdata_name')
        plugin = weechat.infolist_string(infolist, 'plugin_name') or 'weechat'
        hdata[plugin][hdata_name]['description'] = weechat.infolist_string(infolist, 'description')
        variables = ''
        lists = ''
        ptr_hdata = weechat.hdata_get(hdata_name)
        if ptr_hdata:
            hdata2 = []
            string = weechat.hdata_get_string(ptr_hdata, 'var_keys_values')
            if string:
                for item in string.split(','):
                    (key, value) = item.split(':')
                    var_type = int(value) >> 16
                    var_offset = int(value) & 0xFFFF
                    var_array_size = weechat.hdata_get_var_array_size_string(ptr_hdata, '', key)
                    if var_array_size:
                        var_array_size = ', array_size: \'%s\'' % var_array_size
                    var_hdata = weechat.hdata_get_var_hdata(ptr_hdata, key)
                    if var_hdata:
                        var_hdata = ', hdata: \'%s\'' % var_hdata
                    hdata2.append({'offset': var_offset,
                                   'text': '\'%s\' (%s%s%s)' % (key,
                                                                weechat.hdata_get_var_type_string(ptr_hdata, key),
                                                                var_array_size,
                                                                var_hdata)})
                hdata2 = sorted(hdata2, key=itemgetter('offset'))
                for item in hdata2:
                    if variables:
                        variables += ' +\n'
                    variables += '  %s' % item['text']
            hdata[plugin][hdata_name]['vars'] = '\n%s' % variables

            string = weechat.hdata_get_string(ptr_hdata, 'list_keys')
            if string:
                for item in sorted(string.split(',')):
                    if lists:
                        lists += ' +\n'
                    lists += '  \'%s\'' % item
                lists = '\n%s' % lists
            else:
                lists = '\n  -'
            hdata[plugin][hdata_name]['lists'] = lists
    weechat.infolist_free(infolist)
    return hdata

def get_completions():
    """Get list of completions hooked by plugins in a dict with 3 indexes: plugin, item, xxx."""
    global ignore_completions_items
    completions = defaultdict(lambda: defaultdict(defaultdict))
    infolist = weechat.infolist_get('hook', '', 'completion')
    while weechat.infolist_next(infolist):
        completion_item = weechat.infolist_string(infolist, 'completion_item')
        if not re.search('|'.join(ignore_completions_items), completion_item):
            plugin = weechat.infolist_string(infolist, 'plugin_name') or 'weechat'
            completions[plugin][completion_item]['description'] = weechat.infolist_string(infolist, 'description')
    weechat.infolist_free(infolist)
    return completions

def get_url_options():
    """Get list of completions hooked by plugins in a dict with 3 indexes: plugin, item, xxx."""
    url_options = []
    infolist = weechat.infolist_get('url_options', '', '')
    while weechat.infolist_next(infolist):
        url_options.append({ 'name': weechat.infolist_string(infolist, 'name').lower(),
                             'option': weechat.infolist_integer(infolist, 'option'),
                             'type': weechat.infolist_string(infolist, 'type'),
                             'constants': weechat.infolist_string(infolist, 'constants').lower().replace(',', ', ') })
    weechat.infolist_free(infolist)
    return url_options

def update_file(oldfile, newfile, num_files, num_files_updated, obj):
    """Update a doc file."""
    try:
        shaold = hashlib.sha224(open(oldfile, 'r').read()).hexdigest()
    except:
        shaold = ''
    try:
        shanew = hashlib.sha224(open(newfile, 'r').read()).hexdigest()
    except:
        shanew = ''
    if shaold != shanew:
        if os.path.exists(oldfile):
            os.unlink(oldfile)
        os.rename(newfile, oldfile)
        num_files_updated['total1'] += 1
        num_files_updated['total2'] += 1
        num_files_updated[obj] += 1
    else:
        if os.path.exists(oldfile):
            os.unlink(newfile)
    num_files['total1'] += 1
    num_files['total2'] += 1
    num_files[obj] += 1

def docgen_cmd_cb(data, buffer, args):
    """Callback for /docgen command."""
    global locale_list
    if args:
        locales = args.split(' ')
    else:
        locales = locale_list
    commands = get_commands()
    options = get_options()
    infos = get_infos()
    infos_hashtable = get_infos_hashtable()
    infolists = get_infolists()
    hdata = get_hdata()
    completions = get_completions()
    url_options = get_url_options()

    # get path and replace ~ by home if needed
    path = weechat.config_get_plugin('path')
    if path.startswith('~'):
        path = '%s%s' % (os.environ['HOME'], path[1:])

    # write to doc files, by locale
    num_files = defaultdict(int)
    num_files_updated = defaultdict(int)

    translate = lambda s: (s and _(s)) or s
    escape = lambda s: s.replace('|', '\\|')

    for locale in locales:
        for key in num_files:
            if key != 'total2':
                num_files[key] = 0
                num_files_updated[key] = 0
        t = gettext.translation('weechat', weechat.info_get('weechat_localedir', ''),
                                languages=['%s.UTF-8' % locale], fallback=True)
        t.install()
        directory = '%s/%s/autogen' % (path, locale[0:2])
        if not os.path.isdir(directory):
            weechat.prnt('', '%sdocgen error: directory "%s" does not exist' % (weechat.prefix('error'),
                                                                                directory))
            continue
        # write commands
        for plugin in commands:
            filename = '%s/user/%s_commands.txt' % (directory, plugin)
            tmpfilename = '%s.tmp' % filename
            f = open(tmpfilename, 'w')
            for command in sorted(commands[plugin]):
                args = translate(commands[plugin][command]['args'])
                args_formats = args.split(' || ')
                description = translate(commands[plugin][command]['description'])
                args_description = translate(commands[plugin][command]['args_description'])
                f.write('[[command_%s_%s]]\n' % (plugin, command))
                f.write('[command]*`%s`* %s::\n' % (command, description))
                f.write('........................................\n')
                prefix = '/%s  ' % command
                if args_formats != ['']:
                    for fmt in args_formats:
                        f.write('%s%s\n' % (prefix, fmt))
                        prefix = ' ' * len(prefix)
                if args_description:
                    f.write('\n')
                    for line in args_description.split('\n'):
                        f.write('%s\n' % line)
                f.write('........................................\n\n')
            f.close()
            update_file(filename, tmpfilename, num_files, num_files_updated, 'commands')

        # write config options
        for config in options:
            filename = '%s/user/%s_options.txt' % (directory, config)
            tmpfilename = '%s.tmp' % filename
            f = open(tmpfilename, 'w')
            for section in sorted(options[config]):
                for option in sorted(options[config][section]):
                    opt_type = options[config][section][option]['type']
                    string_values = options[config][section][option]['string_values']
                    default_value = options[config][section][option]['default_value']
                    opt_min = options[config][section][option]['min']
                    opt_max = options[config][section][option]['max']
                    null_value_allowed = options[config][section][option]['null_value_allowed']
                    description = translate(options[config][section][option]['description'])
                    type_nls = translate(opt_type)
                    values = ''
                    if opt_type == 'boolean':
                        values = 'on, off'
                    elif opt_type == 'integer':
                        if string_values:
                            values = string_values.replace('|', ', ')
                        else:
                            values = '%d .. %d' % (opt_min, opt_max)
                    elif opt_type == 'string':
                        if opt_max <= 0:
                            values = _('any string')
                        elif opt_max == 1:
                            values = _('any char')
                        elif opt_max > 1:
                            values = '%s (%s: %d)' % (_('any string'),
                                                      _('max chars'),
                                                      opt_max)
                        else:
                            values = _('any string')
                        default_value = '"%s"' % default_value.replace('"', '\\"')
                    elif opt_type == 'color':
                        values = _('a WeeChat color name (default, black, '
                                   '(dark)gray, white, (light)red, (light)green, '
                                   'brown, yellow, (light)blue, (light)magenta, '
                                   '(light)cyan), a terminal color number or '
                                   'an alias; attributes are allowed before '
                                   'color (for text color only, not '
                                   'background): \"*\" for bold, \"!\" for '
                                   'reverse, \"_\" for underline')
                    f.write('* [[option_%s.%s.%s]] *%s.%s.%s*\n' % (config, section, option, config, section, option))
                    f.write('** %s: `%s`\n' % (_('description'), description))
                    f.write('** %s: %s\n' % (_('type'), type_nls))
                    f.write('** %s: %s (%s: `%s`)\n' % (_('values'), values,
                                                        _('default value'), default_value))
                    if null_value_allowed:
                        f.write('** %s\n' % _('undefined value allowed (null)'))
                    f.write('\n')
            f.close()
            update_file(filename, tmpfilename, num_files, num_files_updated, 'options')

        # write infos hooked
        filename = '%s/plugin_api/infos.txt' % directory
        tmpfilename = '%s.tmp' % filename
        f = open(tmpfilename, 'w')
        f.write('[width="100%",cols="^1,^2,6,6",options="header"]\n')
        f.write('|========================================\n')
        f.write('| %s | %s | %s | %s\n\n' % (_('Plugin'), _('Name'), _('Description'), _('Arguments')))
        for plugin in sorted(infos):
            for info in sorted(infos[plugin]):
                description = translate(infos[plugin][info]['description'])
                args_description = translate(infos[plugin][info]['args_description']) or '-'
                f.write('| %s | %s | %s | %s\n\n' % (escape(plugin),
                                                     escape(info),
                                                     escape(description),
                                                     escape(args_description)))
        f.write('|========================================\n')
        f.close()
        update_file(filename, tmpfilename, num_files, num_files_updated, 'infos')

        # write infos (hashtable) hooked
        filename = '%s/plugin_api/infos_hashtable.txt' % directory
        tmpfilename = '%s.tmp' % filename
        f = open(tmpfilename, 'w')
        f.write('[width="100%",cols="^1,^2,6,6,6",options="header"]\n')
        f.write('|========================================\n')
        f.write('| %s | %s | %s | %s | %s\n\n' % (_('Plugin'), _('Name'), _('Description'),
                                                  _('Hashtable (input)'), _('Hashtable (output)')))
        for plugin in sorted(infos_hashtable):
            for info in sorted(infos_hashtable[plugin]):
                description = translate(infos_hashtable[plugin][info]['description'])
                args_description = translate(infos_hashtable[plugin][info]['args_description'])
                output_description = translate(infos_hashtable[plugin][info]['output_description']) or '-'
                f.write('| %s | %s | %s | %s | %s\n\n' % (escape(plugin),
                                                          escape(info),
                                                          escape(description),
                                                          escape(args_description),
                                                          escape(output_description)))
        f.write('|========================================\n')
        f.close()
        update_file(filename, tmpfilename, num_files, num_files_updated, 'infos_hashtable')

        # write infolists hooked
        filename = '%s/plugin_api/infolists.txt' % directory
        tmpfilename = '%s.tmp' % filename
        f = open(tmpfilename, 'w')
        f.write('[width="100%",cols="^1,^2,5,5,5",options="header"]\n')
        f.write('|========================================\n')
        f.write('| %s | %s | %s | %s | %s\n\n' % (_('Plugin'), _('Name'), _('Description'),
                                                  _('Pointer'), _('Arguments')))
        for plugin in sorted(infolists):
            for infolist in sorted(infolists[plugin]):
                description = translate(infolists[plugin][infolist]['description'])
                pointer_description = translate(infolists[plugin][infolist]['pointer_description']) or '-'
                args_description = translate(infolists[plugin][infolist]['args_description']) or '-'
                f.write('| %s | %s | %s | %s | %s\n\n' % (escape(plugin),
                                                          escape(infolist),
                                                          escape(description),
                                                          escape(pointer_description),
                                                          escape(args_description)))
        f.write('|========================================\n')
        f.close()
        update_file(filename, tmpfilename, num_files, num_files_updated, 'infolists')

        # write hdata hooked
        filename = '%s/plugin_api/hdata.txt' % directory
        tmpfilename = '%s.tmp' % filename
        f = open(tmpfilename, 'w')
        f.write('[width="100%",cols="^1,^2,5,5,5",options="header"]\n')
        f.write('|========================================\n')
        f.write('| %s | %s | %s | %s | %s\n\n' % (_('Plugin'), _('Name'), _('Description'),
                                                  _('Variables'), _('Lists')))
        for plugin in sorted(hdata):
            for hdata_name in sorted(hdata[plugin]):
                description = translate(hdata[plugin][hdata_name]['description'])
                variables = hdata[plugin][hdata_name]['vars']
                lists = hdata[plugin][hdata_name]['lists']
                f.write('| %s | %s | %s |%s |%s\n\n' % (escape(plugin),
                                                        escape(hdata_name),
                                                        escape(description),
                                                        escape(variables),
                                                        escape(lists)))
        f.write('|========================================\n')
        f.close()
        update_file(filename, tmpfilename, num_files, num_files_updated, 'hdata')

        # write completions hooked
        filename = '%s/plugin_api/completions.txt' % directory
        tmpfilename = '%s.tmp' % filename
        f = open(tmpfilename, 'w')
        f.write('[width="65%",cols="^1,^2,8",options="header"]\n')
        f.write('|========================================\n')
        f.write('| %s | %s | %s\n\n' % (_('Plugin'), _('Name'), _('Description')))
        for plugin in sorted(completions):
            for completion_item in sorted(completions[plugin]):
                description = translate(completions[plugin][completion_item]['description'])
                f.write('| %s | %s | %s\n\n' % (escape(plugin),
                                                escape(completion_item),
                                                escape(description)))
        f.write('|========================================\n')
        f.close()
        update_file(filename, tmpfilename, num_files, num_files_updated, 'completions')

        # write url options
        filename = '%s/plugin_api/url_options.txt' % directory
        tmpfilename = '%s.tmp' % filename
        f = open(tmpfilename, 'w')
        f.write('[width="100%",cols="2,^1,7",options="header"]\n')
        f.write('|========================================\n')
        f.write('| %s | %s | %s\n\n' % (_('Option'), _('Type'), _('Constants') + ' ^(1)^'))
        for option in url_options:
            constants = option['constants']
            if constants:
                constants = ' ' + constants
            f.write('| %s | %s |%s\n\n' % (escape(option['name']),
                                            escape(option['type']),
                                            escape(constants)))
        f.write('|========================================\n')
        f.close()
        update_file(filename, tmpfilename, num_files, num_files_updated, 'url_options')

        # write counters
        weechat.prnt('',
                     'docgen: %s: %3d files   (%2d cmd, %2d opt, %2d infos, '
                     '%2d infos_hash, %2d infolists, %2d hdata, %2d complt)'
                     % (locale, num_files['total1'],
                        num_files['commands'], num_files['options'],
                        num_files['infos'], num_files['infos_hashtable'],
                        num_files['infolists'], num_files['hdata'],
                        num_files['completions']))
        weechat.prnt('',
                     '               %3d updated (%2d cmd, %2d opt, %2d infos, '
                     '%2d infos_hash, %2d infolists, %2d hdata, %2d complt)'
                     % (num_files_updated['total1'], num_files_updated['commands'],
                        num_files_updated['options'], num_files_updated['infos'],
                        num_files_updated['infos_hashtable'], num_files_updated['infolists'],
                        num_files_updated['hdata'], num_files_updated['completions']))
    weechat.prnt('',
                 'docgen: total: %d files, %d updated' % (num_files['total2'], num_files_updated['total2']))
    return weechat.WEECHAT_RC_OK

def docgen_completion_cb(data, completion_item, buffer, completion):
    """Callback for completion."""
    global locale_list
    for locale in locale_list:
        weechat.hook_completion_list_add(completion, locale, 0, weechat.WEECHAT_LIST_POS_SORT)
    return weechat.WEECHAT_RC_OK

if __name__ == '__main__' and import_ok:
    if weechat.register(SCRIPT_NAME, SCRIPT_AUTHOR, SCRIPT_VERSION, SCRIPT_LICENSE,
                        SCRIPT_DESC, '', ''):
        weechat.hook_command(SCRIPT_COMMAND,
                             'Documentation generator.',
                             '[locales]',
                             'locales: list of locales to build (by default build all locales)',
                             '%(docgen_locales)|%*',
                             'docgen_cmd_cb', '')
        weechat.hook_completion('docgen_locales', 'locales for docgen', 'docgen_completion_cb', '')
        if not weechat.config_is_set_plugin('path'):
            weechat.config_set_plugin('path', DEFAULT_PATH)

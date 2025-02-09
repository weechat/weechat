# WeeChat Upgrade guidelines

These upgrade guidelines only contain instructions for version upgrades which require manual actions.\
If a version is not listed here, that means no particular action is required for this version.

When upgrading from version X to Y, please read and apply all instructions from version X+1 to version Y (both included).

For a list of all changes in each version, please see [CHANGELOG.md](CHANGELOG.md).

## Version 4.6.0

### Relay remote commands

Commands on remote buffers can now be toggled: execution on remote WeeChat or
locally, with a new default key: `Alt`+`Ctrl`+`l` (L).

You can add this key with this command:

```text
/key missing
```

## Version 4.3.1

### Detection of libgcrypt

The detection of libgcrypt has been fixed to properly detect libgcrypt ≥ 1.11.\
As a consequence, the detection of an old version of libgcrypt is failing if the
file `libgcrypt.pc` is not found.\
This affects old distributions like Debian Buster and Ubuntu Bionic.

## Version 4.3.0

### Relay options

The following relay options have been renamed:

- relay.color.status_waiting_auth → relay.color.status_authenticating
- relay.weechat.commands → relay.network.commands (new default value: `*,!quit`)

### Lag in buflist

The lag is now stored in all IRC buffers: server (like it always has been),
channels and private buffers.

Consequently, if you use `${format_lag}` in buflist options, this lag will be
displayed on server and all channels and private buffers.

If you want to display the lag only on server buffer in buflist, you can use
such format:

```text
${if:${type}==server?${format_lag}}
```

### Color of IRC channel modes

The option `irc.color.item_channel_modes` has been moved to core and renamed to
`weechat.color.status_modes`.

### Signal "buffer_line_added"

The signal "buffer_line_added" is now sent for every line added or modified
on a buffer with free content.

## Version 4.2.2

### Script keys

Some arguments to the `/script` command were renamed in version 4.1.0, but the
keys using these arguments were not changed at same time.

They're now using the new arguments by default, but you must reset manually
the keys with the following commands:

```text
/reset weechat.key_mouse.@chat(script.scripts):button1
/reset weechat.key_mouse.@chat(script.scripts):button2
/reset weechat.key_mouse.@chat(script.scripts):wheeldown
/reset weechat.key_mouse.@chat(script.scripts):wheelup
```

## Version 4.2.0

### Microseconds in buffer lines

Microseconds have been added in buffer lines (for both date and printed date).

Here are the changes that could affect plugins and scripts:

- hook_print: the C callback receives a new argument "date_usec" (microseconds
  of date), after the argument "date" (scripting API is unchanged: the
  microseconds are not available)
- trigger of types "print" and "timer": the format of variable `${tg_date}` is
  changed from `%Y-%m-%d %H:%M:%S` to `%FT%T.%f` (where `%f` is the number of
  microseconds on 6 digits)

### IRC anti-flood

The anti-flood mechanism in IRC plugin has been improved and is now configured
in milliseconds instead of seconds.\
It is done with a single option `irc.server_default.anti_flood` (and same option
in servers), which replaces both options `anti_flood_prio_high` and
`anti_flood_prio_low`.

The default value is 2000 (2 seconds), and for example if you want to set
a delay of 0.5 seconds between your messages sent:

```text
/set irc.server_default.anti_flood 500
```

When upgrading from an old WeeChat version, you'll see such messages, which are
perfectly normal (they're displayed to warn you about unknown options, and then
you have to set the new option if needed):

```text
=!= | Warning: /home/user/.config/weechat/irc.conf, line 131: ignoring unknown option for section "server_default": anti_flood_prio_high = 2
=!= | Warning: /home/user/.config/weechat/irc.conf, line 132: ignoring unknown option for section "server_default": anti_flood_prio_low = 2
=!= | Warning: /home/user/.config/weechat/irc.conf, line 212: ignoring invalid value for option in section "server": libera.anti_flood_prio_high
=!= | Warning: //home/user/.config/weechat/irc.conf, line 213: ignoring invalid value for option in section "server": libera.anti_flood_prio_low
```

### Search in commands history

Search in commands history has been added with new keys and a new key context
called "histsearch".

Some existing keys have been moved as well.

New keys can be changed and added with the following commands after upgrade
from an old WeeChat version:

```text
/key missing
/key unbind ctrl-s,ctrl-u
/key bind meta-U /allbuf /buffer set unread
/key bind ctrl-r /input search_history
/key bindctxt search ctrl-r /input search_previous
```

### RGB colors in IRC messages

Support for RGB colors in IRC messages has been added and a new key
`Ctrl`+`c`, `d` is available to insert this color code in command line.

You can add this key with this command:

```text
/key missing
```

### Custom bar items

Custom bar items must now have a different name than default bar items
(for example the custom bar item name `time` is now forbidden).\
If you have such names in your config, WeeChat will now fail to load them
(this should not happen anyway, since such bar items cannot be properly used
or can cause a crash of WeeChat).

### Nick color infos

The infos irc_nick_color and irc_nick_color_name are deprecated again, and the
algorithm to compute IRC nick colors has been reverted to case-sensitive.\
The server name has been removed from arguments.

## Version 4.1.1

### Custom bar items

Custom bar items must now have a different name than default bar items
(for example the custom bar item name `time` is now forbidden).\
If you have such names in your config, WeeChat will now fail to load them
(this should not happen anyway, since such bar items cannot be properly used
or can cause a crash of WeeChat).

## Version 4.1.0

### New keys to move in cursor mode

New keys have been added to move in cursor mode, and existing keys to move to
another area have been changed: modifier is now `Alt`+`Shift` instead of
`Alt`.

You can change existing keys and add new ones with these commands:

```text
/key bindctxt cursor meta-up /cursor move edge_top
/key bindctxt cursor meta-down /cursor move edge_bottom
/key bindctxt cursor meta-left /cursor move edge_left
/key bindctxt cursor meta-right /cursor move edge_right
/key bindctxt cursor meta-end /cursor move bottom_right
/key bindctxt cursor meta-home /cursor move top_left
/key bindctxt cursor meta-shift-up /cursor move area_up
/key bindctxt cursor meta-shift-down /cursor move area_down
/key bindctxt cursor meta-shift-left /cursor move area_left
/key bindctxt cursor meta-shift-right /cursor move area_right
```

### IRC CTCP replies

IRC CTCP replies are now evaluated, with the same variables available, so now
the syntax is for example `${version}` instead of `$version`.\
The existing options `irc.ctcp.*` are automatically converted on upgrade.

In addition, for privacy reasons, these default CTCP replies have been removed:

- FINGER
- USERINFO

If ever you want that WeeChat replies to these CTCP requests, you can add them
back with the two following commands:

```text
/set irc.ctcp.finger "WeeChat ${version}"
/set irc.ctcp.userinfo "${username} (${realname})"
```

They will then be advertised in reply to "CTCP CLIENTINFO", which is now built
dynamically with these options.

### Nick color infos

Two infos to get nick colors have been added: nick_color_ignore_case and
nick_color_name_ignore_case.\
They are similar to nick_color and nick_color_name, except they take as second
argument a range of chars to apply on the nick: the nick is converted to lower
case using this range of chars.

The infos irc_nick_color and irc_nick_color_name, that were deprecated since
version 1.5 are now used again, with a change in parameter: the server is now
optional before the nick: "server,nick".\
The nick is first converted to lower case, following the value of CASEMAPPING
on the server, then hashed to compute the color.\
That means the color for a nick is now case-insensitive (in the way IRC servers
are case-insensitive, so with a limited range of chars only).

If a script was using this info with a comma in nickname (which should not happen
anyway), this is now interpreted as the server name, and the script must be
modified.\
Anyway, it is recommended to always give the server name to respect the
CASEMAPPING of the server and prevent any issue with a comma in the nickname.

For example nick color of "alice" and "ALICE" is now always guaranteed to be the
same:

```python
# with server name (recommended)
weechat.info_get("irc_nick_color", "libera,alice") == weechat.info_get("irc_nick_color", "libera,ALICE")
weechat.info_get("irc_nick_color_name", "libera,alice") == weechat.info_get("irc_nick_color_name", "libera,ALICE")

# without server name (allowed but not recommended, kept for compatibility)
weechat.info_get("irc_nick_color", "alice") == weechat.info_get("irc_nick_color", "ALICE")
weechat.info_get("irc_nick_color_name", "alice") == weechat.info_get("irc_nick_color_name", "ALICE")
```

### Allowed values for options on fset buffer

A new variable `allowed_values` has been added in fset options.

The default value for the second format has changed.\
You can reset it with this command:

```text
/reset fset.format.option2
```

### Mouse actions on fset buffer

Mouse actions on fset buffer has been fixed when option `fset.look.format_number`
is set to 2.

The key for button 1 on fset buffer has been fixed.\
You can reset it with this command:

```text
/reset weechat.key_mouse.@chat(fset.fset):button1
```

## Version 4.0.6

### Custom bar items

Custom bar items must now have a different name than default bar items
(for example the custom bar item name `time` is now forbidden).

If you have such names in your config, WeeChat will now fail to load them
(this should not happen anyway, since such bar items cannot be properly used
and can cause a crash of WeeChat).

## Version 4.0.1

### Functions config_set_plugin and config_set_desc_plugin

The functions [config_set_plugin](https://weechat.org/doc/weechat/plugin/#_config_set_plugin)
and [config_set_desc_plugin](https://weechat.org/doc/weechat/plugin/#_config_set_desc_plugin)
are not converting anymore the option name to lower case because since version 4.0.0,
the name of options is case-sensitive.

### Grab raw key and command

Key `Alt`+`K` (upper case) has been removed, as well as commands
`/input grab_raw_key` and `/input grab_raw_key_command`.

Now the key `Alt`+`k` displays the actual key name and command, possibly
raw key.

## Version 4.0.0

This is a major version that includes breaking changes described below.

### Support of new IRC capabilities

Support of new capabilities has been introduced in this version and are all
enabled by default, if the server supports them:

- batch
- draft/multiline
- echo-message

When the capability "echo-message" is enabled, you may notice time before your
own IRC messages are displayed in the buffer, this is normal : the capability
forces the server to sent back messages, and WeeChat displays messages only
when they are received from the server.

If you want to disable this capability on all servers, you can do:

```text
/set irc.server_default.capabilities "*,!echo-message"
```

If you are already connected to a server with echo-message enabled, just ask
the server to disable the capability on-the-fly (no need to reconnect):

```text
/cap req -echo-message
```

### Configuration file version

A version has been introduced in configuration file, and due to the many
breaking changes (listed in the chapters below), the following files are
automatically upgraded to a new version:

- weechat.conf: new key names
  (see <<v4.0.0_key_bindings_improvements,Key bindings improvements>>)
- alias.conf: aliases converted to lower case
  (see <<v4.0.0_case_sensitive_identifiers,Case-sensitive identifiers>>)
- irc.conf: options `ssl*` renamed to `tls*`
  (see <<v4.0.0_tls,TLS options and connections>>)
- relay.conf: options and protocol `ssl*` renamed to `tls*`
  (see <<v4.0.0_tls,TLS options and connections>>)

> [!WARNING]
> Because of this new format, you must **NOT** load the new configuration
files in any older WeeChat version < 4.0.0 once you have run any version ≥ 4.0.0
at least one time.\
For example the new key names make the input completely broken (you cannot
enter most chars in input anymore and Enter key does not work).

### Key bindings improvements

The format of key bindings has changed to be more user-friendly, and this is
a breaking change: legacy keys are automatically converted, but some triggers,
plugins or scripts might need manual changes.

Overview of new features:

- use of alias for keys (`meta-left` instead of `meta2-1;3D`)
- use comma to separate keys in combos (`meta-w,meta-up` instead of
  `meta-wmeta-meta2-A`)
- control keys are converted to lower keys (`ctrl-a` instead of `ctrl-A`)
- keys are normal options, so they are shown and can be updated with `/set`
  and `/fset` commands
- command `/key` without arguments opens the fset buffer with all keys

See [Key bindings improvements specification](https://specs.weechat.org/specs/2023-002-key-bindings-improvements.html)
for more information.

#### New key format

Aliases are now used for keys, like `f1`, `home`, `return`, etc.\
In addition, a comma is now required between different keys, for example `ctrl-cb`
is not valid anymore and must be replaced by `ctrl-c,b`.

The keys in weechat.conf are automatically converted from legacy format on first
run or upgrade with a legacy configuration file.

For keys bound in external plugins or scripts, WeeChat tries to convert them
on-the-fly to stay compatible, but this cannot work in all cases (this is a
breaking change).

The following fixes are done on keys when they are defined:

- transform upper case ctrl keys to lower case
- replace space char by `space`
- replace `meta2-` by `meta-[` (modifier `meta2-` doesn't exist anymore)
- mouse modifiers are now in this order: `alt-` then `ctrl-`.

A warning is displayed when a raw key or invalid key is added.\
For example `meta-[A` (which should be `up`) or `ctrl-ca` (missing comma, it
should be `ctrl-c,a`).

#### Grab raw key and command

New key `Alt`+`K` (upper case) is added to grab raw key and its command.

Most of times this command is not needed, and the existing key `Alt`+`k`
(lower case) is preferred, as it returns the key using the new aliases.

For example:

- key `Alt`+`k` then up arrow displays: `up /input history_previous`
- key `Alt`+`K` then up arrow displays: `meta-[A`

Raw keys have higher priority than key with alias (they are looked first);
they can still be used and bound, but this is not recommended.\
They should be used only in case of problem with the new aliases or with your
terminal.

#### Control keys as lower case

Keys using the `Ctrl` key and a letter are now automatically converted to
lower case.\
That means for example keys `ctrl-q` and `ctrl-Q` are the same and saved as
`ctrl-q` (lower case).

Example of key being automatically converted to lower case:

```text
/key bind ctrl-Q /print test
```

Output is now:

```text
New key binding (context "default"): ctrl-q => /print test
```

With older releases, upper case was mandatory and lower case letter for control
keys were not working at all.

### Case-sensitive identifiers

Many identifiers are made case-sensitive, including among others:

- configuration files, sections, options
- commands, aliases
- completion (except nick completion)
- bars, bar items
- colors
- filters
- IRC servers
- scripts
- triggers.

See [Case-sensitive identifiers specification](https://specs.weechat.org/specs/2023-001-case-sensitive-identifiers.html)
for more information.

Accordingly, default aliases are now in lower case.\
All aliases (default ones and those added manually) are automatically converted
to lower case with a message like this one:

```text
Alias converted to lower case: "CLOSE" => "close"
```

### TLS options and connections

Option `weechat.color.status_name_insecure` has been added, the buffer name
is now displayed with color `lightmagenta` by default if the connection with
the server is **NOT** made with TLS.

Options, commands and documentation have been updated to "TLS" instead of "SSL":

- core option:
  - weechat.color.status_name_ssl → weechat.color.status_name_tls
- IRC server default options:
  - irc.server_default.ssl → irc.server_default.tls
  - irc.server_default.ssl_cert → irc.server_default.tls_cert
  - irc.server_default.ssl_dhkey_size → irc.server_default.tls_dhkey_size
  - irc.server_default.ssl_fingerprint → irc.server_default.tls_fingerprint
  - irc.server_default.ssl_password → irc.server_default.tls_password
  - irc.server_default.ssl_priorities → irc.server_default.tls_priorities
  - irc.server_default.ssl_verify → irc.server_default.tls_verify
- IRC options for a specific server:
  - irc.server.xxx.ssl → irc.server.xxx.tls
  - irc.server.xxx.ssl_cert → irc.server.xxx.tls_cert
  - irc.server.xxx.ssl_dhkey_size → irc.server.xxx.tls_dhkey_size
  - irc.server.xxx.ssl_fingerprint → irc.server.xxx.tls_fingerprint
  - irc.server.xxx.ssl_password → irc.server.xxx.tls_password
  - irc.server.xxx.ssl_priorities → irc.server.xxx.tls_priorities
  - irc.server.xxx.ssl_verify → irc.server.xxx.tls_verify
- Relay options:
  - relay.network.ssl_cert_key → relay.network.tls_cert_key
  - relay.network.ssl_priorities → relay.network.tls_priorities
- Relay:
  - protocol `ssl` → `tls`
- Relay command:
  - `/relay sslcertkey` → `/relay tlscertkey`

Default value of option `irc.server_default.tls` is now `on`.\
Connection to IRC servers is done with TLS and port 6697 by default.

For example to create libera.chat server, using TLS (if option
`irc.server_default.tls` is `on`) and default port 6697:

```text
/server add libera irc.libera.chat

irc: server added: libera -> irc.libera.chat/6697 (TLS: enabled)
```

To force non-TLS connection (with default port 6667):

```text
/server add libera irc.libera.chat -notls

irc: server added: libera -> irc.libera.chat/6667 (TLS: disabled)
```

### Insert of multiple pasted lines in input

By default multiple pasted lines are now inserted in input and not sent immediately.

To enable this feature, the default value of option `weechat.look.paste_max_lines`
has been changed to `100` (it was `1`).\
If needed, you can reset the option to the new default value after upgrade:

```text
/reset weechat.look.paste_max_lines
```

The option `weechat.look.paste_auto_add_newline` has been removed.

### Default triggers

The default triggers "cmd_pass", "cmd_pass_register" and "server_pass" have
been updated to be compatible with multiline input.

You can restore these default triggers with the following command:

```text
/trigger restore cmd_pass cmd_pass_register server_pass
```

### Build

#### Autotools

The autotools support for build of WeeChat has been removed.\
WeeChat must now be built with CMake.

#### Documentation

The auto-generated files for documentation are now built with `weechat-headless`,
after compilation of WeeChat and the plugins (the files are not in repository
anymore).\
This implies all plugins must be compiled and loaded in order to have complete docs
(User's guide and Plugin API reference).

If ever you want to disable some plugins and force the build of incomplete docs,
a new option has been added: `ENABLE_DOC_INCOMPLETE` (`OFF` by default).

For example if you disable PHP plugin but still want docs where PHP options,
commands, etc. are missing:

```bash
cmake .. -DENABLE_PHP=OFF -DENABLE_DOC=ON -DENABLE_DOC_INCOMPLETE=ON
```

### Packaging

#### Tarballs

The command `make dist` now builds only `.gz` and `.xz` compressed tarballs.\
Formats `.bz2` and `.zst` are not built anymore.

#### RPM packaging

The file `weechat.spec` used for RPM packaging has been removed.\
openSUSE has its own here:
https://build.opensuse.org/package/view_file/server:irc/weechat/weechat.spec?expand=1

#### cpack

The configuration for cpack has been removed.\
It was used to build binary package of WeeChat, which has never been released
in this format.

### Function bar_new

When the bar name already exists, the API function
[bar_new](https://weechat.org/doc/weechat/plugin/#_bar_new) returns the pointer to
the bar (instead of NULL) and sets the default value for all options with the
values received.\
If you don't want to set default values in an existing bar, it is recommended
to first check if the bar exists with the API function
[bar_search](https://weechat.org/doc/weechat/plugin/#_bar_search).

## Version 3.8

### Move of options out of command /input

Some options of `/input` command have been moved to other commands (they can
still be used with `/input` but marked as deprecated and completion has been
removed):

Old command                             | New command                   | Default key
--------------------------------------- | ----------------------------- | ----------------------
`/input jump_smart`                     | `/buffer jump smart`          | `Alt`+`a`
`/input jump_previously_visited_buffer` | `/buffer jump prev_visited`   | `Alt`+`<`
`/input jump_next_visited_buffer`       | `/buffer jump next_visited`   | `Alt`+`>`
`/input jump_last_buffer_displayed`     | `/buffer jump last_displayed` | `Alt`+`/`
`/input hotlist_clear`                  | `/hotlist clear`              | `Alt`+`h`, `Alt`+`c`
`/input hotlist_remove_buffer`          | `/hotlist remove`             | `Alt`+`h`, `Alt`+`m`
`/input hotlist_restore_buffer`         | `/hotlist restore`            | `Alt`+`h`, `Alt`+`r`
`/input hotlist_restore_all`            | `/hotlist restore -all`       | `Alt`+`h`, `Alt`+`Shift`+`R`
`/input set_unread`                     | `/allbuf /buffer set unread`  | `Ctrl`+`s`, `Ctrl`+`u`
`/input set_unread_current_buffer`      | `/buffer set unread`          | (none)
`/input switch_active_buffer`           | `/buffer switch`              | `Ctrl`+`x`
`/input switch_active_buffer_previous`  | `/buffer switch -previous`    | (none)
`/input zoom_merged_buffer`             | `/buffer zoom`                | `Alt`+`x`

The following default keys can be reset to use the new command:

```text
/key reset meta-a
/key reset meta-<
/key reset meta->
/key reset meta-/
/key reset meta-hmeta-c
/key reset meta-hmeta-m
/key reset meta-hmeta-r
/key reset meta-hmeta-R
/key reset ctrl-Sctrl-U
/key reset ctrl-X
/key reset meta-x
```

### Return code of string comparison functions

The following functions now return arithmetic result of subtracting the last
compared UTF-8 char in string2 from the last compared UTF-8 char in string1:

- string_charcmp
- string_charcasecmp
- string_charcasecmp_range
- string_strcasecmp
- string_strcasecmp_range
- string_strncasecmp
- string_strncasecmp_range
- string_strcmp_ignore_chars

In addition, the case conversion has been extended, now in addition to range
A-Z, all chars that have a lower case version are handled.\
That means for example the case-insensitive comparison of "é" and "É" is 0
(chars are considered equal).

Example with WeeChat 3.8:

```c
int diff = string_strcasecmp ("aaa", "CCC");  /* == -2 */
```

With older releases:

```c
int diff = string_strcasecmp ("aaa", "CCC");  /* == -1 */
```

### API functions string_tolower and string_toupper

The functions [string_tolower](https://weechat.org/doc/weechat/plugin/#_string_tolower)
and [string_toupper](https://weechat.org/doc/weechat/plugin/#_string_toupper)
now return newly allocated string instead of doing the change in place.\
The returned string must then be freed after use.

### Trigger regex command

The trigger regex now starts with a command, which is "s" (regex replace, default)
or "y" (translate chars).

For compatibility, any regex starting with a delimiter different from a letter
will still work.

If you defined some triggers with a regex starting with a letter (used as delimiter),
then you must change them **before** upgrading WeeChat, otherwise they'll be lost
after upgrade (with an error when WeeChat tries to load them from configuration file).

For example this regex is now invalid:

```text
XabcXdefX
```

And must be replaced by:

```text
sXabcXdefX
```

For more information on the regex format, see the trigger chapter in the
_WeeChat User's guide_.

### Remove Python 2 support

The CMake option `ENABLE_PYTHON2` and autotools option `--enable-python2`
have been removed, and WeeChat cannot be compiled with Python 2.x anymore.

### Callbacks of function config_new_option

The two callbacks "callback_change" and "callback_delete" in scripting API function
config_new_option have been changed: an integer return value was expected by error,
now any return value is ignored (like it has always been in the C API).

## Version 3.7

### Argument "object_id" in callback of upgrade_new

In all script languages (except PHP), the argument "object_id" sent to the
callback of "upgrade_new" is now an integer (it was a string in older releases).

To be compatible with all versions, it is recommended to convert the argument
to integer before testing it, for example in Python:

```python
if int(object_id) == 1:
    # ...
```

### Argument "remaining_calls" in callback of hook_timer

In all script languages (except PHP), the argument "remaining_calls" sent to the
callback of "hook_timer" is now an integer (it was a string in older releases).

To be compatible with all versions, it is recommended to convert the argument
to integer before testing it, for example in Python:

```python
if int(remaining_calls) > 0:
    # ...
```

### Delete previous word until whitespace

A new parameter `delete_previous_word_whitespace` has been added in `/input`
command to delete previous word until backspace.\
This is now bound by default to the key `Ctrl`+`w`
(see issue [#559](https://github.com/weechat/weechat/issues/559)).

A new key kbd:[Alt+Backspace] has been added to delete word, like `Ctrl`+`w`
did in previous releases.

You can get the new behavior for `Ctrl`+`w` with this command:

```text
/key bind ctrl-W /input delete_previous_word_whitespace
```

And add the new key `Alt`+`Backspace` with this command:

```text
/key missing
```

### Function string_rebuild_split_string

The API function string_build_with_split_string has been renamed to
[string_rebuild_split_string](https://weechat.org/doc/weechat/plugin/#_string_rebuild_split_string)
and two new arguments have been added: _index_start_ and _index_end_.

To stay compatible, the existing calls to the function must be done with the
new function name and these values:

- _index_start_: `0`
- _index_end_: `-1`

## Version 3.6

### Hook print on empty messages

The "hook_print" callback is now called even when an empty message is displayed
(with or without prefix).

This was a bug, but is mentioned here just in case some scripts callbacks
would be surprised to be called with such empty messages.

### Default trigger "beep"

The command of "beep" trigger is now executed only if the buffer notify is NOT
set to `none` (in addition to existing conditions).

You can restore the default trigger "beep" with the following command:

```text
/trigger restore beep
```

## Version 3.5

### Compression "zstandard" in relay

Relay of type "weechat" now offers a compression with [Zstandard](https://facebook.github.io/zstd/),
which allows better compression and is much faster than zlib for both compression and decompression.

The new compression type is `zstd`, and the default compression is now `off`
instead of `zlib`: the compression must now be explicitly given in the
[handshake](https://weechat.org/doc/weechat/relay/#command_handshake) command.

The option `compression` in [init](https://weechat.org/doc/weechat/relay/#command_handshake)
command has been removed, it is now ignored and must be given in the
[handshake](https://weechat.org/doc/weechat/relay/#command_handshake) command
(it was deprecated since WeeChat 2.9).

The option relay.network.compression_level has been renamed to relay.network.compression
and is now a percentage between `0` and `100`:

- `0`: disable compression
- `1`: low compression (fast)
- `100`: best compression (slow)

## Version 3.4

### Default triggers

The existing triggers "cmd_pass" and "cmd_pass_register" have been updated to
hide key and password in command `/msg nickserv setpass <nick> <key> <password>`
and support the option `-server <name>`.

You can restore the default triggers with the following command:

```text
/trigger restore cmd_pass cmd_pass_register
```

### New parameters in function hdata_search

New parameters have been added in function
[hdata_search](https://weechat.org/doc/weechat/plugin/#_hdata_search), used for the
evaluation of expression.

New parameters are the same as function
[string_eval_expression](https://weechat.org/doc/weechat/plugin/#_string_eval_expression):

- pointers: hashtable with pointers (pointers)
- extra_vars: hashtable with extra variables (strings)
- options: hashtable with options (strings).

The following scripts are updated consequently to be compatible with all
WeeChat versions:

- [autoauth.py](https://weechat.org/scripts/source/autoauth.py/) 1.3
- [buffer_open.py](https://weechat.org/scripts/source/buffer_open.py/) 0.3
- [collapse_channel.py](https://weechat.org/scripts/source/collapse_channel.py/) 0.9
- [grep_filter.py](https://weechat.org/scripts/source/grep_filter.py/) 0.11
- [samechannel.rb](https://weechat.org/scripts/source/samechannel.rb/) 0.2
- [soju.py](https://weechat.org/scripts/source/soju.py/) 0.1.4
- [stalker.pl](https://weechat.org/scripts/source/stalker.pl/) 1.6.3

### Static array support in hdata

Support of static array in hdata has been added.
For pointers to arrays, a prefix `*,` must be added in parameter `array_size`
of API function [hdata_new_var](https://weechat.org/doc/weechat/plugin/#_hdata_new_var).

## Version 3.3

### New keys for hotlist

New keys have been added to manipulate the hotlist:

- `Alt`+`h`, `Alt`+`c`: clear the whole hotlist (former key: `Alt`+`h`)
- `Alt`+`h`, `Alt`+`m`: mark the current buffer as read by removing it from the hotlist
- `Alt`+`h`, `Alt`+`r`: restore latest hotlist removed in the current buffer
- `Alt`+`h`, `Alt`+`Shift`+`R`: restore latest hotlist removed in all buffers

You can add them with the following command:

```text
/key missing
```

Since the key `Alt`+`h` has been moved to `Alt`+`h`, `Alt`+`c`, you must
manually remove the old key:

```text
/key unbind meta-h
```

### Bar item "typing"

A bar item called "typing" has been added to status bar by default. It is used
to display users that are currently typing a message on the current IRC channel
or private buffer.

If you want to display typing notifications in the status bar, add `,[typing]`
in your option weechat.bar.status.items.

### Ordered hashtables

Hashtables entries are now ordered by creation date, the following functions
are now returning entries sorted by insertion order:

- hashtable_map
- hashtable_map_string
- hashtable_get_string (all properties except "keys_sorted" and "keys_values_sorted")
- hashtable_add_to_infolist

### IRC default capabilities

All supported capabilities are now enabled by default if the server support
them:

- account-notify
- away-notify
- cap-notify
- chghost
- extended-join
- invite-notify
- message-tags
- multi-prefix
- server-time
- setname
- userhost-in-names

Two new options have been added and enabled by default to customize the behavior
of capabilities "account-notify" and "extended-join":

- irc.look.display_account_message: display ACCOUNT messages received
- irc.look.display_extended_join: display extended join info in the JOIN
  messages: account name and real name

The default value of option irc.server_default.capabilities is now `*` which
means that all capabilities supported by both WeeChat and the server are enabled
by default.

After upgrade, to enable all capabilities and remove custom capabilities you
have set, you can do:

```text
/set irc.server_default.capabilities "*"
/unset irc.server.example.capabilities
```

You can also explicitly disable some capabilities with this syntax
(see `/help irc.server_default.capabilities`):

```text
/set irc.server_default.capabilities "*,!away-notify,!extended-join"
```

### IRC SASL DH-BLOWFISH and DH-AES mechanisms removed

The SASL mechanisms DH-BLOWFISH and DH-AES have been removed, because they
are insecure and already removed from most IRC servers.\
If you were using one of these mechanisms, it is highly recommended to switch
to any other supported SASL mechanism.

For example:

```text
/set irc.server.example.sasl_mechanism scram-sha-256
```

## Version 3.2

### XDG directories

Support of XDG directories has been added.

For compatibility, if the XDG directories are not found but `~/.weechat` exists,
this single directory is used for all files.

If you want to switch to XDG directories, you must create and move manually
all files in the appropriate directories.\
See [WeeChat XDG specification](https://specs.weechat.org/specs/001285-follow-xdg-base-dir-spec.html#weechat-home)
for more information.

Some options have new default value: `%h` is replaced by `${weechat_xxx_dir}`.\
If you didn't change the value, it is recommended to set the new default value,
by doing `/unset <option>` on each option:

Option                     | Old default value   | New default value
-------------------------- | ------------------- | -------------------------------------------------
fifo.file.path             | `%h/weechat_fifo`   | `${weechat_runtime_dir}/weechat_fifo_${info:pid}`
logger.file.path           | `%h/logs/`          | `${weechat_data_dir}/logs`
relay.network.ssl_cert_key | `%h/ssl/relay.pem`  | `${weechat_config_dir}/ssl/relay.pem`
script.scripts.path        | `%h/script`         | `${weechat_cache_dir}/script`
weechat.plugin.path        | `%h/plugins`        | `${weechat_data_dir}/plugins`
xfer.file.download_path    | `%h/xfer`           | `${weechat_data_dir}/xfer`

The following scripts are updated to take care of XDG directories, be sure
they are all up-to-date, otherwise files may be saved in wrong directories or
the script may not find some files:

- [autoconf.py](https://weechat.org/scripts/source/autoconf.py/) 0.4
- [axolotl.py](https://weechat.org/scripts/source/axolotl.py/) 0.1.1
- [beinc.py](https://weechat.org/scripts/source/beinc.py/) 4.2
- [buddylist.pl](https://weechat.org/scripts/source/buddylist.pl/) 2.1
- [bufsave.py](https://weechat.org/scripts/source/bufsave.py/) 0.5
- [chanop.py](https://weechat.org/scripts/source/chanop.py/) 0.3.4
- [chanstat.py](https://weechat.org/scripts/source/chanstat.py/) 0.2
- [colorize_lines.pl](https://weechat.org/scripts/source/colorize_lines.pl/) 4.0
- [confversion.py](https://weechat.org/scripts/source/confversion.py/) 0.4
- [country.py](https://weechat.org/scripts/source/country.py/) 0.6.2
- [cron.py](https://weechat.org/scripts/source/cron.py/) 0.5
- [crypt.py](https://weechat.org/scripts/source/crypt.py/) 1.4.5
- [grep.py](https://weechat.org/scripts/source/grep.py/) 0.8.5
- [growl.py](https://weechat.org/scripts/source/growl.py/) 1.0.7
- [histman.py](https://weechat.org/scripts/source/histman.py/) 0.8.2
- [hl2file.py](https://weechat.org/scripts/source/hl2file.py/) 0.3
- [hotlist2extern.pl](https://weechat.org/scripts/source/hotlist2extern.pl/) 1.0
- [jnotify.pl](https://weechat.org/scripts/source/jnotify.pl/) 1.2
- [latex_unicode.py](https://weechat.org/scripts/source/latex_unicode.py/) 1.2
- [luanma.pl](https://weechat.org/scripts/source/luanma.pl/) 0.3
- [otr.py](https://weechat.org/scripts/source/otr.py/) 1.9.3
- [pop3_mail.pl](https://weechat.org/scripts/source/pop3_mail.pl/) 0.4
- [purgelogs.py](https://weechat.org/scripts/source/purgelogs.py/) 0.5
- [query_blocker.pl](https://weechat.org/scripts/source/query_blocker.pl/) 1.3
- [queryman.py](https://weechat.org/scripts/source/queryman.py/) 0.6
- [queue.py](https://weechat.org/scripts/source/queue.py/) 0.4.3
- [rslap.pl](https://weechat.org/scripts/source/rslap.pl/) 1.4
- [rssagg.pl](https://weechat.org/scripts/source/rssagg.pl/) 1.3
- [slack.py](https://weechat.org/scripts/source/slack.py/) 2.8.0
- [stalker.pl](https://weechat.org/scripts/source/stalker.pl/) 1.6.2
- [substitution.rb](https://weechat.org/scripts/source/substitution.rb/) 0.0.2
- [triggerreply.py](https://weechat.org/scripts/source/triggerreply.py/) 0.4.3
- [update_notifier.py](https://weechat.org/scripts/source/update_notifier.py/) 0.6
- [url_olde.py](https://weechat.org/scripts/source/url_olde.py/) 0.8
- [urlserver.py](https://weechat.org/scripts/source/urlserver.py/) 2.6
- [weetext.py](https://weechat.org/scripts/source/weetext.py/) 0.1.3
- [zncplayback.py](https://weechat.org/scripts/source/zncplayback.py/) 0.2.1

To check if XDG directories are used, you can run command `/debug dirs`, which
should show different directories for config/data/cache/runtime, like this:

```text
Directories:
  home:
    config: /home/user/.config/weechat
    data: /home/user/.local/share/weechat
    cache: /home/user/.cache/weechat
    runtime: /run/user/1000/weechat
  lib: /usr/lib/x86_64-linux-gnu/weechat
  lib (extra): -
  share: /usr/share/weechat
  locale: /usr/share/locale
```

With the legacy `~/.weechat` directory, the output would be:

```text
Directories:
  home:
    config: /home/user/.weechat
    data: /home/user/.weechat
    cache: /home/user/.weechat
    runtime: /home/user/.weechat
  lib: /usr/lib/x86_64-linux-gnu/weechat
  lib (extra): -
  share: /usr/share/weechat
  locale: /usr/share/locale
```

### GnuTLS certificate authorities

A new option has been added to load system's default trusted certificate
authorities on startup: weechat.network.gnutls_ca_system (boolean, enabled
by default).

The option weechat.network.gnutls_ca_file has been renamed to
weechat.network.gnutls_ca_user and is now used for extra certificates
(not the system ones).\
This option now supports multiple files, separated by colons.

If you have set a user CA file in option weechat.network.gnutls_ca_file,
then you must set this same value in the new option weechat.network.gnutls_ca_user.

When one of these options are changed, all certificates are purged and reloaded
from files.

### Split of commands before evaluation

The split of commands is now performed before the evaluation of string, in the
following cases:

- IRC server option irc.server_default.command or irc.server.xxx.command
- startup option with command line parameter `-r` / `--run-command`
- options weechat.startup.command_before_plugins and weechat.startup.command_after_plugins.

If ever you used here multiple commands that are found by evaluation, then
you must use multiple commands directly.

For example if you did this:

```text
/secure set commands "/command1 secret1;/command2 secret2"
/set irc.server.libera.command "${sec.data.commands}"
```

This will now execute a single command: `/command1` with two parameters:
`secret1;/command2` and `secret2`, which is not what you expect.

So you must now do this instead:

```text
/secure set command1 "/command1 secret1"
/secure set command2 "/command2 secret2"
/set irc.server.libera.command "${sec.data.command1};${sec.data.command2}"
```

You could also do this, but be careful, there are two evaluations of strings
(the secured data itself is evaluated as well):

```text
/secure set commands "/command1 secret1;/command2 secret2"
/set irc.server.libera.command "/eval -s ${sec.data.commands}"
```

## Version 3.1

### External program to read secured data passphrase

A new option `sec.crypt.passphrase_command` has been added to read the passphrase
from the output of an external program (like a password manager).

The option `sec.crypt.passphrase_file` has been removed, because the command
can now read a file as well. If you used a file to read the passphrase, you
must now setup the command like this:

```text
/set sec.crypt.passphrase_command "cat ~/.weechat-passphrase"
```

For security reasons, it is of course highly recommended to use a password manager
or a program to decrypt a file with your passphrase rather than using a file with
the clear password.\
For example with password-store (command `pass`):

```text
/set sec.crypt.passphrase_command "/usr/bin/pass show weechat/passphrase"
```

### Vertical split of windows

The vertical split of windows has been fixed
(see issue [#1612](https://github.com/weechat/weechat/issues/1612)): now the new
window has the asked size, instead of the original window.

For example with this command the new window (on the right) has size 80% instead
of 20% in previous releases:

```text
/window splitv 80
```

### List of buffer local variables

The command `/buffer localvar` has been renamed to `/buffer listvar`.

The option `localvar` is still supported to stay compatible with scripts
calling it or referencing it in the documentation.\
It is deprecated and will be removed in a future release.

New options `setvar` and `delvar` were also added in command `/buffer`,
see `/help buffer`.

### Buflist option buflist.look.use_items

A new buflist option `buflist.look.use_items` has been added to speed up
display of buflist, in case you use a single buflist item (the most common use case).

If ever you use more than one item (item "buflist2" or even "buflist3"), you
must adjust the value of the new option, which defaults to 1:

```text
/set buflist.look.use_items 2
```

## Version 3.0

### New option to enable download of scripts

A new option has been added to allow the script plugin to download the list of
scripts and the scripts themselves (on weechat.org by default).

This option is `off` by default, so you must opt in if you want to use the
`/script` command, even if you upgraded from an old WeeChat version:

```text
/set script.scripts.download_enabled on
```

Note: when this option is enabled, WeeChat can sometimes download again the list
of scripts when you use the `/script` command, even if you don't install a script.

### IRC commands /allchan, /allpv and /allserv

The command and arguments given to commands `/allchan`, `/allpv` and `/allserv`
are now evaluated (see `/help` on the commands for more information).

Additionally, a breaking change has been introduced: the leading `/` is now
required in these commands, so this allows to send text to buffers without
using the command `/msg * xxx`.

So with previous releases, to say "hello" on all channels:

```text
/allchan msg * hello
```

Now it can be done like this:

```text
/allchan hello
```

If you want to use a command, you must add explicitly the leading `/`:

```text
/allchan /msg * hello
```

### Default trigger "beep"

The command of "beep" trigger is now executed only if the message does NOT
contain the tag "notify_none" (in addition to existing conditions).

You can restore the default trigger "beep" with the following command:

```text
/trigger restore beep
```

## Version 2.9

### New background color for inactive bars

A new option has been added in bar: "color_bg_inactive". It is used for window
bars, when the window is not the active window.

By default this color is set to "default" for default bars, except for status
and title: the color is set to "darkgray".

If you upgrade from a previous release, the color will be "default" for all
bars, so if you want to use the new default settings, you can do:

```text
/set weechat.bar.title.color_bg_inactive darkgray
/set weechat.bar.status.color_bg_inactive darkgray
```

If you changed the option "color_bg" in some bars, you should also adjust
the new option "color_bg_inactive", depending on your needs.

The function [bar_new](https://weechat.org/doc/weechat/plugin/#_bar_new) in API is updated,
so this is an incompatible change: all plugins and scripts calling this function must be updated.

The following scripts are updated consequently to be compatible with all
WeeChat versions:

- [buddylist.pl](https://weechat.org/scripts/source/buddylist.pl/) 2.0
- [buffers.pl](https://weechat.org/scripts/source/buffers.pl/) 5.7
- [chanmon.pl](https://weechat.org/scripts/source/chanmon.pl/) 2.6
- [chanop.py](https://weechat.org/scripts/source/chanop.py/) 0.3.2
- [highmon.pl](https://weechat.org/scripts/source/highmon.pl/) 2.7
- [iset.pl](https://weechat.org/scripts/source/iset.pl/) 4.4
- [menu.pl](https://weechat.org/scripts/source/menu.pl/) 1.0
- [moc_control.py](https://weechat.org/scripts/source/moc_control.py/) 1.9
- [newsbar.pl](https://weechat.org/scripts/source/newsbar.pl/) 0.19
- [pv_info.pl](https://weechat.org/scripts/source/pv_info.pl/) 0.0.6
- [rssagg.pl](https://weechat.org/scripts/source/rssagg.pl/) 1.2
- [urlbar.py](https://weechat.org/scripts/source/urlbar.py/) 14
- [urlselect.lua](https://weechat.org/scripts/source/urlselect.lua/) 0.5
- [vimode.py](https://weechat.org/scripts/source/vimode.py/) 0.8

### New modifier_data for modifier "weechat_print"

The modifier "weechat_print" has been fixed and the content of "modifier_data"
sent to the callback has changed (see issue [#42](https://github.com/weechat/weechat/issues/42)).

This is an incompatible change, therefore all plugins, scripts and triggers
using this modifier and the "modifier_data" argument must be updated.

The old format contained plugin name + ";" + buffer name + ";" + tags:

```text
irc;libera.#weechat;tag1,tag2,tag3
```

The new format contains buffer pointer + ";" + tags:

```text
0x123abc;tag1,tag2,tag3
```

The following scripts are updated consequently to be compatible with all
WeeChat versions:

- [colorize_lines.pl](https://weechat.org/scripts/source/colorize_lines.pl/) 3.9
- [colorize_nicks.py](https://weechat.org/scripts/source/colorize_nicks.py/) 27
- [colorizer.rb](https://weechat.org/scripts/source/colorizer.rb/) 0.2
- [curiousignore.pl](https://weechat.org/scripts/source/curiousignore.pl/) 0.4
- [format_lines.pl](https://weechat.org/scripts/source/format_lines.pl/) 1.6
- [identica.py](https://weechat.org/scripts/source/identica.py/) 0.4.3
- [mass_hl_blocker.pl](https://weechat.org/scripts/source/mass_hl_blocker.pl/) 0.2
- [noirccolors.py](https://weechat.org/scripts/source/noirccolors.py/) 0.4
- [parse_relayed_msg.pl](https://weechat.org/scripts/source/parse_relayed_msg.pl/) 1.9.3
- [unhighlight.py](https://weechat.org/scripts/source/unhighlight.py/) 0.1.3
- [weemoticons.py](https://weechat.org/scripts/source/weemoticons.py/) 0.3

### Move of API completion functions

Completion functions have been added in WeeChat 2.9, to allow relay clients or
plugins/scripts to complete a string, without using the buffer input.

Therefore two functions have been renamed in API and moved to the new
"completion" category:

- hook_completion_get_string → [completion_get_string](https://weechat.org/doc/weechat/plugin/#_completion_get_string)
- hook_completion_list_add → [completion_list_add](https://weechat.org/doc/weechat/plugin/#_completion_list_add)

Note: the old names are still valid for compatibility reasons, but it is recommended
to use only the new names as the old ones may be removed in an upcoming release.

### GnuTLS is now a required dependency

The build options `ENABLE_GNUTLS` (in CMake) and `--disable-gnutls` (in autotools)
have been removed. That means now GnuTLS is always compiled and used in WeeChat.

Motivations:

- communications encryption should be built-in, not optional
- GnuTLS library should be available everywhere
- reduce complexity of code and tests of builds.

### The trigger "cmd_pass" does not hide anymore values of /set command

The default trigger "cmd_pass" does not hide anymore values of options in `/set`
command which contain "password" in the name.

The reason is that it was masking values of options that contains the word
"password" but the value is not a password and does not contain sensitive data,
for example these options were affected:

- irc.look.nicks_hide_password
- relay.network.allow_empty_password
- relay.network.password_hash_algo
- relay.network.password_hash_iterations

Since all real password options are now evaluated, it is recommended to use
secure data to store the passwords ciphered in config file.\
By using secure data, the passwords are never displayed on screen (even with
`/set` command) nor written in log files.

For example you can do that:

```text
/secure passphrase my_secret_passphrase
/secure set libera my_password
/set irc.server.libera.sasl_password "${sec.data.libera}"
```

This will be displayed like that in WeeChat, using the new regex value of
"cmd_pass" trigger:

```text
/secure passphrase ********************
/secure set libera ***********
/set irc.server.libera.sasl_password "${sec.data.libera}"
```

If you want to use the new trigger regex after upgrade, you can do:

```text
/trigger restore cmd_pass
```

If ever you prefer the old trigger regex, you can change it like that:

```text
/set trigger.trigger.cmd_pass.regex  "==^((/(msg|m|quote) +(-server +[^ ]+ +)?nickserv +(id|identify|set +password|ghost +[^ ]+|release +[^ ]+|regain +[^ ]+|recover +[^ ]+) +)|/oper +[^ ]+ +|/quote +pass +|/set +[^ ]*password[^ ]* +|/secure +(passphrase|decrypt|set +[^ ]+) +)(.*)==${re:1}${hide:*,${re:+}}"
```

### Evaluation of trigger command arguments

The arguments for a trigger command (except the command itself) are now evaluated.

That means you can use for example new lines in the command description, like that:

```text
/trigger add test command "test;test command;arg1 arg2;arg1: description 1${\n}arg2: description 2"
```

The command `/help test` will display this help in WeeChat:

```text
[trigger]  /test  arg1 arg2

test command

arg1: description 1
arg2: description 2
```

### Add of handshake and nonce in weechat relay protocol

#### Handshake

A `handshake` command has been added in weechat relay protocol.\
The client should send this command before the `init` to negotiate the way to
authenticate with the relay server.

See the [handshake command](https://weechat.org/doc/weechat/relay/#command_handshake)
in Relay protocol doc for more information.

#### Server "nonce"

Furthermore, a "nonce" is now generated for each client connecting and must be
used by the client in case of hashed password in the `init` command.\
The goal is to prevent replay attacks in case someone manages to read exchanges
between the client and relay.

When hashing the password, the client must use salt composed by this nonce
as binary (it is hexadecimal and must be base16-decoded), concatenated with
a client nonce after this one.\
So the hash is computed on: (`server nonce` + `client nonce` + `password`).

This salt is now mandatory even for algorithms `SHA256` and `SHA512`; this is
a breaking change in protocol, needed for security reasons.

See the [init command](https://weechat.org/doc/weechat/relay/#command_init)
in Relay protocol doc for more information.

## Version 2.8

### Auto connection to servers

The command line option `-a` (or `--no-connect`), which can also be used in the
`/plugin` command, is now used to set a new info called `auto_connect`
(see the function [info_get](https://weechat.org/doc/weechat/plugin/#_info_get) in the Plugin API reference).

Therefore, the option is not sent anymore to the function `weechat_plugin_init`
of plugins.\
The plugins using this option must now get the info `auto_connect` and check
if the value is "1" (a string with just `1`).

The purpose of this change is to allow scripts as well to check this info on
startup, and connect or not, depending on the value
(see issue [#1453](https://github.com/weechat/weechat/issues/1453)).

To be compatible with WeeChat ≤ 2.7, the script can do this, for example
in Python:

```python
auto_connect = weechat.info_get("auto_connect", "") != "0"
```

The variable `auto_connect` will be set like that, depending on the WeeChat
version:

- WeeChat ≤ 2.7: always `True` because the info is an empty string (it does not
  exist), which is different from "0",
- WeeChat ≥ 2.8: `True` by default, and `False` if `-a` or `--no-connect` is
  given by the user (either on command line or when loading the plugin).

## Version 2.7

### CMake errors on missing dependencies

When compiling WeeChat with CMake (which is the recommended way), errors are
now displayed on any missing dependency, if the optional feature was enabled
(most features are automatically enabled, except documentation, man page and
tests).

Any error on a missing dependency is fatal, so WeeChat cannot be compiled.
This is a new behavior compared to old versions, where any missing dependency
was silently ignored and the compilation was possible anyway.

For example if PHP is not installed on your system, CMake will display an error
on missing PHP library:

```text
-- checking for one of the modules 'php7'
CMake Warning at cmake/FindPHP.cmake:57 (message):
  Could not find libphp7.  Ensure PHP >=7.0.0 development libraries are
  installed and compiled with `--enable-embed`.  Ensure `php-config` is in
  `PATH`.  You may set `-DCMAKE_LIBRARY_PATH=...` to the directory containing
  libphp7.
Call Stack (most recent call first):
  src/plugins/CMakeLists.txt:157 (find_package)


CMake Error at src/plugins/CMakeLists.txt:161 (message):
  Php not found
```

Then you can either install PHP or explicitly disable PHP if you don't need this
plugin, using this cmake option:

```bash
cmake .. -DENABLE_PHP=OFF
```

### Strings received in Python 3 callbacks

The strings sent to script callbacks in Python 3 are now automatically converted
according to the content:

- if the string is valid UTF-8, it is sent as `str` (legacy behavior)
- if the string is not valid UTF-8, it is sent as `bytes` (new).

In some cases only, the string may not be valid UTF-8, so it is received as
`bytes` in the callback, which must take care of that.

For more information, see the WeeChat scripting guide: chapter about strings
received in callbacks (see also issue [#1389](https://github.com/weechat/weechat/issues/1389)).

Note: there are no changes for Python 2 (which is now deprecated and should not
be used anymore), the strings sent to callbacks are always of type `str`, and
may contain invalid UTF-8 data, in the cases mentioned in the WeeChat scripting
guide.

### IRC message decoding/encoding

A new server option called "charset_message" has been added, replacing the
option irc.network.channel_encode.

This new server option has three possible values:

- _message_ (default): the whole IRC message is decoded/encoded, this is the
  new default behavior; in case of problem with channel names, try to use
  _text_ instead
- _channel_: the message is decoded/encoded starting at the channel name (or
  the text if no channel is present); this is like setting the old option
  irc.network.channel_encode to `on`
- _text_: the message is decoded/encoded starting from the text (for example
  the user message); this is like setting the old option
  irc.network.channel_encode to `off` (so this was the default behavior
  in previous versions)

### Suffix for files received via DCC

Files received via DCC (xfer plugin) now have a suffix ".part" during the
transfer. When the transfer is successful, the suffix is removed.

This suffix can be customized with the new option xfer.file.download_temporary_suffix.

If you prefer the legacy behavior (no suffix added), you can set an empty value
in the new option:

```text
/set xfer.file.download_temporary_suffix ""
```

## Version 2.6

### Python 3 by default

Python 3 is now used by default to compile the "python" plugin (no fallback
on Python 2).

The CMake option `ENABLE_PYTHON3` has been renamed to `ENABLE_PYTHON2`
(configure option `--enable-python2`). If this option is enabled, the "python"
plugin is built with Python 2 (no fallback on Python 3).

### Nick colors

The function to compute the colors based on the nick letters has been fixed
(now the function uses only a 64-bit integer even if the underlying architecture
is 32-bit).

If you're running WeeChat on a 32-bit architecture and want to keep same colors
as the previous releases, you can use one of the two new hash values.

If you were using "djb2", you can switch to "djb2_32":

```text
/set weechat.look.nick_color_hash djb2_32
```

If you were using "sum", you can switch to "sum_32":

```text
/set weechat.look.nick_color_hash sum_32
```

## Version 2.5

### Aspell plugin renamed to Spell

The "aspell" plugin has been renamed to "spell", a more generic term, because
it supports aspell and also enchant.

Consequently, the following things have been renamed as well:

- file aspell.conf → spell.conf (the content of the file has not changed, so you can just rename the file to keep your changes)
- options `aspell.*` → `spell.*`
- command `/aspell` → `/spell`
- default key `Alt`+`s` → `/mute spell toggle`
- bar item aspell_dict → spell_dict
- bar item aspell_suggest → spell_suggest
- info aspell_dict → spell_dict

If you are upgrading from a previous release, you can copy the config file
before doing `/upgrade`, in WeeChat:

```text
/save aspell
/exec -sh cp ~/.weechat/aspell.conf ~/.weechat/spell.conf
/upgrade
```

If you already upgraded WeeChat:

```text
/exec -sh cp ~/.weechat/aspell.conf ~/.weechat/spell.conf
/reload spell
```

Once configuration is OK, you can delete the file `~/.weechat/aspell.conf`.

Then you can search if you are using "aspell" in values of options:

```text
/fset =aspell
```

If there are options displayed, replace "aspell" by "spell" in values.

The default key `Alt`+`s` can be changed to the new `/spell` command:

```text
/key bind meta-s /mute spell toggle
```

### Speed limit option for DCC files

The option xfer.network.speed_limit has been renamed to xfer.network.speed_limit_send.

If you changed the value of this option, you must set it again after upgrade.

A new option xfer.network.speed_limit_recv has been added to limit the
speed of received files.

### Allocated string in hook info and function info_get

The hook info callback now returns an allocated string, which must be freed
after use (in previous versions, a pointer to a static string was returned).

Consequently, the function info_get returns an allocated string, which must
be freed after use.

This affects only C code, no changes are required in scripts.

## Version 2.4

### Nick completer

A space is not added automatically anymore when you complete a nick at the
beginning of command line.\
Purpose of this change is to be more flexible: you can choose whether the space
is added or not (it was always added in previous releases).

The default value of option `weechat.completion.nick_completer` has been changed
to add the space by default, but the value of option is not changed automatically
on upgrade.

So you can run this command if you upgraded from an old version
and want the space still added automatically:

```text
/set weechat.completion.nick_completer ": "
```

### Base64 API functions

The functions to encode/decode base64 strings have been renamed and now support
base 16, 32, and 64.

New functions in C API, supporting base 16, 32, and 64:

- [string_base_encode](https://weechat.org/doc/weechat/plugin/#_string_base_encode)
- [string_base_decode](https://weechat.org/doc/weechat/plugin/#_string_base_decode)

Functions removed from C API:

- string_encode_base64
- string_decode_base64

## Version 2.2

### Default conditions for hotlist

The default value for option `weechat.look.hotlist_add_conditions` has been
changed to take care about the number of connected clients on the relay
with weechat protocol: if at least one client is connected, the buffer is
always added to the hotlist.

The new value contains three conditions, if one of them is true, the buffer
is added to the hotlist:

- `${away}`: true if you are away on the server matching the buffer,
- `${buffer.num_displayed} == 0`: true if the buffer is not displayed in any
  window
- `${info:relay_client_count,weechat,connected} > 0`: true if at least
  one client is connected on a weechat relay (new condition).

To use the new default value, you can reset the option with this command:

```text
/unset weechat.look.hotlist_add_conditions
```

Or set explicitly the value:

```text
/set weechat.look.hotlist_add_conditions "${away} || ${buffer.num_displayed} == 0 || ${info:relay_client_count,weechat,connected} > 0"
```

### Default triggers

The existing triggers "cmd_pass" and "msg_auth" have been updated to hide
password in command `/msg nickserv set password` and support the option
`-server <name>`.

You can restore the default triggers with the following command:

```text
/trigger restore cmd_pass msg_auth
```

### Tags in IRC "in" signals

The IRCv3 tags are now sent in these IRC signals for received messages
(`xxx` is IRC server name, `yyy` is IRC command name):

- `xxx,irc_in_yyy`
- `xxx,irc_in2_yyy`
- `xxx,irc_raw_in_yyy`
- `xxx,irc_raw_in2_yyy`

This could break plugins or scripts that parse IRC messages and don't expect
to receive tags (even if tags **are** part of the IRC message, so this was a bug
in the IRC signals).

See issue [#787](https://github.com/weechat/weechat/issues/787) for more information.

Note: it is recommended for plugins and scripts to use the WeeChat IRC parser:
see the function [info_get_hashtable](https://weechat.org/doc/weechat/plugin/#_info_get_hashtable)
in the Plugin API reference.

Now the whole IRC message is received by the signal callback, for example:

```text
@tag1=abc;tag2=def :nick!user@host PRIVMSG #test :this is a test
```

In older releases, this message was received:

```text
:nick!user@host PRIVMSG #test :this is a test
```

### New Debian package for headless version

A new Debian package has been added: "weechat-headless" which contains the
binary "weechat-headless" and its man page.

In version 2.1, this binary was in the package "weechat-curses".

## Version 2.1

### Completion for /set and /help commands

A new option weechat.completion.partial_completion_templates has been added to
force partial completion on some templates.  By default, the option name
completed in `/set` and `/help` commands are now using partial completion.

If you prefer old behavior, you can remove the templates from the new option
with this command:

```text
/set weechat.completion.partial_completion_templates ""
```

For more information about this feature, you can read help with:

```text
/help weechat.completion.partial_completion_templates
```

### Option to check license of scripts loaded

A configuration file has been added for each script plugin: python.conf,
perl.conf, ruby.conf, ...

Therefore the option to check license of loaded scripts has been moved from
`plugins.var.<language>.check_license` (type: string) to the plugin
configuration file (type: boolean, default is `off`).

List of options moved:

- plugins.var.python.check_license (string) → python.look.check_license (boolean)
- plugins.var.perl.check_license (string) → perl.look.check_license (boolean)
- plugins.var.ruby.check_license (string) → ruby.look.check_license (boolean)
- plugins.var.lua.check_license (string) → lua.look.check_license (boolean)
- plugins.var.tcl.check_license (string) → tcl.look.check_license (boolean)
- plugins.var.guile.check_license (string) → guile.look.check_license (boolean)
- plugins.var.javascript.check_license (string) → javascript.look.check_license (boolean)
- plugins.var.php.check_license (string) → php.look.check_license (boolean)

## Version 2.0

### Fset plugin

A new plugin "fset" has been added, it replaces the script iset.pl and has many
new features.

By default the fset plugin changes the behavior of `/set` command when it is
used with only an option name: it opens the fset buffer if at least one option
is found.

The old behavior was to display the list of options in the core buffer.

If you prefer the old behavior, you can restore it with this command:

```text
/set fset.look.condition_catch_set ""
```

For more information about this feature, you can read help with:

```text
/help fset.look.condition_catch_set
```

### Split of scripting Debian packages

The Debian packaging has changed (for now only on weechat.org repositories,
not in Debian official repositories).\
The package "weechat-plugins" has been split into 9 packages:

- weechat-plugins (with only the following plugins: aspell, exec, fifo, relay,
  script, trigger)
- weechat-python
- weechat-perl
- weechat-ruby
- weechat-lua
- weechat-tcl
- weechat-guile
- weechat-javascript
- weechat-php.

If you are using the packages from weechat.org, you will have to install
manually the scripting packages (according to the languages you'll use
in WeeChat), for example Python/Perl only:

```bash
sudo apt-get install weechat-python weechat-perl
```

For development packages:

```bash
sudo apt-get install weechat-devel-python weechat-devel-perl
```

### Evaluation in buflist

The evaluation of expressions in buflist options is not recursive anymore,
to prevent too many evaluations, for example in buffer variables
(see issue [#1060](https://github.com/weechat/weechat/issues/1060) for more information).\
If you are using custom variables/options containing evaluated expressions,
like `${some.config.option}`, and if this option contains evaluated strings (`${...}`),
you must evaluate them with: `${eval:${some.config.option}}`.

Note: the default buflist formats (`${format_buffer}`, `${format_name}`,
`${format_hotlist}`, ...) are automatically evaluated in options buflist.format.buffer,
buflist.format.buffer_current and buflist.format.hotlist.

### Function hook_connect

In scripts, the arguments "status", "gnutls_rc" and "sock" sent to the callback
of "hook_connect" are now integers (they were strings in older releases).\
To be compatible with all versions, it is recommended to convert the argument
to integer before using it, for example in Python: `int(sock)`.

### Function hook_fd

In scripts, the argument "fd" sent to the callback of "hook_fd" is now
an integer (it was a string in older releases).\
To be compatible with all versions, it is recommended to convert the argument
to integer before using it, for example in Python: `int(fd)`.

## Version 1.8

### Buflist plugin

A new plugin called "buflist" has been added, it replaces the script "buffers.pl".

If the script is installed, you will see two sidebars with list of buffers.

If you fell in love with buflist and that the script buffers.pl is installed,
you can uninstall the script and remove its bar with these commands:

```text
/script remove buffers.pl
/bar del buffers
```

If you don't want the list of buffers, you can disable buflist:

```text
/set buflist.look.enabled off
```

To save extra memory, you can even unload the buflist plugin, remove the bar and
prevent the plugin from loading on next startup:

```text
/plugin unload buflist
/bar del buflist
/set weechat.plugin.autoload "*,!buflist"
```

### Options

The option script.scripts.url_force_https has been removed because now the
site weechat.org can only be used with HTTPS.\
Both HTTP and HTTPS protocols are allowed in the option script.scripts.url.\
For `http://weechat.org/` an automatic redirection to `https://weechat.org/`
will occur, so you should check that the CA certificates are properly installed
on your machine.

Aspell option with color for suggestion on a misspelled word has been renamed:

- aspell.color.suggestions has been renamed to aspell.color.suggestion

## Version 1.7

### FIFO options

A new configuration file "fifo.conf" has been added and the old option
plugins.var.fifo.fifo has been moved to fifo.file.enabled.

A new option fifo.file.path can be used to customize the FIFO pipe
path/filename.

### Default triggers

A new trigger "cmd_pass_register" has been added to hide only password and not
email in command `/msg nickserv register password email`.\
The existing trigger "cmd_pass" has been updated to hide password in all
commands except `/msg nickserv register`.

You can add the new trigger and restore the other one with the following
command:

```text
/trigger restore cmd_pass_register cmd_pass
```

## Version 1.6

### IRC kick/part/quit default messages

Server options with default messages for kick/part/quit have been renamed:

- options by default for all servers:
  - irc.server_default.default_msg_kick → irc.server_default.msg_kick
  - irc.server_default.default_msg_part → irc.server_default.msg_part
  - irc.server_default.default_msg_quit → irc.server_default.msg_quit
- options in each server:
  - irc.server.xxx.default_msg_kick → irc.server.xxx.msg_kick
  - irc.server.xxx.default_msg_part → irc.server.xxx.msg_part
  - irc.server.xxx.default_msg_quit → irc.server.xxx.msg_quit

If you changed the value of these options, you must set them again after upgrade.

### Printf API functions

Some "printf" functions have been removed from C API (there were not in scripting API):

- printf_date
- printf_tags

The function "printf_date_tags" must now be used instead of these functions
(the two functions removed were just C macros on function "printf_date_tags"
with tags set to NULL for "printf_date" and date set to 0 for "printf_tags").

## Version 1.5

### Nick coloring moved to core

The nick coloring feature has been moved from irc plugin to core.

Two options have been moved from irc plugin (irc.conf) to core (weechat.conf),
and you must set new value if you customized them:

- irc.look.nick_color_force → weechat.look.nick_color_force
- irc.look.nick_color_hash → weechat.look.nick_color_hash
- irc.look.nick_color_stop_chars → weechat.look.nick_color_stop_chars

The default value for option weechat.look.nick_color_hash is now `djb2`
instead of `sum`.

The following info names (used by API function "info_get") are renamed as well:

- irc_nick_color → nick_color
- irc_nick_color_name → nick_color_name

Note: the old info irc_nick_color and irc_nick_color_name are kept for
compatibility (especially scripts) and will be removed in an upcoming release.

### Pointer in callbacks

A pointer has been added in all callbacks used by scripts, so the callbacks
will receive an extra `const void *pointer` before the `void *data`
(in the C API only, not scripting API).

This is used to remove linked list of callbacks in scrips (improve speed,
reduce memory usage).

Following functions are changed in the C API:

- [exec_on_files](https://weechat.org/doc/weechat/plugin/#_exec_on_files)
- [config_new](https://weechat.org/doc/weechat/plugin/#_config_new)
- [config_new_section](https://weechat.org/doc/weechat/plugin/#_config_new_section)
- [config_new_option](https://weechat.org/doc/weechat/plugin/#_config_new_option)
- [hook_command](https://weechat.org/doc/weechat/plugin/#_hook_command)
- [hook_command_run](https://weechat.org/doc/weechat/plugin/#_hook_command_run)
- [hook_timer](https://weechat.org/doc/weechat/plugin/#_hook_timer)
- [hook_fd](https://weechat.org/doc/weechat/plugin/#_hook_fd)
- [hook_process](https://weechat.org/doc/weechat/plugin/#_hook_process)
- [hook_process_hashtable](https://weechat.org/doc/weechat/plugin/#_hook_process_hashtable)
- [hook_connect](https://weechat.org/doc/weechat/plugin/#_hook_connect)
- [hook_print](https://weechat.org/doc/weechat/plugin/#_hook_print)
- [hook_signal](https://weechat.org/doc/weechat/plugin/#_hook_signal)
- [hook_hsignal](https://weechat.org/doc/weechat/plugin/#_hook_hsignal)
- [hook_config](https://weechat.org/doc/weechat/plugin/#_hook_config)
- [hook_completion](https://weechat.org/doc/weechat/plugin/#_hook_completion)
- [hook_modifier](https://weechat.org/doc/weechat/plugin/#_hook_modifier)
- [hook_info](https://weechat.org/doc/weechat/plugin/#_hook_info)
- [hook_info_hashtable](https://weechat.org/doc/weechat/plugin/#_hook_info_hashtable)
- [hook_infolist](https://weechat.org/doc/weechat/plugin/#_hook_infolist)
- [hook_hdata](https://weechat.org/doc/weechat/plugin/#_hook_hdata)
- [hook_focus](https://weechat.org/doc/weechat/plugin/#_hook_focus)
- [unhook_all](https://weechat.org/doc/weechat/plugin/#_unhook_all)
- [buffer_new](https://weechat.org/doc/weechat/plugin/#_buffer_new)
- [bar_item_new](https://weechat.org/doc/weechat/plugin/#_bar_item_new)
- [upgrade_new](https://weechat.org/doc/weechat/plugin/#_upgrade_new)
- [upgrade_read](https://weechat.org/doc/weechat/plugin/#_upgrade_read)

The function `unhook_all` has a new argument `const char *subplugin` to remove
only hooks created by this "subplugin" (script).

### Bar item "away"

The bar item "away" has been moved from irc plugin to core (so that away status
can be displayed for any plugin, using the buffer local variable "away").

Two options have been moved from irc plugin (irc.conf) to core (weechat.conf):

- irc.look.item_away_message → weechat.look.item_away_message
- irc.color.item_away → weechat.color.item_away

### Default triggers

The default triggers "cmd_pass" and "msg_auth" have been updated to include
nickserv commands "recover" and "regain".

You can restore them with the following command:

```text
/trigger restore cmd_pass msg_auth
```

## Version 1.4

### IRC alternate nicks

The option irc.network.alternate_nick has been moved into servers
(irc.server_default.nicks_alternate and irc.server.xxx.nicks_alternate).

If you disabled this option, you must switch it off again, globally or by
server.

Globally (default value for all servers):

```text
/set irc.server_default.nicks_alternate off
```

For a specific server:

```text
/set irc.server.libera.nicks_alternate off
```

## Version 1.3

### IRC channels encoding

If you are using exotic charsets in your channel names (anything different from
UTF-8, like ISO charset), you should turn on a new option:

```text
/set irc.network.channel_encode on
```

This will force WeeChat to decode/encode the channel name
(like WeeChat 1.2 or older did).

See these issues for more information: [#482](https://github.com/weechat/weechat/issues/482)
and [#218](https://github.com/weechat/weechat/issues/218).

Note: it is **highly recommended** to use only UTF-8 in WeeChat (wherever you can),
because everything is stored as UTF-8 internally.

### Alias command

The command `/alias` has been updated to list, add and remove aliases.
Therefore the command `/unalias` has been removed.

To add an alias, the argument `add` must be used in command `/alias` before the
name, for example:

```text
/alias add split /window splith
```

And the alias is removed with this command:

```text
/alias del split
```

### Script path

The option script.scripts.dir has been renamed to script.scripts.path
(and the content is now evaluated, see `/help eval`).

If you changed the value of this option, you must set it again after upgrade.

## Version 1.2

The word chars are now customizable with two options:

- weechat.look.word_chars_highlight
- weechat.look.word_chars_input

The behavior has changed for command line: now any non-word char is used as
delimiter for keys to move to previous/next word or delete previous/next word.

You can restore the old behavior (only use spaces as delimiters) with this
command:

```text
/set weechat.look.word_chars_input "!\u00A0,!\x20,*"
```

## Version 1.1

### New format for regex replacement in triggers

A new format is used in regex replacement to use regex groups, this format
is incompatible with version 1.0.

The existing triggers are **NOT automatically updated**.

Old format     | New format               | Examples (new format)
-------------- | ------------------------ | ----------------------------------------
`$0` ... `$99` | `${re:0}` ... `${re:99}` | `${re:1}`
`$+`           | `${re:+}`                | `${re:+}`
`$.*N`         | `${hide:*,${re:N}}`      | `${hide:*,${re:2}}`, `${hide:-,${re:+}}`

Moreover, default triggers used to hide passwords have been fixed for BSD operating systems.

You can restore them with the following command:

```text
/trigger restore cmd_pass msg_auth server_pass
```

If you added triggers with the old regex replacement format, you must update
them manually.

### Default "beep" trigger

The command of "beep" trigger is now executed only if the message is displayed
(not filtered with `/filter`).

You can restore the default "beep" trigger with the following command:

```text
/trigger restore beep
```

### Return code of commands

The API function [command](https://weechat.org/doc/weechat/plugin/#_command)
now sends the value returned return by command callback.

WeeChat does not display anymore an error when a command returns
`WEECHAT_RC_ERROR`. Consequently, all plugins/scripts should display an
explicit error message before returning `WEECHAT_RC_ERROR`.

For C plugins, two macros have been added in weechat-plugin.h:
`WEECHAT_COMMAND_MIN_ARGS` and `WEECHAT_COMMAND_ERROR`.

### Completion of inline commands

WeeChat now completes by default inline commands (not only at beginning of
line).

When this feature is enabled, there is no more automatic completion of
absolute paths (except if you are completing a path inside a command argument,
like `/dcc send <nick> <path>`).

To restore the old behavior (no completion of inline commands):

```text
/set weechat.completion.command_inline off
```

### Relay option relay.irc.backlog_tags

The option relay.irc.backlog_tags is now a list separated by commas
(it was separated by semicolons in older versions).

If you are using a list of tags in this option, you must adjust the value
manually.

### IPv4-mapped IPv6 client address in relay

The string "::ffff:" has been removed from IPv4-mapped IPv6 client address
in relay plugin.

If you are using "::ffff:" in option relay.network.allowed_ips, you can
remove it.

### Temporary servers disabled by default with /connect

Creating a temporary server with command `+/connect <address>+` or
`+/connect irc://...+` is now forbidden by default.

A new option has been added to unlock the feature, you can do that for
the old behavior in command `/connect`:

```text
/set irc.look.temporary_servers on
```

### Microseconds in API timeval functions

The API functions using timeval are now using or returning microseconds,
instead of milliseconds:

- function [util_timeval_diff](https://weechat.org/doc/weechat/plugin/#_util_timeval_diff):
  returns microseconds
- function [util_timeval_add](https://weechat.org/doc/weechat/plugin/#_util_timeval_add):
  the argument "interval" is now expressed in microseconds.

## Version 1.0

### Channel type not added by default on /join

The channel type is not anymore automatically added to a channel name on join
(for example `/join weechat` will not send `/join #weechat`).

If you are lazy and want to automatically add the channel type, you can turn on
the new option:

```text
/set irc.look.join_auto_add_chantype on
```

### Hide IRC channel modes arguments

The option irc.look.item_channel_modes_hide_key has been renamed to
irc.look.item_channel_modes_hide_args and is now a string.\
It can now hide arguments for multiple channel modes.

By default, a channel key (mode "k") will hide channel arguments. For old
behavior (never hide arguments, even with a channel key), you can do:

```text
/set irc.look.item_channel_modes_hide_args ""
```

### Jump to first/last buffer

The command `/input jump_last_buffer` has been replaced by `/buffer +`.
You can rebind the key `Alt`+`j`, `Alt`+`l` (`L`):

```text
/key bind meta-jmeta-l /buffer +
```

Note: the command `/input jump_last_buffer` still works for compatibility reasons,
but it should not be used anymore.

Similarly, a new key has been added to jump to first buffer: `Alt`+`j`, `Alt`+`f`.
You can add it with the following command:

```text
/key missing
```

### Hotlist conditions

A new option weechat.look.hotlist_add_conditions has been added. This option
replaces the option weechat.look.hotlist_add_buffer_if_away, which has been
removed.

Default conditions are `${away} || ${buffer.num_displayed} == 0`, which means
that a buffer is added in hotlist if you are away ("away" local variable is
set), or if the buffer is not visible on screen (not displayed in any window).

If you have set weechat.look.hotlist_add_buffer_if_away to `off` (to not add
current buffer in hotlist when you are away), then you must manually change the
default conditions with the following command:

```text
/set weechat.look.hotlist_add_conditions "${buffer.num_displayed} == 0"
```

### Rmodifier replaced by Trigger plugin

The trigger plugin replaces the rmodifier plugin, which has been removed
(and trigger has much more features than rmodifier).

Default triggers have same features as default rmodifiers (to hide passwords
in commands and output).

If you added some custom rmodifiers, you must create similar triggers, see
`/help trigger` and the complete trigger doc in the _WeeChat User's guide_.

Note: if on startup you have an error about API mismatch in plugin "rmodifier.so",
you can manually remove the file (the command `make install` does not remove
obsolete plugins).

### Bare display

A bare display mode has been added (for easy text selection and click on URLs),
the new default key is `Alt`+`l` (`L`).

Use command `/key missing` to add the key or `/key listdiff` to see differences
between your current keys and WeeChat default keys.

### Function hook_print

In scripts, the arguments "displayed" and "highlight" sent to the callback of
"hook_print" are now integers (they were strings in older releases).

To be compatible with all versions, it is recommended to convert the argument
to integer before testing it, for example in Python:

```python
if int(highlight):
    # ...
```

## Version 0.4.3

### Colors in messages

The color code for "reverse video" in IRC message has been fixed: now WeeChat
uses 0x16 like other clients (and not 0x12 anymore).\
The code 0x12 is not decoded anymore, so if it is received (for example from
an old WeeChat version), it is not displayed as reverse video.

The color code for "underlined text" in input line has been fixed: now WeeChat
uses 0x1F, the same code sent to IRC server.

The default keys for "reverse video" and "underlined text" have changed:

- reverse video: key `Ctrl`+`c`, `r` is replaced by `Ctrl`+`c`, `v`
- underlined text: key `Ctrl`+`c`, `u` is replaced by `Ctrl`+`c`, `_`

You can remove the old keys and add the new ones with these commands:

```text
/key unbind ctrl-Cr
/key unbind ctrl-Cu
/key missing
```

### Terminal title

The boolean option weechat.look.set_title has been renamed to
weechat.look.window_title and is now a string.

The content is evaluated and the default value is `WeeChat ${info:version}`.

Note: only static content should be used in this option, because the title is
refreshed only when the option is changed.

### New bar item buffer_last_number

The bar item "buffer_count" now displays the number of opened buffers (each
merged buffer counts 1).

The new bar item "buffer_last_number" displays the highest buffer number
currently used.

If you want to display last number in the status bar, replace "buffer_count"
by "buffer_last_number" in your option weechat.bar.status.items.

### New bar item buffer_zoom

A new bar item has been added: "buffer_zoom".
The default value for status bar items becomes:

```text
/set weechat.bar.status.items "[time],[buffer_count],[buffer_plugin],buffer_number+:+buffer_name+(buffer_modes)+{buffer_nicklist_count}+buffer_zoom+buffer_filter,[lag],[hotlist],completion,scroll"
```

### IRC messages on channel join

The names are not displayed anymore by default on channel join (they are in
nicklist anyway).

Names can be displayed with the value "353" in option
irc.look.display_join_message (which is not in default value).\
The value "366" shows only names count on channel.

If you want to display all messages on join (including names), you can do:

```text
/set irc.look.display_join_message "329,332,333,353,366"
```

### Maximum lag in IRC

Option irc.network.lag_max has been added.

The value of option irc.network.lag_reconnect (if set to non-zero value) must
be less than or equal to irc.network.lag_max, otherwise the reconnection will
never occur.

You should check the value of both options and fix them if needed.

## Version 0.4.2

### Day change message

The day change message is now dynamically displayed, and therefore is not stored
as a line in buffer anymore.

Option weechat.look.day_change_time_format has been split into two options
weechat.look.day_change_message_{1date|2dates} (color codes are allowed in
these options, see `/help eval`).

New color option weechat.color.chat_day_change has been added.

After `/upgrade` from an old version, you will see two messages for each day
change. This is a normal behavior and will not happen again with the next day
changes.

### Regex search in buffer

Search with regular expression has been added, and therefore some keys in search
context have been changed.

The key `Ctrl`+`r` in search context is now used to switch string/regex search
(instead of searching exact text).

If you never changed keys in search context, you can reset them all with one
command:

```text
/key resetall -yes search
```

Or the manual method:

```text
/key missing search
/key bindctxt search ctrl-R /input search_switch_regex
```

To view keys in search context:

```text
/key list search
```

For more information, see the chapter about keys in the _WeeChat User's guide_.

### New rmodifier

A new rmodifier "secure" has been added to hide passphrase and passwords
displayed by command `/secure`. Use command `/rmodifier missing` to add it.

### Color codes in options

The format for color codes in some options has changed. The options are
evaluated with the function "string_eval_expression", which uses the format
`${color:xxx}`.

Following options are affected:

- weechat.look.buffer_time_format
- weechat.look.prefix_action
- weechat.look.prefix_error
- weechat.look.prefix_join
- weechat.look.prefix_network
- weechat.look.prefix_quit

The options using old format `${xxx}` must be changed with new format
`${color:xxx}` (where xxx is a color name or number, with optional color
attributes).

Example:

```text
/set weechat.look.buffer_time_format "${color:251}%H${color:243}%M${color:238}%S"
```

### Binary and man page

WeeChat binary and man page have been renamed from `weechat-curses` to
`weechat`.

A symbolic link has been added for binary: `weechat-curses` → `weechat`
(so that the `/upgrade` from a old version will still work).

If you upgrade from an old version, it is recommended to force the use of the
new binary name with the command: `/upgrade /path/to/weechat` (replace the path
accordingly).

Note: for packagers: you should create the link `weechat-curses` → `weechat`
if it's not automatically created in the package (both CMake and configure are
creating this link on make install).

### Man page / documentation

Documentation is not built by default anymore, you have to use option
`-DENABLE_DOC=ON` in cmake command to enable it.

The man page is now built with asciidoc and translated in several
languages. A new CMake option `ENABLE_MAN` has been added to compile man page
(`OFF` by default).

### Aspell colors

Option aspell.look.color has been renamed to aspell.color.misspelled.

If you changed the value of this option, you must set it again after upgrade.

## Version 0.4.1

### Nicklist diff in relay

A new message with identifier "_nicklist_diff" has been added in relay (WeeChat
protocol). WeeChat may decide to send full nicklist or this nicklist diff at
any time (depending on size of message, the smaller is sent).

Clients using nicklist must implement it.

For more info about content of message, see document _WeeChat Relay Protocol_.

### Dynamic nick prefix/suffix

The nick prefix/suffix (for example: "<" and ">") are now dynamic and used on
display (not stored anymore in the line).

Options moved from irc plugin (irc.conf) to core (weechat.conf):

- irc.look.nick_prefix → weechat.look.nick_prefix
- irc.look.nick_suffix → weechat.look.nick_suffix
- irc.color.nick_prefix → weechat.color.chat_nick_prefix
- irc.color.nick_suffix → weechat.color.chat_nick_suffix

Types and default values for these four options remain unchanged.

Two new options to customize the truncature char (by default "`+`"):

- weechat.look.prefix_align_more_after (boolean, `on` by default)
- weechat.look.prefix_buffer_align_more_after (boolean, `on` by default)

When these options are enabled (default), the "`+`" is displayed after the
text, replacing the space that should be displayed there.\
When turned off, the "`+`" will replace last char of text.

Example for a nicks "FlashCode" and "fc" with different values for options
weechat.look.prefix_align_max, weechat.look.prefix_align_more_after,
weechat.look.nick_prefix and weechat.look.nick_suffix:

```text
                      # align_max, more_after, prefix/suffix

FlashCode │ test      # 0, on
       fc │ test

FlashCod+│ test       # 8, on
      fc │ test

FlashCo+ │ test       # 8, off
      fc │ test

<FlashCode> │ test    # 0, on,  < >
       <fc> │ test

<FlashC>+│ test       # 8, on,  < >
    <fc> │ test

<Flash+> │ test       # 8, off, < >
    <fc> │ test
```

After `/upgrade`, if you set new options to non-empty strings, and if old
options were set to non-empty strings too, you will see double prefix/suffix
on old messages, this is normal behavior (lines displayed before `/upgrade`
have prefix/suffix saved in prefix, but new lines don't have them anymore).

New options in logger plugin (logger.conf):

- logger.file.nick_prefix: prefix for nicks in log files (default: empty string)
- logger.file.nick_suffix: suffix for nicks in log files (default: empty string)

### IRC reconnection on important lag

Option irc.network.lag_disconnect has been renamed to irc.network.lag_reconnect
and value is now a number of seconds (instead of minutes).

If you changed the value of this option, you must set it again after upgrade.

### IRC passwords hidden

IRC plugin is now using modifiers "irc_command_auth" and "irc_message_auth" to
hide passwords.

The option irc.look.hide_nickserv_pwd has been removed, and a new option
irc.look.nicks_hide_password has been added (by default passwords are hidden
only for "nickserv").

A new rmodifier "message_auth" has been added to hide passwords displayed by
command `/msg nickserv identify|register|ghost|release` and the rmodifier
"nickserv" has been renamed to "command_auth".

If you never added/changed rmodifiers, you can just reset all rmodifiers:

```text
/rmodifier default -yes
```

If you added/changed some rmodifiers, do it manually with these commands:

```text
/rmodifier del nickserv
/rmodifier add command_auth history_add,input_text_display,irc_command_auth 1,4* ^(/(msg|quote) +nickserv +(id|identify|register|ghost \S+|release \S+) +)(.*)
/rmodifier add message_auth irc_message_auth 1,3* ^(.*(id|identify|register|ghost \S+|release \S+) +)(.*)
```

### Lua constants

For consistency with other supported languages, the API constants in Lua have
been redefined as constants instead of functions.

Therefore, the use of a constant must be changed: the parentheses must be
removed.

The old syntax was:

```lua
return weechat.WEECHAT_RC_OK()
```

The new syntax is:

```lua
return weechat.WEECHAT_RC_OK
```

### Guile callbacks

The way to give arguments for guile callbacks has been fixed: now arguments are
sent individually (instead of a list with all arguments inside).

Therefore, existing guile scripts must be modified accordingly. Moreover,
WeeChat now requires Guile ≥ 2.0 to compile.

## Version 0.4.0

### Conditions in bars

Conditions in bars have changed, and now an expression is evaluated.

If you have a value with many conditions in a bar, like: `nicklist,active`, you
must now use an expression like: `${nicklist} && ${active}` (see the chapter
about bars in the _WeeChat User's guide_).

### IPv6 by default

#### IRC

IPv6 is now used by default to connect to IRC servers, with fallback to
IPv4. The option irc.server_default.ipv6 is now `on` by default. If IPv6 is
not enabled or fails, IPv4 will be used. The "ipv6" option in server is now
used to disable IPv6 and force IPv4 (if option is turned off).

#### Relay

Relay plugin is now listening by default on an IPv6 socket (new option
relay.network.ipv6, `on` by default), so connections with IPv4 will have
IPv4-mapped IPv6 addresses, like: "::ffff:127.0.0.1" (for "127.0.0.1"); check
that value of option relay.network.allowed_ips supports this mapping, or
disable IPv6 in relay if you don't plan to use it at all:

```text
/set relay.network.ipv6 off
```

## Version 0.3.9

### Options moved

Options moved from core (weechat.conf) to irc plugin (irc.conf):

- weechat.look.nickmode → irc.look.nick_mode (new type: integer with values: none/prefix/action/both)
- weechat.look.nickmode_empty → irc.look.nick_mode_empty

### New bar item buffer_modes

A new bar item has been added: "buffer_modes" and irc option irc.look.item_channel_modes
has been removed; to display irc channel modes in status bar (after channel name),
you have to manually add the new item "buffer_modes" (this is now used by default
in status bar items), default value for status bar items becomes:

```text
/set weechat.bar.status.items "[time],[buffer_count],[buffer_plugin],buffer_number+:+buffer_name+(buffer_modes)+{buffer_nicklist_count}+buffer_filter,[lag],[hotlist],completion,scroll"
```

### Command /aspell

New options in command `/aspell`:

- `enable`: enable aspell
- `disable`: disable aspell
- `toggle`: toggle aspell (new default key: `Alt`+`s`)

Options renamed in command `/aspell`:

- `enable` → `setdict` (set dictionary for current buffer)
- `disable` → `deldict` (delete dictionary used on current buffer)
- `dictlist` → `listdict` (show installed dictionaries)

### Horizontal separator

An horizontal separator has been added between split windows, and two options
have been added to toggle separators (both are enabled by default):

- weechat.look.window_separator_horizontal
- weechat.look.window_separator_vertical

### New keys

New keys were added, use command `/key missing` to add them or `/key listdiff`
to see differences between your current keys and WeeChat default keys.

## Version 0.3.8

### Options

Options weechat.look.prefix_align_more and weechat.look.prefix_buffer_align_more
have been converted from type boolean to string:

- If the value was `on` (default), new value is "+" and you can now customize
  this char.
- If the value was `off`, you have to set " " (string with one space).

### Paste detection

Option weechat.look.paste_max_lines can now be used with value 0 to detect
paste with one line (only if terminal "bracketed paste mode" is enabled when
option weechat.look.paste_bracketed is `on`); so now the value -1 is used to
disable paste detection: if your value was 0, you should set it to -1:

```text
/set weechat.look.paste_max_lines -1
```

### Rmodifier

Rmodifier "nickserv" has a new default regex which includes option "release"
for command `/msg nickserv`.

If you never added/changed rmodifiers, you can just reset all rmodifiers:

```text
/rmodifier default -yes
```

If you added/changed some rmodifiers, do it manually with these commands:

```text
/rmodifier del nickserv
/rmodifier add nickserv history_add,input_text_display 1,4* ^(/(msg|quote) +nickserv +(id|identify|ghost \S+|release \S+) +)(.*)
```

## Version 0.3.7

### Options

Option `scroll_unread` has been moved from command `/input` to `/window`,
therefore default command of key `Alt`+`u` has been updated. To bind key with
new default value:

```text
/key bind meta-u /window scroll_unread
```

Option weechat.history.max_lines has been renamed to weechat.history.max_buffer_lines_number.

If you changed the value of this option, you must set it again after upgrade.

Option weechat.plugin.extension now supports list of extensions, and new
default value is ".so,.dll" (with this value, weechat.conf is compatible with
Cygwin).

### Extended regex

Extended regex is used in filters and irc ignore, so some chars that needed
escape in past do not need anymore (for example `[0-9]\+` becomes `[0-9]+`),
filters and ignore have to be manually fixed.

Option weechat.look.highlight_regex becomes case-insensitive by default, to
make it case-sensitive, use "(?-i)" at beginning of string, for example:
"(?-i)FlashCode|flashy".

## Version 0.3.6

### Options

Option weechat.look.hline_char has been renamed to weechat.look.separator_horizontal.

If you changed the value of this option, you must set it again after upgrade.

### Bold in colors

Bold is not used anymore for basic colors (used only if terminal has less than
16 colors), a new option has been added to force bold if needed:
weechat.look.color_basic_force_bold.

## Version 0.3.5

### Colors

If you have some colors defined in section "palette" with version 0.3.4, you
should remove all colors defined, and add new aliases (it's not needed anymore
to add colors before using them).

Colors for nick prefixes (char for op, voice, ..) are defined in a single
option irc.color.nick_prefixes, therefore following options will be lost:
irc.color.nick_prefix_op, irc.color.nick_prefix_halfop,
irc.color.nick_prefix_voice, irc.color.nick_prefix_user .

### Hotlist

#### Counters

Count of messages have been added to hotlist by default, if you want to come
back to old behavior, do that:

```text
/set weechat.look.hotlist_count_max 0
/set weechat.look.hotlist_buffer_separator ","
```

#### Away and current buffer

When you are away, all buffers are now added to hotlist by default (even if
they are displayed in a window), if you want to come back to old behavior, do
that:

```text
/set weechat.look.hotlist_add_buffer_if_away off
```

### New keys

New keys were added, use command `/key missing` to add them or `/key listdiff`
to see differences between your current keys and WeeChat default keys.

## Version 0.3.4

### After /upgrade

If you are using `/upgrade` from a previous release:

- Some nick prefixes can be wrong, so it is recommended to do `/allchan names`.
- Nick colors are defined with a new option weechat.color.chat_nick_colors ,
  therefore old options weechat.color.chat_nick_color1..10 will be lost when
  upgrading.
- Nick colors in messages displayed will be wrong if you changed some nick
  colors (old default colors will be used).

### Options

Some IRC options have been renamed, before upgrading to this version, note
value for old options, and set them again with new name:

- options moved from "network" section to servers (with global value, and
  server value, like other server options):
  - irc.network.connection_timeout → irc.server_default.connection_timeout
  - irc.network.anti_flood_prio_high → irc.server_default.anti_flood_prio_high
  - irc.network.anti_flood_prio_low → irc.server_default.anti_flood_prio_low
  - irc.network.away_check → irc.server_default.away_check
  - irc.network.away_check_max_nicks → irc.server_default.away_check_max_nicks
  - irc.network.default_msg_part → irc.server_default.default_msg_part
  - irc.network.default_msg_quit → irc.server_default.default_msg_quit
- other IRC options renamed:
  - irc.look.open_channel_near_server → irc.look.new_channel_position
    (old option was boolean, new is integer with value as string)
  - irc.look.open_pv_near_server → irc.look.new_pv_position
    (old option was boolean, new is integer with value as string)

## Version 0.3.3

### After /upgrade

If you are using `/upgrade` from a previous release, then you must reconnect to
IRC servers in order to use new command `/wallchops`.

### Options

Option irc.look.show_away_once has been renamed to irc.look.display_pv_away_once.

Option irc.network.lag_min_show is now in milliseconds, you should set new
value: your current value multiplied by 1000 (new default value is `500`).

## Version 0.3.2

### After /upgrade

If you are using `/upgrade` from a previous release, then you must execute this
command on all IRC servers/channels/private buffers and xfer DCC chats (not
needed on WeeChat core buffer or buffers from other plugins/scripts):

```text
/buffer set highlight_words $nick
```

## Version 0.3.1.1

This version fixes crashes with SSL connection and purge of old DCC chats.

All users of version 0.3.1 should upgrade to this version.

## Version 0.3.1

### Aliases

IRC commands `/ame` and `/amsg` are now aliases, if you are upgrading from version
0.3.0, you must create aliases with the following commands:

```text
/alias aaway allserv /away
/alias ame allchan /me
/alias amsg allchan /amsg *
/alias anick allserv /nick
```

## Version 0.3.0

This version brings **MAJOR** changes, especially for configuration files and
plugin API and is not compatible with previous versions.

Major differences:

- It is **NOT POSSIBLE** to use command `/upgrade` from a version 0.2.x to 0.3.x;
  you have to quit your old WeeChat, then run new version.
- New configuration files (`*.conf`) are not compatible with old files
  (`*.rc`).
- Name of options is similar to old versions, but there is now one
  configuration file by plugin, and one file for WeeChat core; there is
  **no automatic conversion** for your old options to new configuration files,
  so you'll have to setup again your IRC servers and all other options.
- Plugin API has been rewritten and is not compatible with previous versions;
  accordingly, scripts and plugins must have been designed for version 0.3.x to
  be loaded into WeeChat.

## Version 0.2.2

### Charset plugin

For users of any previous version, all your charset settings in weechat.rc will
be LOST! You should save your weechat.rc to keep your values and set them again
with new _charset_ plugin.

For ISO users: history of channels may be without accents (after `/upgrade`),
this is not recoverable, but this is not a bug. All new messages should be OK.

Be careful, now default encode is UTF-8 for all channels (before it was
terminal charset). If you still want to send messages as "ISO-8859-1", you
should set either global encode or server specific encode to `ISO-8859-1`.

For global encode:

```text
/setp charset.global.encode = "ISO-8859-1"
```

For server encode (on server buffer):

```text
/charset encode ISO-8859-1
```

### New keys

New keys for topic scroll: `F9` / `F10`.

Key `F10` was used for `infobar_clear` in previous WeeChat versions, you
have to manually rebind this key (except for new WeeChat users):

```text
/key <press alt+"k" then F10> scroll_topic_right
```

Which gives something like:

```text
/key meta2-21~ scroll_topic_right
```

## Version 0.2.0

### After /upgrade

If you upgraded with `/upgrade` in WeeChat, you should `/disconnect` and then
`/reconnect` on each server, to display properly channel/user modes.

### Plugins

If you're using plugins, you should remove some old plugins libraries in
WeeChat system library directory (commonly `/usr/local/lib/weechat/plugins`):
remove `lib*` files (like `libperl.*`, `libpython.*`, ..) and keep only new
libraries (`perl.*`, `python.*`, ..).

## Version 0.1.9

### DCC chat

Please close all DCC chat buffers before using `/upgrade` command, otherwise you
may experience problems with DCC chats.

### Script API

Some changes in script API: now timer handlers functions takes exactly 0 (zero)
argument (in version 0.1.8, two arguments were mandatory but not used: server
and arguments).

## Version 0.1.8

### After /upgrade

After installing 0.1.8 (or with `/upgrade`), issue both commands (if you didn't
redefine these keys (`Alt`+`Home` / `Alt`+`End`):

```text
/key unbind meta-meta2-1~
/key unbind meta-meta2-4~
```

Then launch again WeeChat (or issue `/upgrade`).

### Configuration files

It is recommended for users of version 0.1.7 (or any older), to replace values
in setup file (_~/.weechat/weechat.rc_):

- option: log_path: replace `~/.weechat/logs` by `%h/logs`
- option: plugins_path: replace `~/.weechat/plugins` by `%h/plugins`

The string `%h` is replaced by WeeChat home (default: `~/.weechat`, may be
overridden by new command line argument `--dir`).

### Keys

Keys `Alt`+`Home` / `Alt`+`End` were used for nicklist scroll, they're now
replaced by `Alt`+`F11` / `Alt`+`F12`.

## Version 0.1.7

### Ruby

Ruby script plugin has been added but is experimental in this release. You're
warned!

### Command /away

Command `/away` was changed to be RFC 2812 compliant. Now argument is required
to set away, and no argument means remove away ("back").

Option irc_default_msg_away has been removed.

## Version 0.1.6

### Script API

Incompatibility with some old scripts: now all handlers have to return a code
for completion, and to do some actions about message to ignore (please look at
documentation for detail).

### OpenBSD

On OpenBSD, the new option plugins_extension should be set to `.so.0.0` since
the plugins names are ending by `.so.0.0` and not `.so`.

### UTF-8

With new and full UTF-8 support, the option look_charset_internal should be
set to blank for most cases. Forces it only if your locale is not properly
detected by WeeChat (you can set `UTF-8` or `ISO-8859-15` for example,
depending on your locale). WeeChat is looking for "UTF-8" in your locale name
at startup.

<!--
SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# WeeChat

<p align="center">
  <img src="https://weechat.org/media/images/weechat_logo_large.png" alt="WeeChat" />
</p>

[![Mastodon](https://img.shields.io/badge/mastodon-follow-blue.svg)](https://hostux.social/@weechat)
[![X](https://img.shields.io/badge/x-follow-blue.svg)](https://x.com/WeeChatClient)
[![Devel blog](https://img.shields.io/badge/devel%20blog-follow-blue.svg)](https://blog.weechat.org/)
[![Donate](https://img.shields.io/badge/help-donate%20%E2%9D%A4-ff69b4.svg)](https://weechat.org/donate/)

[![CI](https://github.com/weechat/weechat/workflows/CI/badge.svg)](https://github.com/weechat/weechat/actions)
[![Code coverage](https://codecov.io/gh/weechat/weechat/branch/main/graph/badge.svg)](https://codecov.io/gh/weechat/weechat)
[![REUSE status](https://api.reuse.software/badge/github.com/weechat/weechat)](https://api.reuse.software/info/github.com/weechat/weechat)
[![Packaging status](https://repology.org/badge/tiny-repos/weechat.svg)](https://repology.org/project/weechat/versions)

**WeeChat** (Wee Enhanced Environment for Chat) is a free chat client, fast and light, designed for many operating systems.\
It is highly customizable and extensible with scripts.

Homepage: [https://weechat.org/](https://weechat.org/)

## Features

- **Modular chat client**: WeeChat has a lightweight core and optional [plugins](https://weechat.org/doc/weechat/user/#plugins). All plugins (including [IRC](https://weechat.org/doc/weechat/user/#irc)) are independent and can be unloaded.
- **Multi-platform**: WeeChat runs on GNU/Linux, *BSD, GNU/Hurd, Haiku, macOS and Windows (WSL and Cygwin).
- **Multi-protocol**: WeeChat is designed to support multiple protocols via plugins, like IRC.
- **Standards-compliant**: the IRC plugin is compliant with RFCs [1459](https://datatracker.ietf.org/doc/html/rfc1459), [2810](https://datatracker.ietf.org/doc/html/rfc2810), [2811](https://datatracker.ietf.org/doc/html/rfc2811), [2812](https://datatracker.ietf.org/doc/html/rfc2812), [2813](https://datatracker.ietf.org/doc/html/rfc2813) and [7194](https://datatracker.ietf.org/doc/html/rfc7194).
- **Small, fast, and very light**: the core is and should stay as light and fast as possible.
- **Customizable and extensible**: there are a lot of options to customize WeeChat, and it is extensible with C plugins and [scripts](https://weechat.org/scripts/) ([Perl](https://weechat.org/scripts/language/perl/), [Python](https://weechat.org/scripts/language/python/), [Ruby](https://weechat.org/scripts/language/ruby/), [Lua](https://weechat.org/scripts/language/lua/), [Tcl](https://weechat.org/scripts/language/tcl/), [Scheme](https://weechat.org/scripts/language/guile/), [JavaScript](https://weechat.org/scripts/language/javascript/) and [PHP](https://weechat.org/scripts/language/php/)).
- **Remote access**: WeeChat can run in background (`weechat-headless`) and, with the [relay plugin](https://weechat.org/doc/weechat/user/#relay), be used from [remote interfaces](https://weechat.org/about/interfaces/) (including WeeChat itself) or act as an IRC proxy.
- **Fully documented**: there is comprehensive [documentation](https://weechat.org/doc/weechat/), which is [translated](https://weechat.org/doc/weechat/dev/#translations) into several languages.
- **Developed from scratch**: WeeChat was built from scratch and is not based on any other client.
- **Free software**: WeeChat is released under [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html).

<p align="center">
  <img src="https://weechat.org/media/images/screenshots/weechat/medium/weechat_2013-04-27_phlux_shadow.png" alt="WeeChat" />
</p>

On WeeChat's website you can find [more screenshots](https://weechat.org/about/screenshots/).

## Installation

WeeChat can be installed using your favorite package manager (recommended) or by compiling it yourself.\
For detailed instructions, please check the [WeeChat user's guide](https://weechat.org/doc/weechat/user/#install).

### Build from source

The main dependencies are a C compiler, CMake, pkg-config, ncurses, libcurl, libgcrypt, GnuTLS and zlib; many others are optional (zstd, gettext, scripting languages, etc.).\
See the [full list of dependencies](https://weechat.org/doc/weechat/user/#dependencies).

To build and install WeeChat in your home directory:

```bash
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make
make install
```

CMake options can be given with `-DOPTION=VALUE`, for example:

- `-DCMAKE_BUILD_TYPE=Debug`: build with debug info (recommended if you are using a development version)
- `-DENABLE_PHP=OFF`: disable a plugin (here the PHP plugin)
- `-DENABLE_DOC=ON -DENABLE_MAN=ON`: build HTML documentation and man page
- `-DENABLE_TESTS=ON`: build tests (run them with `ctest -V`)

See the [list of CMake options](https://weechat.org/doc/weechat/user/#build).

## First steps

Start WeeChat with the command `weechat`, then add an IRC server, connect to it and join a channel:

```text
/server add libera irc.libera.chat
/connect libera
/join #weechat
```

Help is available in WeeChat with `/help` (list of commands) and `/help <command>` or `/help <option>`, and options can be browsed and changed with `/fset`.

For more information, see the [quick start guide](https://weechat.org/doc/weechat/quickstart/).

## Documentation

| Document                                                                 | Description                                               |
|--------------------------------------------------------------------------|-----------------------------------------------------------|
| [Quick start guide](https://weechat.org/doc/weechat/quickstart/)         | First steps with WeeChat.                                 |
| [User's guide](https://weechat.org/doc/weechat/user/)                    | Installation, usage, commands and options.                |
| [Cheat sheet](https://weechat.org/doc/weechat/devel/cheatsheet/)         | Default key bindings and mouse actions.                   |
| [FAQ](https://weechat.org/doc/weechat/faq/)                              | Frequently asked questions.                               |
| [Scripting guide](https://weechat.org/doc/weechat/scripting/)            | How to write scripts (Python, Perl, Ruby, etc.).          |
| [Plugin API reference](https://weechat.org/doc/weechat/plugin/)          | API for C plugins and scripts.                            |
| [Relay API](https://weechat.org/doc/weechat/relay_api/)                  | HTTP REST API of the relay plugin ("api" protocol).       |
| [Relay WeeChat protocol](https://weechat.org/doc/weechat/relay_weechat/) | Binary protocol of the relay plugin ("weechat" protocol). |
| [Developer's guide](https://weechat.org/doc/weechat/dev/)                | Sources, coding rules, contributing and translations.     |

The man pages are available with `man weechat` and `man weechat-headless`.\
All the documentation is available in [several languages](https://weechat.org/doc/weechat/).

## Support

- **IRC** (recommended): channels `#weechat` (English) and `#weechat-fr` (French) on server `irc.libera.chat`.
- **Bugs and feature requests**: [GitHub issues](https://github.com/weechat/weechat/issues).

See the [support page](https://weechat.org/about/support/) for more information.

## Contributing

Bug reports, feature requests, translations and pull requests are welcome; please read [CONTRIBUTING.md](CONTRIBUTING.md) first.\
Before submitting a pull request, please read the [coding rules](https://weechat.org/doc/weechat/dev/#coding_rules) and the [commit message format](https://weechat.org/doc/weechat/dev/#git_repository) in the developer's guide.

Please **DO NOT** file a GitHub issue for security related problems; see [SECURITY.md](SECURITY.md) to report a vulnerability.

## Semantic versioning

WeeChat follows "practical" semantic versioning; see [CONTRIBUTING.md](CONTRIBUTING.md#semantic-versioning).

## Copyright

<!-- REUSE-IgnoreStart -->
Copyright © 2003-2026 [Sébastien Helleu](https://github.com/flashcode)

This file is part of WeeChat, the extensible chat client.

WeeChat is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

WeeChat is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
<!-- REUSE-IgnoreEnd -->

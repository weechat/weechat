<!--
SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# AGENTS.md

Instructions for AI coding agents working on WeeChat.

## Project overview

WeeChat (Wee Enhanced Environment for Chat) is a free chat client, fast and
light, written in C and released under GPL-3.0-or-later.

It has a lightweight core (`src/core`, `src/gui`) and optional plugins
(`src/plugins`), like IRC, relay, logger and the scripting plugins (Perl,
Python, Ruby, Lua, Tcl, Guile, JavaScript, PHP).

It runs in a terminal with the ncurses interface (`weechat`) or in background
without interface (`weechat-headless`).

## Build and test

The build uses CMake; the main dependencies are listed in [README.md](README.md),
and the full list is in the
[User's guide](https://weechat.org/doc/weechat/user/#dependencies).

Unit tests require [CppUTest](https://cpputest.github.io/) (package
`libcpputest-dev`) and a C++ compiler.

Build with debug info and tests:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build build -j
```

Run the tests:

```bash
cd build && ctest -V
```

The code must compile without any warning and all tests must pass; see chapter
"General rules" in the Developer's guide for other rules (unit tests, copyright
headers…).

## Project structure

See chapter "Sources" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)), which describes
the directories and every source file of core, plugins and tests.

## Coding guidelines

See chapters "Coding rules" and "Core internals" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)), which contain the
general rules, the C and Python styles and the naming conventions.

## Documentation

When editing documentation, update every language under `doc/`: see chapter
"Asciidoc" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)).

## Changelog

The file [CHANGELOG.md](CHANGELOG.md) must be updated for every user-visible
change: see chapter "Changelog" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)).

## Commits

Follow the rules in the [AI policy](CONTRIBUTING.md#ai-policy) of
[CONTRIBUTING.md](CONTRIBUTING.md), and the format and rules of commit messages
described in chapter "Git repository" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)).

## Security

Never report a security issue publicly (GitHub issue, pull request, commit
message…): see [SECURITY.md](SECURITY.md).

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

WeeChat is single-threaded: any blocking call (`sleep`, blocking I/O…) freezes
the whole client, in core, plugins and scripts.

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

Run the tests (the option `-V` displays the duration of each test):

```bash
cd build && ctest -V
```

Build the documentation (requires `asciidoctor`), to check changes in `doc/`:

```bash
cmake -B build -DENABLE_DOC=ON -DENABLE_MAN=ON
cmake --build build -j
```

Rules:

- The code must compile without any warning: look for `warning:` in the full
  build output, do not rely on the exit code only.
- All tests must pass.
- Add unit tests in `tests/unit/` for any non-trivial change; tests must be
  fast (no sleep, no wait for a timeout), the test suite duration must grow by
  a few milliseconds only.
- When fixing an infinite loop, never run the unfixed code to prove that a test
  catches it.

See chapter "General rules" in the Developer's guide for other rules (unit
tests, copyright headers…).

## Project structure

See chapter "Sources" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)), which describes
the directories and every source file of core, plugins and tests.

Any new source or test file must be added in this chapter, in all languages.

## Coding guidelines

See chapters "Coding rules" and "Core internals" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)), which contain the
general rules, the C and Python styles, the naming conventions and the rules
for `/upgrade` compatibility.

The rules most often missed:

- Never declare C functions `static` (unit tests declare them `extern`),
  including helper functions in tests.
- Comment every function, in the imperative mood ("Check if…", not
  "Checks if…"), and update this comment when the behavior of the function
  changes (parameters, return value, units…).
- Never mention other commits in comments ("added in next commit"…).
- Never change `UPGRADE_SIGNATURE`; keep restoring data saved by older versions
  in upgrade files.

## Documentation

When editing documentation, update every language under `doc/`: see chapter
"Asciidoc" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)).

In languages other than English, untranslated text gets a line
`// TRANSLATION MISSING` above each paragraph, heading, table row and example
caption: add it in the same edit as the English text.

A new feature is documented with the version in development, for example
`_(WeeChat ≥ X.Y.Z)_`: take it from variable `weechat_devel` in
[version.sh](version.sh), not from the most recent marker found in docs.

## Code review

An AI review does not replace the human review, which is mandatory in all cases
(see the [AI policy](CONTRIBUTING.md#ai-policy)).

When reviewing changes, check:

- **Correctness:** memory leaks and use after free (on every error path),
  NULL pointers, buffer overflows and off-by-one errors, return values of
  allocation functions.
- **Security:** data received from the network (IRC servers, relay clients,
  remote relays, DCC) or from scripts is untrusted: check sizes, bounds and
  formats before using it.
- **Single thread:** no blocking call, no long loop on large data in the main
  thread.
- **API:** pointers received from plugins and scripts for buffers, windows,
  bars, bar items and completions are checked in API functions; pointers to
  config files, sections and options are deliberately not checked (for speed).
- **Compatibility:** `/upgrade` still works (same version and from the
  previous release); scripting API, relay protocols and config keep working,
  or the change is marked as breaking and described in
  [UPGRADING.md](UPGRADING.md).
- **Tests:** non-trivial changes have unit tests, which are fast and do not
  depend on external network access or on the local system.
- **Comments:** function comments are in the imperative mood and up to date.
- **Documentation:** updated in all languages (with `// TRANSLATION MISSING`),
  new files listed in chapter "Sources" of the Developer's guide, version
  markers correct.
- **Translatable strings:** new or changed strings (`_("...")`, `N_("...")`)
  are needed and consistent with existing messages.
- **Changelog:** a user-visible change has its entry in the same commit, in the
  right section and at the right place.
- **Commits:** each commit builds, passes tests and works on its own, with a
  message following the rules below.

## Changelog

The file [CHANGELOG.md](CHANGELOG.md) must be updated for every user-visible
change, in the same commit as the change; purely internal changes have no
entry: see chapter "Changelog" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)) for the sections,
the order of entries and the quoting style.

## Commits

Follow the rules in the [AI policy](CONTRIBUTING.md#ai-policy) of
[CONTRIBUTING.md](CONTRIBUTING.md), and the format and rules of commit messages
described in chapter "Git repository" in the Developer's guide
([doc/en/weechat_dev.en.adoc](doc/en/weechat_dev.en.adoc)).

In short:

- Title `component: summary`, with a component from the table in the
  Developer's guide (also for version bumps), about 70 chars max.
- `(closes #123)` at the end of the title for the commit that fixes an issue,
  `(issue #123)` for other commits related to the issue.
- Never list an AI tool as co-author (for example with a `Co-Authored-By`
  trailer); this trailer is allowed only for human co-authors.
- Never refer to other commits ("next commit", "previous commit"…).
- Each commit of a series must build, pass tests and work on its own: split
  a change as "add first, remove later", not by subsystem.

## Security

Never report a security issue publicly (GitHub issue, pull request, commit
message…): see [SECURITY.md](SECURITY.md).

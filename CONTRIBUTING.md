<!--
SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Contributing to WeeChat

## Reporting bugs

First, some basic things:

- Use only English to communicate with developers.
- Search in issues if the same problem or feature request has already been
  reported (a duplicate is a waste of time for you and the developers!).
- If you can, please check if the problem has been fixed in the development version
  (if you are using a stable release or old version).
- Report only one bug or feature request per issue.
- If you use AI tools to help you write your report, please read the
  [AI policy](#ai-policy) first.

### Security reports

Please **DO NOT** file a GitHub issue for security-related problems;
see [SECURITY.md](SECURITY.md) instead.

### Required info

When reporting [issues](https://github.com/weechat/weechat/issues) on GitHub,
please include the following info (the issue form asks for it):

- Your **WeeChat version**: the output of `/v` in WeeChat, for example:
  `WeeChat 5.0.0-dev (git: v4.10.0-214-g4739fb458)`.\
  If WeeChat does not start at all, please include the version displayed by
  `weechat --help` (or the version installed with your package manager).
- Your **operating system**: its name and version (examples: Linux Debian Trixie,
  FreeBSD 15.0, Windows/Cygwin 64-bit, Windows/Ubuntu 64-bit…).
- The **steps to reproduce**: if possible, please include a reproducible example:
  explain the steps which led you to the problem.\
  It's even better if you can reproduce the problem with a new config (and no
  scripts loaded): try `weechat -t` (temporary home directory, deleted on exit)
  and check if you have the problem there.
- The **gdb backtrace** (only for a crash): if you can reproduce the crash
  (or if you have a core file), please include the backtrace from gdb (see the
  [User's guide](https://weechat.org/doc/weechat/user/#report_crashes) for more info).
- The **actual result**.
- The **expected result**: the correct result you are expecting.

> [!IMPORTANT]
> Most of the time, the WeeChat crash log file (_weechat_crash_YYYYMMDD_xxx.log_)
> is **NOT USEFUL** to fix the bug, so please report this file **ONLY** if a developer
> asked you to send it (and be extremely careful, this file can contain personal
> data like passwords and contents of your chats).

### Script-related issues

If you are using scripts, they can cause problems/crashes. To check if the
problem is related to one script, try to unload them one by one (using
command `/script unload <name>`).

Many issues reported are in fact related to bugs in scripts, so please first
check that before reporting any issue on WeeChat itself.

If you think the problem comes from a specific script, please report the issue
in the [scripts git repository](https://github.com/weechat/scripts/issues) instead.

## Translations

Pull requests on GitHub for fixes or new translations are welcome at any
time, for [WeeChat](https://github.com/weechat/weechat) and the website
[weechat.org](https://github.com/weechat/weechat.org).

To start a translation in a new language (not yet supported), please look at
[translations](https://weechat.org/doc/weechat/dev/#translations)
in the Developer's guide.

## Feature requests

WeeChat is under active development, so your idea may already have been
implemented, or scheduled for a future version (you can check in
[roadmap](https://weechat.org/dev/) or
[milestones](https://github.com/weechat/weechat/milestones) on GitHub).

Pull requests on GitHub are welcome for minor new features.

For major new features, it's better to discuss it on IRC
(server: `irc.libera.chat`, channel `#weechat`).

Before submitting any pull request, be sure you have read the
[coding rules](https://weechat.org/doc/weechat/dev/#coding_rules)
in the Developer's guide, which contains info about styles used, naming convention
and other useful info.

## AI policy

Using AI tools to contribute is allowed, under the following conditions:

- You **must** review the AI-generated content yourself before submitting the
  pull request.
- You **must** fully understand everything you submit: you are responsible
  for it, and you must be able to explain and defend it during the review.
- You **must** ensure that the contribution meets WeeChat's usual standards
  for correctness, quality, security, and maintainability.
- Commit messages **must** be short and focused on what changed and why,
  without paraphrasing the code or listing every detail of the diff.

Mentioning in the pull request that AI was used, and for which parts, is
appreciated but not mandatory.

The following is forbidden:

- AI tools **must not** be listed as co-authors in git commits
  (for example with a `Co-Authored-By` trailer).
- AI tools **must not** be used to review a pull request, nor to answer
  comments made by reviewers (except to translate or fix the grammar of your
  own answers): the review is done between humans only.

AI tools can also assist you in reporting issues (including security issues),
but you **must** understand everything in your report: do not let AI write it
without checking it yourself (for example that the problem can actually be
reproduced).

## Semantic versioning

Since version 4.0.0, WeeChat follows a "practical" semantic versioning.

It is based on [Semantic Versioning](https://semver.org/) but in a less strict way:
breaking changes in API with low user impact don't bump the major version.

The version number is made of three numbers `X.Y.Z`, where:

- `X` is the major version
- `Y` is the minor version
- `Z` is the patch version.

Rules to increment the version number:

- the **major version** number (`X`) is incremented only when intentional breaking changes
  target feature areas that are actively consumed by users, scripts or C plugin API
- the **minor version** number (`Y`) is incremented for any new release of WeeChat
  that includes new features and bug fixes, possibly breaking API with low impact on users
- the **patch version** number (`Z`) is reserved for releases that address severe bugs
  or security issues found after the release.

For more information, see the
[specification](https://specs.weechat.org/specs/2023-003-practical-semantic-versioning.html).

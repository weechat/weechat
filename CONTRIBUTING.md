# Contributing to WeeChat

## Reporting bugs

First, some basic things:

- Use only English to communicate with developers.
- Search in issues if the same problem or feature request has already been
  reported (a duplicate is waste of time for you and the developers!).
- If you can, please check if the problem has been fixed in development version
  (if you are using a stable release or old version).
- Report only one bug or feature request per issue.

### Security reports

Please **DO NOT** file a GitHub issue for security related problems, but send an
email to [security@weechat.org](mailto:security@weechat.org) instead.

### Required info

When reporting [issues](https://github.com/weechat/weechat/issues) on GitHub,
please include:

- Your **WeeChat version**: the output of `/v` in WeeChat, for example:
  `WeeChat 1.7-dev (git: v1.6-6-g997f47f)`.\
  If WeeChat does not start at all, please include the version displayed by
  `weechat --help` (or the version installed with your package manager).
- Your **operating system**: its name and version (examples: Linux Debian Bookworm,
  FreeBSD 13.0, Windows/Cygwin 64-bit, Windows/Ubuntu 64-bit…).
- The **steps to reproduce**: if possible, please include a reproducible example:
  explain the steps which led you to the problem.\
  It's even better if you can reproduce the problem with a new config (and no
  scripts loaded): try `weechat --dir /tmp/weechat` and check if you have the
  problem here.
- The **gdb's backtrace** (only for a crash): if you can reproduce the crash
  (or if you have a core file), please include the backtrace from gdb (look at
  [User's guide](https://weechat.org/doc/weechat/user/#report_crashes) for more info).
- The **actual result**.
- The **expected result**: the correct result you are expecting.

> [!IMPORTANT]
> Most of times, the WeeChat crash log file (_weechat_crash_YYYYMMDD_xxx.log_)
is **NOT USEFUL** to fix the bug, so please report this file **ONLY** if a developer
asked you to send it (and be extremely careful, this file can contain personal
data like passwords and contents of your chats).

### Scripts related issues

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
in Developer's guide.

## Feature requests

WeeChat is under active development, so your idea may already have been
implemented, or scheduled for a future version (you can check in
[roadmap](https://weechat.org/dev/) or
[milestones](https://github.com/weechat/weechat/milestones) on GitHub.

Pull requests on GitHub are welcome for minor new features.

For major new features, it's better to discuss about it in IRC
(server: `irc.libera.chat`, channel `#weechat`).

Before submitting any pull request, be sure you have read the
[coding rules](https://weechat.org/doc/weechat/dev/#coding_rules)
in Developer's guide, which contains info about styles used, naming convention
and other useful info.

## Semantic versioning

Since version 4.0.0, WeeChat is following a "practical" semantic versioning.

It is based on [Semantic Versioning](https://semver.org/) but in a less strict way:
breaking changes in API with low user impact don't bump the major version.

The version number is on three digits `X.Y.Z`, where:

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

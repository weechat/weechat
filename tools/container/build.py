#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""Build WeeChat container image from the local sources.

The sources are taken from the git repository containing this script,
including uncommitted changes in tracked files (untracked files are ignored).

The WeeChat version (used for the image tags) is read from file version.sh.
"""

# ruff: noqa: T201

import argparse
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

CONTAINER_DIR = Path(__file__).resolve().parent
ROOT_DIR = CONTAINER_DIR.parent.parent

DISTROS = (
    "debian",
    "alpine",
)


def get_parser() -> argparse.ArgumentParser:
    """Return the command line parser."""
    parser = argparse.ArgumentParser(
        description="Build of WeeChat container image from the local sources",
    )
    parser.add_argument(
        "-b",
        "--builder",
        default="docker",
        help="program used to build the container image, like docker (default) or podman",
    )
    parser.add_argument(
        "-d",
        "--distro",
        choices=DISTROS,
        default=DISTROS[0],
        help="base Linux distribution for the container",
    )
    parser.add_argument(
        "-l",
        "--latest",
        action="store_true",
        help='add tag "latest" (only for a stable version)',
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="dry run: display commands but do not run them",
    )
    parser.add_argument(
        "-s",
        "--slim",
        action="store_true",
        help="slim version: build without plugins: script, scripts, spell",
    )
    parser.add_argument(
        "-t",
        "--tag",
        action="append",
        default=[],
        help="additional tag for the image (can be given multiple times)",
    )
    return parser


def run_git(*args: str) -> str:
    """Run a git command in the repository and return its output."""
    return subprocess.run(  # noqa: S603
        ["git", "-C", str(ROOT_DIR), *args],  # noqa: S607
        capture_output=True,
        check=True,
        text=True,
    ).stdout.strip()


def get_version() -> str:
    """Return the WeeChat version of the sources (like "4.10.3" or "5.0.0-dev")."""
    return subprocess.run(  # noqa: S603
        [str(ROOT_DIR / "version.sh"), "devel-full"],
        capture_output=True,
        check=True,
        text=True,
    ).stdout.strip()


def get_tags(version: str, distro: str, *, slim: bool, latest: bool) -> list[str]:
    """Get tags to apply on the container image.

    :param version: WeeChat version, like "4.10.3", "5.0.0-rc1" or "5.0.0-dev"
    :param distro: "debian" or "alpine"
    :param slim: slim version
    :param latest: add tag "latest" (ignored for a development version)
    :return: list of tag arguments for command line, for example with
        version "4.10.3", distro "alpine" and no slim/latest:
        ['-t', 'weechat:4-alpine', '-t', 'weechat/weechat:4-alpine',
         '-t', 'weechat:4.10-alpine', '-t', 'weechat/weechat:4.10-alpine',
         '-t', 'weechat:4.10.3-alpine', '-t', 'weechat/weechat:4.10.3-alpine']
    """
    str_slim = "-slim" if slim else ""
    suffixes = [str_slim, f"-{distro}{str_slim}"] if distro == "debian" else [f"-{distro}{str_slim}"]
    if version.endswith("-dev"):
        tags = ["devel"]
    elif "-" in version:
        tags = [version]
    else:
        numbers = version.split(".")
        tags = [".".join(numbers[: i + 1]) for i in range(len(numbers))]
        if latest:
            tags.append("latest")
    tags_args = []
    for tag in tags:
        for suffix in suffixes:
            tags_args.extend(["-t", f"weechat:{tag}{suffix}", "-t", f"weechat/weechat:{tag}{suffix}"])
    return tags_args


def main() -> int:
    """Build the container image."""
    args = get_parser().parse_args()
    version = get_version()
    tags = get_tags(version, args.distro, slim=args.slim, latest=args.latest)
    for tag in args.tag:
        tags.extend(["-t", tag])
    # commit with uncommitted changes in tracked files, or HEAD if no changes
    treeish = run_git("stash", "create") or "HEAD"
    with tempfile.TemporaryDirectory(prefix="weechat-container-") as context_dir:
        archive_cmd = [
            "git",
            "-C",
            str(ROOT_DIR),
            "archive",
            "--prefix=weechat/",
            "-o",
            str(Path(context_dir) / "weechat.tar"),
            treeish,
        ]
        build_cmd = [
            args.builder,
            "build",
            "-f",
            str(CONTAINER_DIR / f"Containerfile.{args.distro}"),
            *(["--build-arg", "SLIM=1"] if args.slim else []),
            *tags,
            context_dir,
        ]
        print(f"WeeChat version: {version}")
        for command in (archive_cmd, build_cmd):
            print(f"Running: {shlex.join(command)}")
            if args.dry_run:
                continue
            try:
                subprocess.run(command, check=True)  # noqa: S603
            except KeyboardInterrupt:
                return 1
            except (OSError, subprocess.CalledProcessError) as exc:
                print(exc, file=sys.stderr)
                return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

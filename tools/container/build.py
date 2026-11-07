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

from __future__ import annotations

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

# images built with option --all: (distro, slim)
ALL_IMAGES = (
    ("debian", False),
    ("debian", True),
    ("alpine", False),
    ("alpine", True),
)

PUSH_REPOSITORY = "weechat/weechat"


def get_parser() -> argparse.ArgumentParser:
    """Return the command line parser."""
    parser = argparse.ArgumentParser(
        description="Build of WeeChat container image from the local sources",
    )
    parser.add_argument(
        "-a",
        "--all",
        action="store_true",
        help="build all images: debian, debian slim, alpine and alpine slim (not compatible with -d, -s and -t)",
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
        help=f"base Linux distribution for the container (default: {DISTROS[0]})",
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
        "-p",
        "--push",
        action="store_true",
        help=(
            f'push the tags "{PUSH_REPOSITORY}:*" of the images built, '
            "only if all images are built successfully and there are no uncommitted changes in tracked files "
            "(login to the registry is required)"
        ),
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
        help="additional tag for the image (can be given multiple times), it is never pushed",
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

    A development version is tagged with its full version, and also with
    "devel" if it is the development version of the main branch (patch
    version 0, like "5.0.0-dev"): a development version of a maintenance
    branch (like "4.10.4-dev") is not tagged "devel".

    :param version: WeeChat version, like "4.10.3", "5.0.0-rc1", "5.0.0-dev"
        or "4.10.4-dev"
    :param distro: "debian" or "alpine"
    :param slim: slim version
    :param latest: add tag "latest" (ignored for a development version or
        a release candidate)
    :return: list of tags, for example with version "4.10.3", distro "alpine"
        and no slim/latest:
        ['weechat:4-alpine', 'weechat/weechat:4-alpine',
         'weechat:4.10-alpine', 'weechat/weechat:4.10-alpine',
         'weechat:4.10.3-alpine', 'weechat/weechat:4.10.3-alpine']
    """
    str_slim = "-slim" if slim else ""
    suffixes = [str_slim, f"-{distro}{str_slim}"] if distro == "debian" else [f"-{distro}{str_slim}"]
    if version.endswith("-dev"):
        tags = [version]
        if version.split("-", maxsplit=1)[0].split(".")[2:] == ["0"]:
            tags.append("devel")
    elif "-" in version:
        tags = [version]
    else:
        numbers = version.split(".")
        tags = [".".join(numbers[: i + 1]) for i in range(len(numbers))]
        if latest:
            tags.append("latest")
    return [f"{name}:{tag}{suffix}" for tag in tags for suffix in suffixes for name in ("weechat", PUSH_REPOSITORY)]


def get_push_command(builder: str, tag: str) -> list[str]:
    """Get command to push a tag to the registry.

    Podman stores images built without registry in "localhost/", so the
    destination on Docker Hub must be given explicitly.
    """
    if Path(builder).name == "podman":
        return [builder, "push", tag, f"docker.io/{tag}"]
    return [builder, "push", tag]


def run_command(command: list[str], *, dry_run: bool) -> bool:
    """Display and run a command (unless dry run).

    :return: True if the command succeeded (or dry run), False if it failed
    """
    print(f"Running: {shlex.join(command)}")
    if dry_run:
        return True
    try:
        subprocess.run(command, check=True)  # noqa: S603
    except KeyboardInterrupt:
        return False
    except (OSError, subprocess.CalledProcessError) as exc:
        print(exc, file=sys.stderr)
        return False
    return True


def get_treeish() -> str | None:
    """Get the git tree-ish to build: commit with uncommitted changes in tracked files, or HEAD.

    :return: tree-ish, None if git failed (for example if not in a git repository)
    """
    try:
        return run_git("stash", "create") or "HEAD"
    except OSError as exc:
        print(f"ERROR: unable to run git: {exc}", file=sys.stderr)
    except subprocess.CalledProcessError as exc:
        print("ERROR: this script must be run from the WeeChat git repository", file=sys.stderr)
        print(exc.stderr.strip(), file=sys.stderr)
    return None


def build_images(args: argparse.Namespace, version: str, treeish: str) -> list[str] | None:
    """Build the container images.

    :param args: command line arguments
    :param version: WeeChat version
    :param treeish: git tree-ish to build
    :return: tags to push, None if an error occurred
    """
    images = ALL_IMAGES if args.all else ((args.distro or DISTROS[0], args.slim),)
    push_tags = []
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
        if not run_command(archive_cmd, dry_run=args.dry_run):
            return None
        for distro, slim in images:
            tags = get_tags(version, distro, slim=slim, latest=args.latest)
            build_cmd = [
                args.builder,
                "build",
                "-f",
                str(CONTAINER_DIR / f"Containerfile.{distro}"),
                *(["--build-arg", "SLIM=1"] if slim else []),
                *[arg for tag in tags + args.tag for arg in ("-t", tag)],
                context_dir,
            ]
            if not run_command(build_cmd, dry_run=args.dry_run):
                return None
            push_tags.extend(tag for tag in tags if tag.startswith(f"{PUSH_REPOSITORY}:"))
    return push_tags


def main() -> int:
    """Build the container images, and push them if asked."""
    parser = get_parser()
    args = parser.parse_args()
    if args.all and (args.distro or args.slim or args.tag):
        parser.error("option -a/--all is not compatible with -d/--distro, -s/--slim and -t/--tag")
    version = get_version()
    print(f"WeeChat version: {version}")
    treeish = get_treeish()
    if treeish is None:
        return 1
    if args.push and treeish != "HEAD":
        print("ERROR: uncommitted changes in tracked files, images can not be pushed", file=sys.stderr)
        return 1
    push_tags = build_images(args, version, treeish)
    if push_tags is None:
        return 1
    if args.push:
        for tag in push_tags:
            if not run_command(get_push_command(args.builder, tag), dry_run=args.dry_run):
                return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

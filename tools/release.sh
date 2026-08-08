#!/bin/bash
#
# SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Make a new WeeChat release:
#   1. bump version
#   2. git commit + tag
#   3. compile, run, test, build packages
#   4. test package: unpack, compile, run, test

set -euo pipefail

release_error ()
{
    [ $# -gt 0 ] && echo >&2 "ERROR: $*"
    exit 1
}

release_start ()
{
    date=$(date +"%Y-%m-%d")

    # Check that working directory is clean.
    root_dir="$(git rev-parse --show-toplevel)"
    if [ -n "$(git status --porcelain)" ]; then
        release_error "working directory not clean"
    fi

    # Get the target used in the changelog compare links: "HEAD" on the main
    # branch, the branch itself on a maintenance branch (for example "4.10").
    branch=$(git rev-parse --abbrev-ref HEAD)
    case "${branch}" in
        main ) compare_target="HEAD" ;;
        [0-9]*.[0-9]* ) compare_target="${branch}" ;;
        * ) release_error "branch \"${branch}\" is neither the main branch nor a maintenance branch" ;;
    esac

    # Sanity check: the changelog must have an [Unreleased] section to release.
    if ! grep -q '^## \[Unreleased\]$' "${root_dir}/CHANGELOG.md"; then
        release_error "no [Unreleased] section found in CHANGELOG.md"
    fi
    # Sanity check: the [Unreleased] compare link is required to build the release links.
    if ! grep -q "^\[Unreleased\]: .*/compare/.*\.\.\.${compare_target}\$" "${root_dir}/CHANGELOG.md"; then
        release_error "no valid [Unreleased] link ending with \"...${compare_target}\" found in CHANGELOG.md"
    fi

    # Check REUSE/licensing compliance.
    reuse lint

    # Get next version.
    version=$("${root_dir}/version.sh" devel)
    if git rev-parse "v${version}" 2>/dev/null; then
        release_error "tag v${version} already exists"
    fi

    # Check if commit for the version already exists.
    msg=$(git log -1 --pretty=%B | tr -d "\n")
    if [ "${msg}" = "Version ${version}" ]; then
        release_error "commit for version already exists"
    fi

    # Create build directory.
    build_dir="${root_dir}/release/${version}"
    if [ -d "${build_dir}" ]; then
        release_error "directory ${build_dir} already exists"
    fi
    mkdir -p "${build_dir}"
    pkg_tar="${build_dir}/weechat-${version}.tar"
}

release_bump_version ()
{
    # Bump version in script version.sh.
    "${root_dir}/tools/bump_version.sh" stable

    # Derive the compare-URL base and the previous tag from the [Unreleased] link,
    # e.g. on the main branch:
    #   [Unreleased]: https://github.com/weechat/weechat/compare/v4.9.0...HEAD
    # and on the maintenance branch 4.10:
    #   [Unreleased]: https://github.com/weechat/weechat/compare/v4.10.0...4.10
    unreleased_link=$(grep -m1 '^\[Unreleased\]: ' "${root_dir}/CHANGELOG.md" | sed -E 's/^\[Unreleased\]: //')
    base_url=${unreleased_link%%/compare/*}
    prev_tag=${unreleased_link##*/compare/}
    prev_tag=${prev_tag%"...${compare_target}"}

    # Update the changelog:
    #   - turn the [Unreleased] heading into the released version and date
    #   - point the [Unreleased] link at the new version
    #   - add the new version link (kept in descending order, after [Unreleased])
    sed -i \
        -e "s|^## \[Unreleased\]$|## [${version}] - ${date}|" \
        -e "s|^\[Unreleased\]: .*|[Unreleased]: ${base_url}/compare/v${version}...${compare_target}|" \
        -e "/^\[Unreleased\]: /a [${version}]: ${base_url}/compare/${prev_tag}...v${version}" \
        "${root_dir}/CHANGELOG.md"

    # Update the upgrading notes: turn the "Unreleased" heading into the released version.
    sed -i "s|^## Unreleased$|## Version ${version}|" "${root_dir}/UPGRADING.md"
}

release_commit_tag ()
{
    cd "${root_dir}"
    git commit -m "Version ${version}" version.sh CHANGELOG.md UPGRADING.md || release_error "git commit error, release already done?"
    git tag -a "v${version}" -m "WeeChat ${version}"
}

release_build ()
{
    cd "${build_dir}"
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${build_dir}/install" \
        -DWEECHAT_HOME="${build_dir}/home" \
        -DENABLE_DOC=ON \
        -DENABLE_MAN=ON \
        -DENABLE_TESTS=ON \
        "${root_dir}"
    make install
    make test CTEST_OUTPUT_ON_FAILURE=TRUE
    make dist
    version_weechat=$("${build_dir}/install/bin/weechat" --version)
    if [ "${version_weechat}" != "${version}" ]; then
        release_error "unexpected version \"${version_weechat}\" (expected: \"${version}\")"
    fi
}

release_test_pkg ()
{
    cd "${build_dir}"
    tar axvf "weechat-${version}.tar.xz"
    cd "weechat-${version}"
    pkg_dir="$(pwd)"
    script_version="${pkg_dir}/version.sh"
    [ "$("${script_version}" stable)" = "${version}" ] || release_error "wrong stable version in ${script_version}"
    [ "$("${script_version}" devel)" = "${version}" ] || release_error "wrong devel version in ${script_version}"
    [ "$("${script_version}" devel-full)" = "${version}" ] || release_error "wrong devel-full version in ${script_version}"
    mkdir build
    cd build
    pkg_build_dir="$(pwd)"
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${pkg_build_dir}/install" \
        -DWEECHAT_HOME="${pkg_build_dir}/home" \
        -DENABLE_DOC=ON \
        -DENABLE_MAN=ON \
        -DENABLE_TESTS=ON \
        "${pkg_dir}"
    make install
    make test CTEST_OUTPUT_ON_FAILURE=TRUE
    version_weechat=$("${pkg_build_dir}/install/bin/weechat" --version)
    if [ "${version_weechat}" != "${version}" ]; then
        release_error "unexpected version \"${version_weechat}\" (expected: \"${version}\")"
    fi
}

release_end ()
{
    # Display a report about the release made.
    echo
    echo "========================= WeeChat release status ========================="
    echo "  version  : ${version}"
    echo "  date     : ${date}"
    echo "  build dir: ${build_dir}"
    echo "  packages :"
    for pkg in "${pkg_tar}".*; do
        echo "    $pkg"
    done
    echo "=========================================================================="
    echo
    echo "*** SUCCESS! ***"
    echo
}

release_start

release_bump_version
release_commit_tag
release_build
release_test_pkg

release_end

exit 0

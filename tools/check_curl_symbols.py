#!/usr/bin/env python3
#
# Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

"""
Check if Curl symbols defined in src/core/wee-url.c are matching symbols
defined in Curl (introduced/deprecated/last versions), using this file:
https://github.com/curl/curl/blob/master/docs/libcurl/symbols-in-versions.

File symbols-in-versions must be passed as stdin to the script.

Usage example:

```
URL=https://raw.githubusercontent.com/curl/curl/master/docs/libcurl/symbols-in-versions
curl $URL | ./check_curl_symbols.py
```

Or with Curl repository cloned locally:

```
./check_curl_symbols.py < /path/to/curl/docs/libcurl/symbols-in-versions
```

This script requires Python 3.7+.
"""

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, TextIO, Tuple

import re
import sys

SRC_PATH = (
    Path(__file__).resolve().parent.parent / "src" / "core" / "wee-url.c"
)

WEECHAT_CURL_MIN_VERSION_RE = (
    r"#if LIBCURL_VERSION_NUM >= (?P<hex_min_version>0x[0-9A-F]+) "
    r"/\* (?P<str_min_version>[0-9][0-9.]+) \*/"
)

WEECHAT_CURL_MIN_MAX_VERSION_RE = (
    r"#if LIBCURL_VERSION_NUM >= (?P<hex_min_version>0x[0-9A-F]+) "
    r"&& LIBCURL_VERSION_NUM < (?P<hex_max_version>0x[0-9A-F]+) "
    r"/\* (?P<str_min_version>[0-9][0-9.]+) < "
    r"(?P<str_max_version>[0-9][0-9.]+) \*/"
)
WEECHAT_ENDIF_RE = r"#endif"
WEECHAT_CURL_CONSTANT_RE = (
    r"    URL_DEF_CONST\((?P<prefix>[A-Z0-9_]+), (?P<name>[A-Z0-9_]+)\),"
)
WEECHAT_CURL_OPTION_RE = (
    r"    URL_DEF_OPTION\((?P<name>[A-Z0-9_]+), (?P<type>[A-Z]+)\), "
    r"(?P<values>[A-Za-z0-9_]+)\),"
)
CURL_SYMBOL_RE = r"[A-Z][A-Z0-9_]"


@dataclass
class WeechatCurlSymbol:
    """A Curl symbol declared in WeeChat."""

    name: str
    min_curl: int = 0
    max_curl: int = 0
    line_no: int = 0


def curl_version_to_int(version: str) -> int:
    """
    Convert Curl version as string to integer.

    :param version: version as string (eg: "7.87.0")
    :return: version as integer, eg: 481024 (== 0x075700, 0x57 == 87)
    """
    if version == "-":
        return 0
    result = 0
    items = version.split(".")
    if len(items) < 3:
        items.append("0")
    factor = 0
    for item in reversed(items):
        result += int(item) << factor
        factor += 8
    return result


def curl_version_to_str(version: int) -> str:
    """
    Convert Curl version as integer to string.

    :param version: version as integer, eg: 481024 (0x075700)
    :return: version as string, eg: "7.87.0"
    """
    if version == 0:
        return "-"
    result = ""
    while version > 0:
        result = str(version & 0xFF) + "." + result
        version = version >> 8
    return result.rstrip(".")


def get_curl_symbols(symbols_file: TextIO) -> Dict[str, Tuple[int, int]]:
    """
    Parse file docs/libcurl/symbols-in-versions from Curl repository.

    :param symbols_file: file with Curl symbols
    :return: Curl symbols as dict: {name: (version_min, version_max)}
    """
    curl_symbol_pattern = re.compile(CURL_SYMBOL_RE)
    symbols: Dict[str, Tuple[int, int]] = {}
    if symbols_file.isatty():
        return symbols
    for line in symbols_file:
        match = re.match(curl_symbol_pattern, line)
        if match:
            name, intro, deprec, last, *_ = (line + " - -").split()
            v_max = last if last != "-" else deprec
            symbols[name] = (
                curl_version_to_int(intro),
                curl_version_to_int(v_max),
            )
    return symbols


def get_weechat_curl_symbols() -> Tuple[List[WeechatCurlSymbol], int]:
    """
    Parse Curl symbols declared in src/core/wee-url.c.

    :return: tuple (list_symbols, errors)
    """
    # pylint: disable=too-many-locals,too-many-statements
    min_version_pattern = re.compile(WEECHAT_CURL_MIN_VERSION_RE)
    min_max_version_pattern = re.compile(WEECHAT_CURL_MIN_MAX_VERSION_RE)
    endif_pattern = re.compile(WEECHAT_ENDIF_RE)
    constant_pattern = re.compile(WEECHAT_CURL_CONSTANT_RE)
    option_pattern = re.compile(WEECHAT_CURL_OPTION_RE)
    v_min: int = 0
    v_max: int = 0
    symbols: List[WeechatCurlSymbol] = []
    errors: int = 0
    line_no: int = 0
    with open(SRC_PATH, encoding="utf-8") as src_file:
        for line in src_file:
            line_no += 1
            # min Curl version
            match = re.match(min_version_pattern, line)
            if match:
                hex_min_vers = match["hex_min_version"]
                str_min_vers = match["str_min_version"]
                v_min, v_max = int(hex_min_vers, 0), 0
                comment_min = curl_version_to_int(str_min_vers)
                if v_min != comment_min:
                    print(
                        f"{SRC_PATH}:{line_no}: min version not matching "
                        f"the comment: "
                        f"{hex_min_vers} != {str_min_vers}"
                    )
                    errors += 1
                continue
            # min + max Curl version
            match = re.match(min_max_version_pattern, line)
            if match:
                hex_min_vers = match["hex_min_version"]
                hex_max_vers = match["hex_max_version"]
                str_min_vers = match["str_min_version"]
                str_max_vers = match["str_max_version"]
                v_min, v_max = int(hex_min_vers, 0), int(hex_max_vers, 0)
                comment_min = curl_version_to_int(str_min_vers)
                comment_max = curl_version_to_int(str_max_vers)
                if v_min != comment_min:
                    print(
                        f"{SRC_PATH}:{line_no}: min version not matching "
                        f"the comment: "
                        f"{hex_min_vers} != {str_min_vers}"
                    )
                    errors += 1
                if v_max != comment_max:
                    print(
                        f"{SRC_PATH}:{line_no}: max version not matching "
                        f"the comment: "
                        f"{hex_max_vers} != {str_max_vers}"
                    )
                    errors += 1
                continue
            # end of min/max Curl version
            match = re.match(endif_pattern, line)
            if match:
                v_min, v_max = 0, 0
                continue
            # Curl constant
            match = re.match(constant_pattern, line)
            if match:
                name = f"CURL{match['prefix']}_{match['name']}"
                symbols.append(WeechatCurlSymbol(name, v_min, v_max, line_no))
                continue
            # Curl option
            match = re.match(option_pattern, line)
            if match:
                symbols.append(
                    WeechatCurlSymbol(match["name"], v_min, v_max, line_no)
                )
                continue
    return symbols, errors


def check_symbols(
    weechat_curl_symbols: List[WeechatCurlSymbol],
    curl_symbols: Dict[str, Tuple[int, int]],
) -> int:
    """
    Check that symbols declared are matching Curl symbols.

    :param weechat_curl_symbols: list of Curl symbols in WeeChat
    :param curl_symbols: list of all Curl symbols
    """
    to_str = curl_version_to_str
    errors = 0
    for symbol in weechat_curl_symbols:
        curl_symbol = curl_symbols.get(symbol.name)
        if not curl_symbol:
            print(
                f"{SRC_PATH}:{symbol.line_no}: symbol {symbol.name} "
                f"not found in Curl"
            )
            errors += 1
            continue
        if symbol.min_curl != curl_symbol[0]:
            print(
                f"{SRC_PATH}:{symbol.line_no}: min version for "
                f"symbol {symbol.name} differs: "
                f"{to_str(symbol.min_curl)} in WeeChat, "
                f"{to_str(curl_symbol[0])} in Curl"
            )
            errors += 1
        if symbol.max_curl != curl_symbol[1]:
            print(
                f"{SRC_PATH}:{symbol.line_no}: max version for "
                f"symbol {symbol.name} differs: "
                f"{to_str(symbol.max_curl)} in WeeChat, "
                f"{to_str(curl_symbol[1])} in Curl"
            )
            errors += 1
    return errors


def main() -> int:
    """Check Curl symbols and return the number of errors found."""
    curl_symbols = get_curl_symbols(sys.stdin)
    if not curl_symbols:
        sys.exit("FATAL: failed to read Curl symbols on standard input")
    weechat_curl_symbols, errors = get_weechat_curl_symbols()
    errors += check_symbols(weechat_curl_symbols, curl_symbols)
    dict_err = {0: "all good!", 1: "1 error"}
    print("Curl symbols:", dict_err.get(errors, f"{errors} errors"))
    return errors


if __name__ == "__main__":
    sys.exit(min(main(), 255))

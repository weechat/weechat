#!/bin/sh
#
# Copyright (C) 2024-2025 Sébastien Helleu <flashcode@flashtux.org>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

#
# Test WeeChat relay HTTP REST API.
#
# Environment variables that can be used:
#
#   RELAY_PASSWORD  Password for WeeChat relay
#

set -o errexit

# default values for options from environment variables
default_relay_password="test"

usage ()
{
    rc=$1
    cat <<-EOF

Syntax: $0 url

  url  URL of the running WeeChat with relay api (without "/api")

Environment variables used:

  RELAY_PASSWORD  password for the relay (default: "${default_relay_password}")

Example:

  RELAY_PASSWORD="test" $0 http://localhost:9000

EOF
    exit "${rc}"
}

error_usage ()
{
    echo >&2 "ERROR: $*"
    usage 1
}

# ================================== START ==================================

# relay password
[ -z "${RELAY_PASSWORD}" ] && RELAY_PASSWORD="${default_relay_password}"

# check command line arguments
if [ $# -eq 0 ]; then
    usage 0
fi
if [ $# -lt 1 ]; then
    error_usage "missing arguments"
fi

# command line arguments
url="$1"

schemathesis run \
    --checks all \
    --validate-schema=true \
    --experimental=openapi-3.1 \
    --base-url "${url}/api" \
    --auth "plain:${RELAY_PASSWORD}" \
    ./src/plugins/relay/api/weechat-relay-api.yaml \
    ;

exit 0

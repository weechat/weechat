<!--
SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Containerized WeeChat

Build of WeeChat images using [Docker](https://www.docker.com/) (default) or [Podman](https://podman.io/).

## Requirements

The following packages are required to build images:

- Docker or Podman
- git
- Python ≥ 3.9

## Image types

Images are based on Debian or Alpine (smaller size, with same features):

- Debian:
  - `debian` (~ 328 MB)
  - `debian-slim`: slim version (~ 125 MB)
- Alpine:
  - `alpine` (~ 218 MB)
  - `alpine-slim`: slim version (~ 41 MB)

The slim version includes all plugins except these ones:

- script manager (command `/script`)
- scripting languages: perl, python, ruby, lua, tcl, guile, php
- spell

## Install and run with Docker Hub

You can install and run directly the latest version from the [Docker Hub](https://hub.docker.com/r/weechat/weechat):

```bash
docker run -it weechat/weechat
```

Or the Alpine version:

```bash
docker run -it weechat/weechat:latest-alpine
```

For a specific WeeChat version (Debian):

```bash
docker run -it weechat/weechat:4.10.3
```

Run with custom home directories on host to persist data (WeeChat ≥ 3.2, using XDG directories):

```bash
mkdir -p ~/.weechat-container/config ~/.weechat-container/data ~/.weechat-container/cache
chmod 777 ~/.weechat-container/config ~/.weechat-container/data ~/.weechat-container/cache
docker run -it \
    -v $HOME/.weechat-container/config:/home/user/.config/weechat \
    -v $HOME/.weechat-container/data:/home/user/.local/share/weechat \
    -v $HOME/.weechat-container/cache:/home/user/.cache/weechat \
    weechat/weechat
```

Run with a custom single home directory on host to persist data (any WeeChat version):

```bash
mkdir -p ~/.weechat-container
chmod 777 ~/.weechat-container
docker run -it \
    -v $HOME/.weechat-container:/home/user/.weechat \
    weechat/weechat \
    weechat -d /home/user/.weechat
```
## Networking

By default, the container runs with an isolated network stack. If you need to
access WeeChat relay ports (for example, for remote clients), you have two options:

### Expose a range of ports

You can publish a range of ports from the container to the host using `-p`:

```bash
docker run -it \
    -p 1400-1405:1400-1405 \
    -v $HOME/.weechat-container/config:/home/user/.config/weechat \
    -v $HOME/.weechat-container/data:/home/user/.local/share/weechat \
    -v $HOME/.weechat-container/cache:/home/user/.cache/weechat \
    weechat/weechat
```

This maps ports `1400` to `1405` from the container to the same ports on the host.

> [!NOTE]
> Port ranges must match on both sides (`host:container`).\
> Docker does not support remapping ranges to different port numbers.

### Use host networking

Alternatively, you can use the host network stack:

```bash
docker run -it \
    --network host \
    -v $HOME/.weechat-container/config:/home/user/.config/weechat \
    -v $HOME/.weechat-container/data:/home/user/.local/share/weechat \
    -v $HOME/.weechat-container/cache:/home/user/.cache/weechat \
    weechat/weechat
```

With this mode, WeeChat binds directly to the host network interfaces and ports,
without any port mapping.

> [!WARNING]
> `--network host` removes network isolation.\
> Not supported in rootless Docker.\
> Not available on Docker Desktop (macOS/Windows).

### Which one should I use?

- Use `-p` (recommended): safer, works everywhere, including rootless Docker.
- Use `--network host`: only if you need direct access to all ports and are running Docker in rootful mode on Linux.

## Build

Images are built from the local WeeChat sources, so the image always matches
the branch it is built from (dependencies differ between WeeChat versions).

The script `build.py` creates an archive of the git repository (including
uncommitted changes in tracked files, but not untracked files) and builds the
image with it.

The WeeChat version is read from file `version.sh` and used for the image tags:

- development version: tag with the full version (like `5.0.0-dev`), plus `devel`
  for the main branch (patch version 0)
- release candidate (like `5.0.0-rc1`): tag with the full version
- stable version (like `4.10.3`): tags `4.10.3`, `4.10` and `4`, plus `latest`
  with option `--latest`

Build a Debian-based image:

```bash
tools/container/build.py
```

Build an Alpine-based image, slim version, with Podman:

```bash
tools/container/build.py --builder podman --distro alpine --slim
```

Build all images (Debian, Debian slim, Alpine, Alpine slim):

```bash
tools/container/build.py --all
```

Display the commands without running them:

```bash
tools/container/build.py --dry-run
```

See `tools/container/build.py --help` for all options.

## Publish images

To publish images of a stable release on the Docker Hub, build all images
from the release tag (with option `--latest` if it is the latest stable
release) and push them with option `--push`.

Only the tags `weechat/weechat:*` of the images built are pushed, after all
images are built successfully; the push is refused if there are uncommitted
changes in tracked files.

Example for the release 4.10.4 (a login to the Docker Hub is required):

```bash
git switch --detach v4.10.4
docker login
tools/container/build.py --all --latest --push
```

## Update image

### Pull the latest image

First pull the latest image:

```bash
docker pull weechat/weechat:latest
```

Or for Alpine:

```bash
docker pull weechat/weechat:latest-alpine
```

### Recreate the container

This preserves your persistent data.

If you use persistent volumes (recommended):

```bash
docker stop weechat
docker rm weechat
docker run -it \
  -v $HOME/.weechat-container/config:/home/user/.config/weechat \
  -v $HOME/.weechat-container/data:/home/user/.local/share/weechat \
  -v $HOME/.weechat-container/cache:/home/user/.cache/weechat \
  weechat/weechat:latest
```

If you use a single home directory:

```bash
docker stop weechat
docker rm weechat
docker run -it \
  -v $HOME/.weechat-container:/home/user/.weechat \
  weechat/weechat:latest \
  weechat -d /home/user/.weechat
```

### Remove the old image

After verifying the new container works:

```bash
docker image prune -f
```

Or to remove only the specific old WeeChat image:

```bash
docker images | grep weechat/weechat
docker rmi <old-image-id>
```

> [!NOTE]
> Your WeeChat configuration, logs, and scripts are preserved as long as you use volume mounts.\
> Always ensure you have backups before updating.\
> The old image will continue taking disk space until pruned.

FROM debian:stable-slim

RUN apt-get update && apt-get install apt-utils \
    libgcrypt20-dev \
    cmake \
    gnutls-dev \
    zlib1g-dev \
    libcurl4-gnutls-dev \
    libncurses-dev \
    libaspell-dev \
    pkg-config \
    php7.3-dev \
    python3-dev \
    libperl-dev \
    liblua5.2-dev \
    ruby-dev \
    guile-2.2-dev \
    tcl-dev \
    libphp-embed \
    libargon2-dev \
    libxml2-dev \
    libsodium-dev -y

RUN mkdir -p /usr/local/weechat/build
WORKDIR /usr/local/weechat
COPY . .

WORKDIR /usr/local/weechat/build
RUN cmake ..
RUN make && make install
ENTRYPOINT ["weechat"]

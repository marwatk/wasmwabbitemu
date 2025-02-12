FROM ubuntu:20.04 AS build

ENV EMSDK_VERSION=3.1.15

RUN set -ex; \
    apt-get update; \
    DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC \
      apt-get install -y \
        build-essential \
        cmake \
        curl \
        g++ \
        git \
        libsdl2-dev \
        python \
        ; \
    mkdir -p /opt/src; \
    : ;

RUN set -ex; \
    cd /opt; \
    git clone https://github.com/emscripten-core/emsdk.git; \
    cd emsdk; \
    git checkout "${EMSDK_VERSION}"; \
    ./emsdk install "${EMSDK_VERSION}"; \
    ./emsdk activate latest; \
    : ;

ENV PATH="/opt/emsdk:/opt/emsdk/node/14.18.2_64bit/bin:/opt/emsdk/upstream/emscripten:$PATH"
ENV EMSDK=/opt/emsdk
ENV EM_CONFIG=/opt/emsdk/.emscripten
ENV EMSDK_NODE=/opt/emsdk/node/14.18.2_64bit/bin/node

RUN npm install -g sass

COPY build/ /opt/src/build/
COPY core/ /opt/src/core/
COPY gui /opt/src/gui/
COPY hardware/ /opt/src/hardware/
COPY interface/ /opt/src/interface/
COPY utilities/ /opt/src/utilities/
COPY Makefile stdafx.h /opt/src/

WORKDIR /opt/src

# TODO: This downloads a precompiled sdl2 which should be 
# embedded with a fixed version instead.
RUN set -ex; \
    env; \
    # Display what we're going to do, useful for debuggin
    emmake make -n; \
    emmake make -j8; \
    : ;

COPY css/ /opt/src/css/

WORKDIR /opt/src/css

RUN sass *.scss /opt/root/public/wxWabbitemu.css

COPY docker/ /opt/root/

# Build the eventual webserver
RUN set -ex; \
    mkdir -p /opt/root/public/roms; \
    cp \
      /opt/src/bin/wxWabbitemu.js \
      /opt/src/bin/wxWabbitemu.wasm \
      /opt/root/public/; \
    chmod -R 444 /opt/root; \
    chmod 555 \
      /opt/root/public \
      /opt/root/public/roms \
      ; \
    chmod a+x /opt/root/entrypoint; \
    : ;

FROM busybox:1.34.1

COPY --from=build /opt/root /

VOLUME /public/roms
EXPOSE 8080

RUN set -e; \
    touch /public/romlist.txt; \
    chmod 666 /public/romlist.txt; \
    : ;

# We don't set USER here to allow for rootless running (e.g. podman)
# without having to create subuids, but you can run as non-root using
# -u on docker run.

ENTRYPOINT ["/entrypoint"]

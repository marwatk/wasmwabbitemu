FROM ubuntu:20.04 AS build

ENV EMSDK_VERSION=3.1.15
ENV BUSYBOX_STATIC=https://busybox.net/downloads/binaries/1.35.0-i686-linux-musl/busybox

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
    mkdir -p /opt/root/bin/; \
    curl -o /opt/root/bin/httpd "${BUSYBOX_STATIC}"; \
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

WORKDIR /opt/src
COPY . /opt/src/

RUN set -ex; \
    env; \
    emmake make -n; \
    emmake make -j8; \
    chmod a-w /opt/src/bin/wxWabbitemu.*; \
    : ;

RUN set -ex; \
    mkdir /opt/root/etc/; \
    echo "" > /opt/root/etc/httpd.conf; \
    echo "I:index.html" >> /opt/root/etc/httpd.conf; \
    echo ".wasm:application/wasm" >> /opt/root/etc/httpd.conf; \
    echo "H:/public" >> /opt/root/etc/httpd.conf; \
    mkdir -p /opt/root/public/roms; \
    cp \
      /opt/src/bin/wxWabbitemu.js \
      /opt/src/bin/wxWabbitemu.wasm \
      /opt/src/index.html \
      /opt/root/public/; \
    chmod -R 444 /opt/root; \
    chmod 555 \
      /opt/root/bin \
      /opt/root/bin/httpd \
      /opt/root/public \
      /opt/root/public/roms \
      ; \
    : ;

FROM scratch

COPY --from=build /opt/root /

VOLUME /public/roms
EXPOSE 8080

ENTRYPOINT ["/bin/httpd"]
CMD ["-p", "8080", "-f"]

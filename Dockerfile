FROM ubuntu:20.04

ENV EMSDK_VERSION=3.1.15

RUN set -ex; \
    apt-get update; \
    DEBIAN_FRONTEND=noninteractive TZ=Etc/UTC \
      apt-get install -y \
        cmake \
        g++ \
        git \
        python \
        ; \
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

RUN set -ex; \
    mkdir -p /opt/src; \
    : ;

WORKDIR /opt/src

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y libsdl2-dev
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y x11-xserver-utils
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y busybox

RUN apt-get install -y tightvncserver

ENV USER=root

RUN set -ex; \
    mkdir -p /root/.vnc; \
    echo -n password | vncpasswd -f > /root/.vnc/passwd; \
    chmod 600 /root/.vnc/passwd; \
    : ;

COPY . /opt/src/
RUN set -ex; \
    env; \
    cp TI85.ROM build/z.rom; \
    emmake make -j8; \
    cp -v index.html bin/; \
    : ;

ENV DISPLAY=:1
ENV HOME=/root

CMD ["busybox", "httpd", "-p", "8080", "-h", "/opt/src/bin", "-f"]
#CMD ["bash", "-c", "tightvncserver; DISPLAY=:1 bin/wxWabbitemu z.rom"]
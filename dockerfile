FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386 \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        wget \
        patch \
        csh \
        file \
        gcc-multilib \
        g++-multilib \
        lib32ncurses5 \
        lib32z1 \
        gcc-4.8 \
        gcc-4.8-multilib \
        g++-4.8 \
        g++-4.8-multilib \
    && update-alternatives \
        --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 40 \
    && update-alternatives \
        --install /usr/bin/g++ g++ /usr/bin/g++-4.8 40 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]
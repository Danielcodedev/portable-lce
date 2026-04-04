FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y software-properties-common \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y \
        build-essential \
        gcc-15 g++-15 \
        python3 \
        ninja-build \
        libsdl2-dev \
        libgl-dev \
        libglm-dev \
        libglu1-mesa-dev \
        libpthread-stubs0-dev \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 100 \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

RUN MESON_VERSION=1.10.2 \
    && wget -qO meson.tar.gz https://github.com/mesonbuild/meson/releases/latest/download/meson-${MESON_VERSION}.tar.gz \
    && mkdir /opt/meson \
    && tar xf meson.tar.gz --strip-components=1 -C /opt/meson \
    && mv /opt/meson/meson.py /opt/meson/meson \
    && ln -s /opt/meson/meson /usr/bin/meson \
    && rm -rf meson.tar.gz

ENV DEBIAN_FRONTEND=dialog

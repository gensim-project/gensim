FROM fedora:26

RUN dnf -y update && dnf -y install \
    autoconf \
    cmake \
    automake \
    antlr3-C-devel \
    findutils \
    gcc-arm-linux-gnu \
    gcc-c++ \
    java-1.8.0-openjdk-headless \
    mercurial \
    make \
    ncurses-devel \
    openssh-clients \
    wget \
    zlib-devel \
    \
    && dnf clean all


FROM ubuntu:14.04

RUN apt-get update && apt-get -y install \
    autoconf \
    cmake \
    libantlr3c-dev \
    gcc-arm-linux-gnueabi \
    g++ \
    libncurses5-dev \
    make \
    mercurial \
    openjdk-7-jre-headless \
    wget \
    zlib1g-dev

RUN apt-get update && \
	apt-get -y install \
    curl \
    xz-utils \
    software-properties-common \
    && \
    \
    curl -SL http://releases.llvm.org/6.0.0/clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04.tar.xz | tar -xJC . && \
    mv clang+llvm-6.0.0-x86_64-linux-gnu-ubuntu-14.04 clang_6.0.0 && \
    \
    curl -SL https://cmake.org/files/v3.11/cmake-3.11.4-Linux-x86_64.sh > cmake.sh && \
    mkdir /cmake && \
    sh cmake.sh --prefix=/cmake --skip-license

RUN \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update -y && \
    apt-get install -y gcc-5 g++-5
    

ENV PATH=/cmake/bin:/clang_6.0.0/bin:${PATH}
ENV LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/cmake/lib:/clang_6.0.0/lib
ENV CC=clang
ENV CXX=clang++

FROM ubuntu:22.04 as build
WORKDIR /build

RUN apt-get update
RUN apt-get install -yy build-essential cmake gcc-arm-none-eabi libantlr3c-dev wget ncurses-dev default-jre
COPY . .

WORKDIR /build/build
RUN cmake ..
RUN make CMAKE_BUILD_TYPE=RELEASE -j8

ENTRYPOINT [ "gensim" ]

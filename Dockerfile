FROM ubuntu:22.04 as build
WORKDIR /build

RUN apt-get update
RUN apt-get install -yy build-essential cmake gcc-arm-none-eabi libantlr3c-dev wget ncurses-dev default-jre
COPY . .

WORKDIR /build/build
RUN cmake ..
RUN make CMAKE_BUILD_TYPE=RELEASE -j8

FROM ubuntu:22.04
WORKDIR /workdir

COPY --from=build /lib/x86_64-linux-gnu/libantlr3c-3.4.so.0 /lib/x86_64-linux-gnu/libantlr3c-3.4.so.0
COPY --from=build /build/build/dist/bin/* /usr/local/bin/
COPY --from=build /build/build/dist/lib /build/build/dist/lib

ENTRYPOINT [ "gensim" ]

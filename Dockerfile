FROM ubuntu:22.04 as build
WORKDIR /workdir

RUN apt-get update
RUN apt-get install -yy build-essential cmake gcc-arm-none-eabi libantlr3c-dev wget ncurses-dev default-jre
COPY . .

WORKDIR /workdir/build
RUN cmake ..
RUN make CMAKE_BUILD_TYPE=RELEASE -j8

FROM ubuntu:22.04
WORKDIR /gensim

COPY --from=build /lib/x86_64-linux-gnu/libantlr3c-3.4.so.0 /lib/x86_64-linux-gnu/libantlr3c-3.4.so.0
COPY --from=build /workdir/build/dist/bin/* /gensim/
COPY --from=build /workdir/build/dist/lib /workdir/build/dist/lib

ENTRYPOINT [ "./gensim" ]

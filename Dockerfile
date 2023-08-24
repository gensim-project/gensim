FROM ubuntu:latest
WORKDIR /gensim

RUN apt-get update
RUN apt-get install -yy build-essential cmake gcc-arm-none-eabi libantlr3c-dev wget ncurses-dev default-jre
COPY . .

WORKDIR /gensim/build
RUN cmake ..
RUN make CMAKE_BUILD_TYPE=RELEASE -j8

ENTRYPOINT [ "/gensim/build/dist/bin/gensim" ]

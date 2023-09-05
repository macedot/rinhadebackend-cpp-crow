FROM alpine:latest

RUN apk update && apk upgrade && apk add --no-cache \
    g++ \
    git \
    make \
    cmake \
    boost-dev

ADD . /service

WORKDIR /service/utility
RUN ./install-crow.sh

WORKDIR /service/build
RUN cmake .. && \
    make

EXPOSE 3000 3000

ENTRYPOINT ["./cpp_crown-exe"]

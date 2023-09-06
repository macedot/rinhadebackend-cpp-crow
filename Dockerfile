# FROM alpine:latest
# RUN apk update && apk upgrade \
#     && apk add --no-cache build-base curl zip unzip tar curl-dev \
#     && apk add --no-cache pkgconfig g++ wget make ninja gcc cmake git

FROM debian:latest as build
RUN apt update && apt upgrade -y && apt install -y \
    curl zip unzip tar autoconf \
    pkg-config g++ wget make ninja-build gcc cmake git

ADD . /service
WORKDIR /service

RUN git clone https://github.com/microsoft/vcpkg.git
RUN ./vcpkg/bootstrap-vcpkg.sh
RUN ./vcpkg/vcpkg install

WORKDIR /service/build
RUN cmake ..  -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/service/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    make

FROM alpine:latest
WORKDIR /service
COPY --from=build /service/build/api-cpp-crow-exe /service
EXPOSE 3000 3000
ENTRYPOINT ["/service/api-cpp-crow-exe"]

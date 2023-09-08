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

FROM debian:12-slim
WORKDIR /service
COPY --from=build /service/build/api-cpp-crow-exe /service
COPY --from=build /service/db.string /service
EXPOSE 3000 3000
ENTRYPOINT ["/service/api-cpp-crow-exe"]

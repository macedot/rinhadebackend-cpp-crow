# Base Image (gblic compatible)
FROM ubuntu:latest as build

# Tools do to business
RUN apt update && apt upgrade -y && apt install -y \
    curl zip unzip tar autoconf pkg-config g++ \
    make ninja-build gcc cmake git

# Base bath
ADD . /service
WORKDIR /service

# vcpkg for all other dependencies
ENV VCPKG_FORCE_SYSTEM_BINARIES=1
RUN git clone https://github.com/microsoft/vcpkg.git
RUN ./vcpkg/bootstrap-vcpkg.sh
RUN ./vcpkg/vcpkg install

# CrowCpp from git (latest release still have boost dependency)
RUN git clone https://github.com/CrowCpp/Crow.git && \
    cd Crow && \
    mkdir build && \
    cd build && \
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF -DCMAKE_TOOLCHAIN_FILE=/service/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    make install

# build the app
WORKDIR /service/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/service/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    make

# copy to release image
FROM debian:12-slim
WORKDIR /service
COPY --from=build /service/build/api-cpp-crow-exe /service
COPY --from=build /service/db.string /service
EXPOSE 3000 3000
ENTRYPOINT ["/service/api-cpp-crow-exe"]

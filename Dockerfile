FROM ubuntu:latest as build

RUN apt update && apt upgrade -y && apt install -y \
    zip unzip tar autoconf pkg-config g++ make ninja-build gcc cmake git \
    libpq-dev libasio-dev

ADD . /service
WORKDIR /service

RUN git clone https://github.com/CrowCpp/Crow.git
RUN cd Crow && \
    mkdir build && \
    cd build && \
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF && \
    make install

WORKDIR /service/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make

FROM debian:12-slim
RUN apt update && apt upgrade -y && apt install -y \
    libpq-dev

WORKDIR /service
COPY --from=build /service/build/api-cpp-crow-exe /service
COPY --from=build /service/db.string /service

EXPOSE 3000 3000

ENTRYPOINT ["/service/api-cpp-crow-exe"]

# Base Image (gblic compatible)
FROM ubuntu:latest as build

RUN apt update && sudo apt upgrade -y && apt install -y \
    curl zip unzip tar autoconf pkg-config g++ \
    make ninja-build gcc cmake git

# Base bath
ADD . /service
WORKDIR /service

RUN ./script/setup.sh
RUN ./script/build.sh

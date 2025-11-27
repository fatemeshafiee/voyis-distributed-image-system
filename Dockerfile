FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libopencv-dev \
    libsqlite3-dev \
    libzmq3-dev \
    nlohmann-json3-dev \
    pkg-config \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
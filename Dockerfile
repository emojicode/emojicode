FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

# Set locale to en_US.UTF-8
RUN apt-get update && apt-get install -y locales && \
    locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

# LLVM toolchain
RUN apt-get update && apt-get install -y wget gnupg && \
    echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-6.0 main" >> /etc/apt/sources.list && \
    echo "deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-6.0 main" >> /etc/apt/sources.list && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

# Dependencies
RUN apt-get update && apt-get install -y \
    clang-6.0 \
    cmake \
    libclang-common-6.0-dev \
    libclang1-6.0 \
    libclang-6.0-dev \
    libfuzzer-6.0-dev \
    libllvm6.0 \
    libncurses-dev \
    libz-dev \
    llvm-6.0 \
    llvm-6.0-dev \
    llvm-6.0-runtime \
    ninja-build \
    python3 \
    rsync

# Copy source
RUN mkdir -p /usr/src/emojicode
WORKDIR /usr/src/emojicode
COPY . .

# Build
RUN mkdir build
WORKDIR build
ENV CC clang-6.0
ENV CXX clang++-6.0
ENV PYTHONIOENCODING utf-8
RUN cmake .. -GNinja && ninja && ninja magicinstall

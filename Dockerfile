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
    echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main" >> /etc/apt/sources.list && \
    echo "deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-8 main" >> /etc/apt/sources.list && \
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

# Dependencies
RUN apt-get update && apt-get install -y \
    clang-8 \
    cmake \
    libclang-common-8-dev \
    libclang1-8 \
    libclang-8-dev \
    libfuzzer-8-dev \
    libllvm8 \
    libncurses-dev \
    libz-dev \
    llvm-8 \
    llvm-8-dev \
    llvm-8-runtime \
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
ENV CC clang-8
ENV CXX clang++-8
ENV PYTHONIOENCODING utf-8
RUN cmake .. -GNinja && ninja && ninja magicinstall

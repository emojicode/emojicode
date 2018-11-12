FROM ubuntu:18.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y wget gnupg

RUN echo "deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-6.0 main" >> /etc/apt/sources.list
RUN echo "deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-6.0 main" >> /etc/apt/sources.list
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -

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

RUN mkdir -p /usr/src/emojicode
WORKDIR /usr/src/emojicode
COPY . .

ENV CC clang-6.0
ENV CXX clang++-6.0
ENV PYTHONIOENCODING utf8

RUN cmake . -GNinja && ninja && ninja magicinstall

CMD ["/bin/bash"]

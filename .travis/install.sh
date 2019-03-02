if [[ "$TRAVIS_OS_NAME" != "osx" ]]; then
  wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
  echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" >> /etc/apt/sources.list
  echo "deb-src http://apt.llvm.org/xenial/ llvm-toolchain-xenial-6.0 main" >> /etc/apt/sources.list
  sudo apt-get update
  sudo apt-get install -y cmake libllvm7 libz-dev llvm-7 llvm-7-dev llvm-7-runtime ninja-build python3 rsync

  if [ $CC = "clang" ]
  then
  	# Clang 6
  	sudo apt-get install -y clang-7 libclang-common-7-dev libclang1-7 libclang-7-dev
  	export CC=clang-6.0
  	export CXX=clang++-6.0
  else
  	# GCC 7
  	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
  	sudo apt-get update
  	sudo apt-get install -y gcc-7 g++-7
  	export CC=gcc-7
  	export CXX=g++-7
  fi
else
  export LLVM_DIR=/usr/local/opt/llvm@6/lib/cmake
  export CC=$CC_FOR_BUILD
  export CXX=$CXX_FOR_BUILD
fi

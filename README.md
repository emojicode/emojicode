# Emojicode [![Build Status](https://travis-ci.org/emojicode/emojicode.svg?branch=master)](https://travis-ci.org/emojicode/emojicode) [![Join the chat at https://gitter.im/emojicode/emojicode](https://badges.gitter.im/emojicode/emojicode.svg)](https://gitter.im/emojicode/emojicode?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Emojicode is an open source, high-level, multi-paradigm
programming language consisting of emojis. It features Object-Orientation, Optionals, Generics and Closures.

## Getting Started

To learn more about the language and getting started visit
http://www.emojicode.org/docs.

## Installing

You can easily install Emojicode from our stable prebuilt binaries:
http://www.emojicode.org/docs/guides/install.html.

## Staying up to date

Follow Emojicode’s Twitter account
[@Real_Emojicode](https://twitter.com/Real_Emojicode).

## Contributions

Contributions are welcome! A contribution guideline will be setup soon.

## Building from source

If you need to build Emojicode from source you can of course also do this. **We, however, recommend you to install Emojicode from the prebuilt binaries if possible.**

Prerequisites:
- clang and clang++ 3.4 or newer, or
- gcc and g++ 4.8 or newer
- GNU Make
- SDL2 (libsdl2-dev) to compile the SDL package
  - `sudo apt-get install libsdl2-dev` on Debian/Ubuntu
  - `brew install SDL2` on OS X

Steps:

1. Clone Emojicode (or download the source code and extract it) and navigate into it:

   ```
   git clone https://github.com/emojicode/emojicode
   cd emojicode
   ```

    Beware of, that the master branch contains development code which probably contains bugs. If you want to build the latest stable release make sure to check it out first: `git checkout v0.2.0`

2.  Then simply run

  ```
  make
  ```

  to compile the Engine, the compiler and all default packages.

  You may need to use a smaller heap size on older Raspberry Pis. You can
  specify the heap size in bytes when compiling the engine:

  ```
  make HEAP_SIZE=128000000
  ```

  The default heap size is 512MB. It’s also possible to change the default
  package search path by setting `DEFAULT_PACKAGES_DIRECTORY`. As for example:

  ```
  make DEFAULT_PACKAGES_DIRECTORY=/opt/strange/place
  ```

3. You can now either install Emojicode and run the tests:

   ```
   [sudo] make install && make tests
   ```

   or package the binaries for distribution:

   ```
   make dist
   ```

  After the command is done you will find a directory and a tarfile
in `builds` named after your platform, e.g. `Emojicode-0.2.0-x86_64-linux-gnu`.

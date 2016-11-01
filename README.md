# Emojicode [![Build Status](https://travis-ci.org/emojicode/emojicode.svg?branch=master)](https://travis-ci.org/emojicode/emojicode) [![Join the chat at https://gitter.im/emojicode/emojicode](https://badges.gitter.im/emojicode/emojicode.svg)](https://gitter.im/emojicode/emojicode?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Emojicode is an open source, high-level, multi-paradigm
programming language consisting of emojis. It features Object-Orientation, Optionals, Generics and Closures.

## 🏁 Getting Started

To learn more about the language and get started quickly visit Emojicode’s
[documentation](http://www.emojicode.org/docs).

You can easily install Emojicode from our stable prebuilt binaries.
[See Installing Emojicode](http://www.emojicode.org/docs/guides/install.html)
for instructions.

## 🗞 Staying up to date

Follow Emojicode’s Twitter account
[@Real_Emojicode](https://twitter.com/Real_Emojicode).

## 🔨 Building from source

If you need to build Emojicode from source you can of course also do this. **We, however, recommend you to install Emojicode from the prebuilt binaries if possible.**

Prerequisites:
- clang and clang++ 3.4 or newer, or
- gcc and g++ 4.9 or newer
- GNU Make
- Allegro 5 to compile the allegro package
  - `sudo apt-get install liballegro5-dev` on Debian/Ubuntu
  - `brew install allegro` on OS X

Steps:

1. Clone Emojicode (or download the source code and extract it) and navigate into it:

   ```
   git clone https://github.com/emojicode/emojicode
   cd emojicode
   ```

    Beware of, that the master branch contains development code which probably contains bugs. If you want to build the latest stable release make sure to check it out first: `git checkout v0.3-beta.2`

2.  Then simply run

  ```
  make
  ```

  to compile the Engine, the compiler and all default packages.

  You may need to use a smaller heap size on older Raspberry Pis, for example.
  You can specify the heap size in bytes when compiling the engine:

  ```
  make HEAP_SIZE=128000000
  ```

  The default heap size is 512MB. It’s also possible to change the default
  package search path by setting `DEFAULT_PACKAGES_DIRECTORY`. As for example:

  ```
  make DEFAULT_PACKAGES_DIRECTORY=/opt/strange/place
  ```

3. Run the tests:

   ```
   make tests
   ```

4. You can now either install Emojicode:

   ```
   make install
   ```

   or copy the binaries from `builds/`.

## 📝 Contributions

Want to improve something? Great! First of all, please be nice and helpful.
You can help in lots of ways, like reporting bugs, fixing bugs, improving the
documentation, suggesting new features, or implementing new features.

Whatever you want to do, please look for an existing issue or create a new one
to discuss your plans briefly.

## 📃 License

Emojicode [is licensed under the Artistic License 2.0](LICENSE).
If you don’t want to read the whole license, here’s a summary without legal force:

- You are allowed to download, use, copy, publish and distribute Emojicode.
- You are allowed to create modified versions of Emojicode but you may only distribute them on some conditions.
-  The license contains a grant of patent rights and does not allow you to use any trademark, service mark, tradename, or logo.
- Emojicode comes with absolutely no warranty.

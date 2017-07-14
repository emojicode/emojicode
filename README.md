# Emojicode [ ![Codeship Status for emojicode/emojicode](https://app.codeship.com/projects/edbc3220-f394-0134-fad2-66135ababc06/status?branch=master)](https://app.codeship.com/projects/209932) [![Join the chat at https://gitter.im/emojicode/emojicode](https://badges.gitter.im/emojicode/emojicode.svg)](https://gitter.im/emojicode/emojicode?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

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

If you don’t want to use the prebuilt binaries or want to work on Emojicode, you
can of course also build Emojicode from source.

**These build instructions only apply to the latest code in the master branch.
To build previous versions, please consult the according README.md by cloning
the repository and checking out a version: `git checkout v0.3`**

### Building locally

Prerequisites (earlier versions might work but are not tested):

- clang and clang++ 3.9 or newer, or
- gcc and g++ 5.4 or newer
- CMake 3.5.1 or newer and (preferably) Ninja
- Python 3.5.2 or newer (for tests and packaging)
- Allegro 5 to compile the allegro package
  - `sudo apt-get install liballegro5-dev` on Debian/Ubuntu
  - `brew install allegro` on macOS

Steps:

1. Clone Emojicode (or download the source code and extract it) and navigate
  into it:

   ```
   git clone https://github.com/emojicode/emojicode
   cd emojicode
   ```

2. Create a `build` directory and run CMake in it:

   ```
   mkdir build
   cd build
   cmake .. -GNinja
   ```

   You can specify the heap size in bytes, which defaults to 512MB, with
   `-DheapSize` and the default package search path with
   `-DdefaultPackagesDirectory`.

   You can of course also run CMake in another directory or use another build
   system than Ninja. Refer to the CMake documentation for more information.

3. Build the Compiler and Real-Time Engine:

   ```
   ninja
   ```

4. You can now test Emojicode:

   ```
   ninja tests
   ```

5. The binaries are ready for use!
   You can the perform a magic installation right away

   ```
   ninja magicinstall
   ```

   or just package the binaries and headers properly

   ```
   ninja dist
   ```

   To create a distribution archive you must call the dist script yourself
   (e.g. `python3 ../dist.py .. archive`).

### Building in Docker container

This repository comes with two Dockerfiles `DockerfileClang` and
`DockerfileGCC`, which are used by our continuous integration service. However,
you can also use them to build Emojicode.

Prerequisites:
- Docker

Steps:

1. Clone Emojicode (or download the source code and extract it) and navigate
  into it:

   ```
   git clone https://github.com/emojicode/emojicode
   cd emojicode
   ```
2. Build and create a container

   ```
   docker build -f DockerfileClang -t emojicode_clang .
   docker create --name emojicontainer -ti emojicode_clang bash
   ```

3. Start and enter the container:

   ```
   docker start -ai emojicontainer
   ```

4. You can now build Emojicode:

   ```
   mkdir build && cd build && cmake .. -GNinja && ninja && ninja tests
   ```


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

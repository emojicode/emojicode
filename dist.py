import platform
import shutil
import os
import sys
import subprocess

version = "1.0-beta.1"
packages = ["s", "files", "sockets", "testtube", "json"]

source = os.path.dirname(os.path.realpath(__file__))
dist_name = "Emojicode-{0}-{1}-{2}".format(version, platform.system(),
                                           platform.machine())
path = os.path.abspath(dist_name)


def copy_packages(destination, source):
    for package in packages:
        dir_path = os.path.join(destination, package)
        make_dir(dir_path)
        shutil.copy2(os.path.join(package, "🏛"), dir_path)
        shutil.copy2(os.path.join(package, "lib" + package + ".a"), dir_path)


def make_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)


def copy_header(*args):
    make_dir(os.path.join(path, "include", *args[:-1]))
    shutil.copy2(os.path.join(source, *args),
                 os.path.join(path, "include", *args))

if __name__ == "__main__":
    make_dir(path)

    shutil.copy2(os.path.join(source, "install.sh"), path)
    shutil.copy2(os.path.join("Compiler", "emojicodec"), path)

    copy_header("runtime", "Runtime.h")
    copy_header("s", "Data.h")
    copy_header("s", "String.h")

    dir_path = os.path.join(path, "packages", "runtime")
    make_dir(dir_path)
    shutil.copy2(os.path.join("runtime", "libruntime.a"), dir_path)

    copy_packages(os.path.join(path, "packages"),
                  os.path.join(source, "headers"))

    if len(sys.argv) > 1 and sys.argv[1] == "archive":
        archive_name = shutil.make_archive(dist_name, "gztar", os.getcwd(), dist_name)
        print(archive_name, end='')

    if len(sys.argv) > 1 and sys.argv[1] == "install":
        bash = "cd " + path + " && yes | " + os.path.join(path, "install.sh")
        subprocess.run(["bash", "-c", bash])

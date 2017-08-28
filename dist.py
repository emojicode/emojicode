import platform
import shutil
import os
import sys
import subprocess

source = sys.argv[1]
version = "0.5.1"
packages = [("files", 0), ("sockets", 0), ("allegro", 0)]
dist_name = "Emojicode-{0}-{1}-{2}".format(version, platform.system(),
                                           platform.machine())
path = os.path.abspath(dist_name)


def copy_packages(destination, source, copy_binaries=True):
    make_dir(os.path.join(destination, "s"))
    shutil.copy2(os.path.join(source, "s.emojic"),
                 os.path.join(destination, "s", "header.emojic"))

    for package, version in packages:
        dir_path = os.path.join(destination, package)
        make_dir(dir_path)
        shutil.copy2(os.path.join(source, package + ".emojic"),
                     os.path.join(dir_path, "header.emojic"))
        if copy_binaries:
            shutil.copy2(package + ".so",
                         os.path.join(dir_path, package + ".so"))

        symlink_name = os.path.join(destination,
                                    "{0}-v{1}".format(package, version))
        if os.path.exists(symlink_name):
            os.unlink(symlink_name)
        os.symlink(package, symlink_name)


def make_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

if __name__ == "__main__":
    make_dir(path)

    shutil.copy2(os.path.join(source, "install.sh"),
                 os.path.join(path, "install.sh"))
    shutil.copy2("emojicode", os.path.join(path, "emojicode"))
    shutil.copy2("emojicodec", os.path.join(path, "emojicodec"))

    copy_packages(os.path.join(path, "packages"),
                  os.path.join(source, "headers"))
    copy_packages(os.path.join(path, "packages", "migration"),
                  os.path.join(source, "headers", "migheaders"),
                  copy_binaries=False)

    if len(sys.argv) > 2 and sys.argv[2] == "archive":
        shutil.make_archive(dist_name, "gztar", os.getcwd(), dist_name)
    if len(sys.argv) > 2 and sys.argv[2] == "install":
        bash = "cd " + path + " && yes | " + os.path.join(path, "install.sh")
        subprocess.run(["bash", "-c", bash])

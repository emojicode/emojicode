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


def make_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

if __name__ == "__main__":
    make_dir(path)

    shutil.copy2(os.path.join(source, "install.sh"),
                 os.path.join(path, "install.sh"))
    make_dir(os.path.join(path, "packages", "s"))
    shutil.copy2(os.path.join(source, "headers", "s.emojic"),
                 os.path.join(path, "packages", "s", "header.emojic"))

    for package, version in packages:
        dir_path = os.path.join(path, "packages", package)
        make_dir(dir_path)
        shutil.copy2(os.path.join(source, "headers", package + ".emojic"),
                     os.path.join(dir_path, "header.emojic"))
        shutil.copy2(package + ".so", os.path.join(dir_path, package + ".so"))

        symlink_name = os.path.join(path, "packages",
                                    "{0}-v{1}".format(package, version))
        if os.path.exists(symlink_name):
            os.unlink(symlink_name)
        os.symlink(package, symlink_name)

    shutil.copy2("emojicode", os.path.join(path, "emojicode"))
    shutil.copy2("emojicodec", os.path.join(path, "emojicodec"))

    if len(sys.argv) > 2 and sys.argv[2] == "archive":
        shutil.make_archive(dist_name, "gztar", os.getcwd(), dist_name)
    if len(sys.argv) > 2 and sys.argv[2] == "install":
        bash = "cd " + path + " && yes | " + os.path.join(path, "install.sh")
        subprocess.run(["bash", "-c", bash])

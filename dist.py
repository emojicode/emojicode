import platform
import shutil
import os
import sys

source = sys.argv[1]
version = "0.5.0-dev"
packages = [("files", 0)]
dist_name = "Emojicode-{0}-{1}-{2}".format(version, platform.system(),
                                           platform.machine())
path = dist_name


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
                     os.path.join(path, "packages", package, "header.emojic"))
        symlink_name = os.path.join(path, "packages",
                                    "{0}-v{1}".format(package, version))
        if os.path.exists(symlink_name):
            os.unlink(symlink_name)
        os.symlink(package, symlink_name)
        # TODO: Copy binaries

    shutil.copy2("emojicode", os.path.join(path, "emojicode"))
    shutil.copy2("emojicodec", os.path.join(path, "emojicodec"))
    shutil.make_archive(dist_name, "gztar", path)

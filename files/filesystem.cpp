//
// Created by Theo Weidmann on 24.03.18.
//

#include "../s/String.h"
#include "../s/Error.h"
#include "../runtime/Runtime.h"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>

namespace files {

using s::String;

extern "C" void filesFsMakeDir(String *path, runtime::Raiser *raiser) {
    EJC_COND_RAISE_IO_VOID(mkdir(path->stdString().c_str(), 0755) == 0, raiser)
}

extern "C" void filesFsDelete(String *path, runtime::Raiser *raiser) {
    EJC_COND_RAISE_IO_VOID(remove(path->stdString().c_str()) == 0, raiser)
}

extern "C" void filesFsDeleteDir(String *path, runtime::Raiser *raiser) {
    EJC_COND_RAISE_IO_VOID(rmdir(path->stdString().c_str()) == 0, raiser)
}

extern "C" void filesFsSymlink(String *org, String *destination, runtime::Raiser *raiser) {
    EJC_COND_RAISE_IO_VOID(symlink(org->stdString().c_str(), destination->stdString().c_str()) == 0, raiser);
}

int filesRecursiveRmdirHelper(const char *fpath, const struct stat * /*sb*/, int typeflag, struct FTW * /*ftwbuf*/){
    int state;
    if (typeflag == FTW_F || typeflag == FTW_SL || typeflag == FTW_SLN) {
        state = remove(fpath);
    }
    else if (typeflag == FTW_DP) {
        state = rmdir(fpath);
    }
    else {
        state = -4;
    }
    return state;
}

extern "C" void filesFsRecursiveDeleteDir(String *path, runtime::Raiser *raiser) {
    EJC_COND_RAISE_IO_VOID(nftw(path->stdString().c_str(), filesRecursiveRmdirHelper, 64, FTW_DEPTH | FTW_PHYS) == 0,
                           raiser)
}

extern "C" runtime::Boolean filesFsExists(String *path) {
    return access(path->stdString().c_str(), F_OK) == 0;
}

extern "C" runtime::Boolean filesFsReadable(String *path) {
    return access(path->stdString().c_str(), R_OK) == 0;
}

extern "C" runtime::Boolean filesFsExecutable(String *path) {
    return access(path->stdString().c_str(), X_OK) == 0;
}

extern "C" runtime::Boolean filesFsWriteable(String *path) {
    return access(path->stdString().c_str(), W_OK) == 0;
}

extern "C" runtime::Integer filesFsSize(String *path, runtime::Raiser *raiser) {
    struct stat st{};
    int state = stat(path->stdString().c_str(), &st);
    if (state != 0) {
        EJC_RAISE(raiser, s::IOError::init());
    }
    return st.st_size;
}

extern "C" String* filesFsAbsolute(String *inPath, runtime::Raiser *raiser) {
    char path[PATH_MAX];
    char *x = realpath(inPath->stdString().c_str(), path);
    if (x == nullptr) {
        EJC_RAISE(raiser, s::IOError::init());
    }
    return String::init(x);
}

}  // namespace files

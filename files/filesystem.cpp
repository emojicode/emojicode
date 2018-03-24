//
// Created by Theo Weidmann on 24.03.18.
//

#include "../s/String.hpp"
#include "../runtime/Runtime.h"
#include <unistd.h>
#include <sys/stat.h>
#include <climits>
#include <ftw.h>
#include <cerrno>
#include <cstdlib>

namespace files {

using s::String;

runtime::Enum errorEnumFromErrno() {
    switch (errno) {
        case EACCES:
            return 1;
        case EEXIST:
            return 2;
        case ENOMEM:
            return 3;
        case ENOSYS:
            return 4;
        case EDOM:
            return 5;
        case EINVAL:
            return 6;
        case EILSEQ:
            return 7;
        case ERANGE:
            return 8;
        case EPERM:
            return 9;
        default:
            return 0;
    }
}

runtime::SimpleOptional<runtime::Enum> returnOptional(bool success) {
    if (success) {
        return runtime::NoValue;
    }
    return errorEnumFromErrno();
}

extern "C" runtime::SimpleOptional<runtime::Enum> filesFsMakeDir(String *path) {
    return returnOptional(mkdir(path->cString(), 0755) == 0);
}

extern "C" runtime::SimpleOptional<runtime::Enum> filesFsDelete(String *path) {
    return returnOptional(remove(path->cString()) == 0);
}

extern "C" runtime::SimpleOptional<runtime::Enum> filesFsDeleteDir(String *path) {
    return returnOptional(rmdir(path->cString()) == 0);
}

extern "C" runtime::SimpleOptional<runtime::Enum> filesFsSymlink(String *org, String *destination) {
    return returnOptional(symlink(org->cString(), destination->cString()) == 0);
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

extern "C" runtime::SimpleOptional<runtime::Enum> filesFsRecursiveDeleteDir(String *path) {
    return returnOptional(nftw(path->cString(), filesRecursiveRmdirHelper, 64, FTW_DEPTH | FTW_PHYS) == 0);
}

extern "C" runtime::Boolean filesFsExists(String *path) {
    return access(path->cString(), F_OK) == 0;
}

extern "C" runtime::Boolean filesFsReadable(String *path) {
    return access(path->cString(), R_OK) == 0;
}

extern "C" runtime::Boolean filesFsExecutable(String *path) {
    return access(path->cString(), X_OK) == 0;
}

extern "C" runtime::Boolean filesFsWriteable(String *path) {
    return access(path->cString(), W_OK) == 0;
}

extern "C" runtime::SimpleError<runtime::Integer> filesFsSize(String *path) {
    struct stat st{};
    int state = stat(path->cString(), &st);
    if (state != 0) {
        return runtime::SimpleError<runtime::Integer>(runtime::MakeError, errorEnumFromErrno());
    }
    return st.st_size;
}

extern "C" runtime::SimpleError<String*> filesFsAbsolute(String *inPath) {
    char path[PATH_MAX];
    char *x = realpath(inPath->cString(), path);
    if (x == nullptr) {
        return runtime::SimpleError<String*>(runtime::MakeError, errorEnumFromErrno());
    }
    return String::fromCString(x);
}

}  // namespace files
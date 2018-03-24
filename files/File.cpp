//
// Created by Theo Weidmann on 24.03.18.
//

#include "../s/String.hpp"
#include "../s/Data.hpp"
#include "../runtime/Runtime.h"
#include <fstream>
#include <ios>
#include <iostream>

using s::String;
using s::Data;

namespace files {

class File : public runtime::Object<File> {
public:
    std::fstream file_;
};

extern "C" runtime::SimpleError<File*> filesFileNewWriting(String *path) {
    auto file = File::allocateAndInitType();
    file->file_ = std::fstream(path->cString(), std::ios_base::out);
    return file;
}

extern "C" runtime::SimpleError<File*> filesFileNewReading(String *path) {
    auto file = File::allocateAndInitType();
    file->file_ = std::fstream(path->cString(), std::ios_base::in);
    return file;
}

extern "C" void filesFileWrite(File *file, Data *data) {
    file->file_.write(reinterpret_cast<char *>(data->data), data->count);
}

extern "C" void filesFileClose(File *file) {
    file->file_.close();
}

extern "C" void filesFileFlush(File *file) {
    file->file_.flush();
}

extern "C" runtime::SimpleError<Data*> filesFileReadBytes(File *file, runtime::Integer count) {
    auto bytes = runtime::allocate<runtime::Byte>(count);
    file->file_.read(reinterpret_cast<char *>(bytes), count);

    auto data = Data::allocateAndInitType();
    data->data = bytes;
    data->count = file->file_.gcount();
    return data;
}

extern "C" void filesFileSeekToEnd(File *file) {
    file->file_.seekp(std::ios_base::end);
}

extern "C" void filesFileSeekTo(File *file, runtime::Integer pos) {
    file->file_.seekp(pos, std::ios_base::beg);
}

extern "C" runtime::SimpleError<Data*> filesFileReadFile(runtime::ClassType, String *path) {
    auto file = std::ifstream(path->cString(), std::ios_base::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto bytes = runtime::allocate<runtime::Byte>(size);
    file.read(reinterpret_cast<char *>(bytes), size);

    auto data = Data::allocateAndInitType();
    data->data = bytes;
    data->count = size;
    return data;
}

extern "C" runtime::SimpleOptional<runtime::Enum> filesFileWriteToFile(runtime::ClassType, String *path, Data *data) {
    auto file = std::ofstream(path->cString(), std::ios_base::out);
    file.write(reinterpret_cast<char *>(data->data), data->count);
    return runtime::NoValue;
}

}  // namespace files

SET_META_FOR(files::File, files, 1f4c4)
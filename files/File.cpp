//
// Created by Theo Weidmann on 24.03.18.
//

#include "../s/String.h"
#include "../runtime/Runtime.h"
#include "../s/Data.h"
#include <fstream>
#include <ios>
#include <iostream>
#include <cerrno>

using s::String;
using s::Data;

namespace files {

runtime::Enum errorEnumFromErrno();

template <typename T, typename F>
runtime::SimpleError<T> returnErrorIfFailed(const T &value, const F &file) {
    if (file.fail()) {
        return { runtime::MakeError, errorEnumFromErrno() };
    }
    return value;
}

class File : public runtime::Object<File> {
public:
    std::fstream file_;
};

extern "C" runtime::SimpleError<File*> filesFileNewWriting(String *path) {
    auto file = File::init();
    file->file_ = std::fstream(path->stdString().c_str(), std::ios_base::out);
    return returnErrorIfFailed(file, file->file_);
}

extern "C" runtime::SimpleError<File*> filesFileNewReading(String *path) {
    auto file = File::init();
    file->file_ = std::fstream(path->stdString().c_str(), std::ios_base::in);
    return returnErrorIfFailed(file, file->file_);
}

extern "C" void filesFileWrite(File *file, Data *data) {
    file->file_.write(reinterpret_cast<char *>(data->data.get()), data->count);
}

extern "C" void filesFileClose(File *file) {
    file->file_.close();
}

extern "C" void filesFileFlush(File *file) {
    file->file_.flush();
}

extern "C" runtime::SimpleError<Data*> filesFileReadBytes(File *file, runtime::Integer count) {
    auto bytes = runtime::allocate<runtime::Byte>(count);
    file->file_.read(reinterpret_cast<char *>(bytes.get()), count);

    auto data = Data::init();
    data->data = bytes;
    data->count = file->file_.gcount();
    return returnErrorIfFailed(data, file->file_);
}

extern "C" void filesFileSeekToEnd(File *file) {
    file->file_.seekp(std::ios_base::end);
}

extern "C" void filesFileSeekTo(File *file, runtime::Integer pos) {
    file->file_.seekp(pos, std::ios_base::beg);
}

extern "C" runtime::SimpleError<Data*> filesFileReadFile(runtime::ClassInfo*, String *path) {
    auto file = std::ifstream(path->stdString().c_str(), std::ios_base::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto bytes = runtime::allocate<runtime::Byte>(size);
    file.read(reinterpret_cast<char *>(bytes.get()), size);

    auto data = Data::init();
    data->data = bytes;
    data->count = size;
    return returnErrorIfFailed(data, file);
}

extern "C" runtime::SimpleOptional<runtime::Enum> filesFileWriteToFile(runtime::ClassInfo*, String *path, Data *data) {
    auto file = std::ofstream(path->stdString().c_str(), std::ios_base::out);
    file.write(reinterpret_cast<char *>(data->data.get()), data->count);
    if (file.fail()) return errorEnumFromErrno();
    return runtime::NoValue;
}

}  // namespace files

SET_INFO_FOR(files::File, files, 1f4c4)

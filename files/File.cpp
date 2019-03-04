//
// Created by Theo Weidmann on 24.03.18.
//

#include "../s/String.h"
#include "../runtime/Runtime.h"
#include "../s/Data.h"
#include "../s/Error.h"
#include <fstream>
#include <ios>
#include <iostream>
#include <cerrno>

using s::String;
using s::Data;

namespace files {

runtime::Enum errorEnumFromErrno();

class File : public runtime::Object<File> {
public:
    std::fstream file_;
};

extern "C" File* filesFileNewWriting(String *path, runtime::Raiser *raiser) {
    auto file = File::init();
    file->file_ = std::fstream(path->stdString().c_str(), std::ios_base::out);
    EJC_COND_RAISE_IO(!file->file_.fail(), raiser);
    return file;
}

extern "C" File* filesFileNewReading(String *path, runtime::Raiser *raiser) {
    auto file = File::init();
    file->file_ = std::fstream(path->stdString().c_str(), std::ios_base::in);
    EJC_COND_RAISE_IO(!file->file_.fail(), raiser);
    return file;
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

extern "C" Data* filesFileReadBytes(File *file, runtime::Integer count, runtime::Raiser *raiser) {
    auto bytes = runtime::allocate<runtime::Byte>(count);
    file->file_.read(reinterpret_cast<char *>(bytes.get()), count);

    auto data = Data::init();
    data->data = bytes;
    data->count = file->file_.gcount();
    EJC_COND_RAISE_IO(!file->file_.fail(), raiser);
    return data;
}

extern "C" void filesFileSeekToEnd(File *file) {
    file->file_.seekp(std::ios_base::end);
}

extern "C" void filesFileSeekTo(File *file, runtime::Integer pos) {
    file->file_.seekp(pos, std::ios_base::beg);
}

extern "C" Data* filesFileReadFile(runtime::ClassInfo*, String *path, runtime::Raiser *raiser) {
    auto file = std::ifstream(path->stdString().c_str(), std::ios_base::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    auto bytes = runtime::allocate<runtime::Byte>(size);
    file.read(reinterpret_cast<char *>(bytes.get()), size);

    auto data = Data::init();
    data->data = bytes;
    data->count = size;
    EJC_COND_RAISE_IO(!file.fail(), raiser);
    return data;
}

extern "C" void filesFileWriteToFile(runtime::ClassInfo*, String *path, Data *data, runtime::Raiser *raiser) {
    auto file = std::ofstream(path->stdString().c_str(), std::ios_base::out);
    file.write(reinterpret_cast<char *>(data->data.get()), data->count);
    EJC_COND_RAISE_IO_VOID(!file.fail(), raiser);
}

}  // namespace files

SET_INFO_FOR(files::File, files, 1f4c4)

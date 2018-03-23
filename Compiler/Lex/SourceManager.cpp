//
// Created by Theo Weidmann on 22.03.18.
//

#include "CompilerError.hpp"
#include "SourceManager.hpp"
#include "SourcePosition.hpp"
#include <fstream>
#include <locale>
#include <codecvt>
#include <algorithm>

namespace EmojicodeCompiler {

SourceManager::File* SourceManager::read(std::string file) {
    std::ifstream f(file, std::ios_base::binary | std::ios_base::in);
    if (f.fail()) {
        throw CompilerError(SourcePosition(0, 0, file), "Couldn't read input file ", file, ".");
    }

    auto string = std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

    auto cache = std::make_unique<File>(conv.from_bytes(string));
    auto ptr = cache.get();
    cache_.emplace(file, std::move(cache));
    return ptr;
}

std::u32string SourceManager::line(const SourcePosition &pos) {
    auto it = cache_.find(pos.file);
    if (it == cache_.end()) {
        return std::u32string();
    }

    auto &file = it->second;
    auto begin = file->lines()[pos.line - 1];
    auto length = pos.line < file->lines().size() ? file->lines()[pos.line] - begin
                                                  : file->file().find_first_of('\n', begin) + 1 - begin;
    return file->file().substr(begin, length);
}

}  // namespace EmojicodeCompiler
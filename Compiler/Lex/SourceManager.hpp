//
// Created by Theo Weidmann on 22.03.18.
//

#ifndef EMOJICODE_SOURCEMANAGER_HPP
#define EMOJICODE_SOURCEMANAGER_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include "Token.hpp"

namespace EmojicodeCompiler {

struct SourcePosition;

class SourceFile {
public:
    explicit SourceFile(std::u32string file, std::string path) : content_(std::move(file)), path_(path) {}
    const std::u32string& file() const { return content_; }
    const std::string& path() const { return path_; }

    void endLine(size_t end) { lines_.emplace_back(end); }
    const std::vector<size_t>& lines() const { return lines_; }

    void addComment(Token &&token) {
        comments_.emplace(std::make_pair(token.position().line, token.position().character), token);
    }

    void findComments(const SourcePosition &a, const SourcePosition &b,
                      const std::function<void (const Token &)> &comment) const;
private:
    const std::u32string content_;
    const std::string path_;
    std::vector<size_t> lines_ = {0};
    std::map<std::pair<size_t, size_t>, Token> comments_;
};

/// The SourceManager is responsible for reading source files. It caches their content and can provide lines from
/// source files.
class SourceManager {
public:
    /// Reads the file at the provided path.
    /// @param file Path to the source file.
    SourceFile* read(std::string file);

private:
    std::map<std::string, std::unique_ptr<SourceFile>> cache_;
};

}  // namespace EmojicodeCompiler


#endif //EMOJICODE_SOURCEMANAGER_HPP

//
// Created by Theo Weidmann on 22.03.18.
//

#ifndef EMOJICODE_SOURCEMANAGER_HPP
#define EMOJICODE_SOURCEMANAGER_HPP

#include <string>
#include <map>
#include <vector>
#include <memory>

namespace EmojicodeCompiler {

struct SourcePosition;

/// The SourceManager is responsible for reading source files. It caches their content and can provide lines from
/// source files.
class SourceManager {
public:
    class File {
    public:
        explicit File(std::u32string file) : file_(std::move(file)) {}
        const std::u32string& file() const { return file_; }

        void endLine(size_t end) { lines_.emplace_back(end); }
        const std::vector<size_t>& lines() const { return lines_; }
    private:
        const std::u32string file_;
        std::vector<size_t> lines_ = {0};
    };

    /// Reads the file at the provided path.
    /// @param file Path to the source file.
    File* read(std::string file);
    /// @returns The line into which the provided SourcePosition points or an empty string if the line cannot be
    /// returned for whatever reason.
    std::u32string line(const SourcePosition &pos);
private:
    std::map<std::string, std::unique_ptr<File>> cache_;
};

}  // namespace EmojicodeCompiler


#endif //EMOJICODE_SOURCEMANAGER_HPP

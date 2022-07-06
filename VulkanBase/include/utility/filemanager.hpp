#pragma once

#include "common.h"

class FileManager{
public:
    explicit FileManager(std::vector<fs::path> searchPaths = {});

    void addSearchPath(const fs::path& searchPath);

    [[nodiscard]]
    byte_string load(const std::string& resource) const;

    [[nodiscard]]
    std::optional<fs::path> getFullPath(const std::string& resource) const;

private:
    std::vector<fs::path> searchPaths_;
};
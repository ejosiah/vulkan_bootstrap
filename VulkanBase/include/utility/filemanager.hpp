#pragma once

#include "common.h"

class FileManager{
public:
    explicit FileManager(const std::vector<fs::path>&  searchPaths = {});

    void addSearchPath(const fs::path& searchPath);

    byte_string load(const std::string& resource);

    std::optional<fs::path> getFullPath(const std::string& resource);

private:
    std::vector<fs::path> searchPaths_;
};
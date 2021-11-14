#include "utility/filemanager.hpp"

FileManager::FileManager(const std::vector<fs::path>& searchPaths)
:searchPaths_{searchPaths}{

}

void FileManager::addSearchPath(const fs::path &searchPath) {
    searchPaths_.push_back(searchPath);
}

byte_string FileManager::load(const std::string &resource) {
    auto maybePath = getFullPath(resource);
    if(!maybePath.has_value()){
        throw std::runtime_error{fmt::format("resource: {} does not exists", resource)};
    }
    return loadFile(maybePath->string());
}

std::optional<fs::path> FileManager::getFullPath(const std::string &resource) {
    fs::path path = resource;
    auto itr = begin(searchPaths_);
    while(!exists(path) && itr != end(searchPaths_)){
        path = fmt::format("{}/{}", itr->string(), resource);
        itr++;
    }
    return exists(path) ? std::optional{path} : std::nullopt;
}


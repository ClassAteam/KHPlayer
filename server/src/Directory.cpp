#include "Directory.h"
#include <algorithm>
#include <dirent.h>

#include <utility>

const std::vector<std::string> kSupportedExts = {".mp4", ".mkv", ".avi", ".mov", ".ts"};

Directory::Directory(std::string path) : path_(std::move(path)) {}

static bool hasVideoExtension(const std::string& name) {
    for (const auto& ext : kSupportedExts) {
        if (name.size() >= ext.size() &&
            name.compare(name.size() - ext.size(), ext.size(), ext) == 0)
            return true;
    }
    return false;
}
std::vector<std::string> Directory::getFileNames() const {

    std::vector<std::string> files;
    DIR* d = opendir(path_.c_str());
    if (!d)
        return files;
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        if (hasVideoExtension(name))
            files.push_back(name);
    }
    closedir(d);
    std::sort(files.begin(), files.end());
    return files;
}

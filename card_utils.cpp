#include "card_utils.h"
#include <filesystem>
#include <string>
#include <iostream>

namespace fs = std::filesystem;

void duplicate_cards(const char* source_dir, const char* dest_dir, int count) {
    if (!fs::exists(source_dir)) {
        throw std::runtime_error("Source directory does not exist.");
    }

    if (!fs::exists(dest_dir)) {
        fs::create_directories(dest_dir);
    }

    for (const auto& entry : fs::directory_iterator(source_dir)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
                for (int i = 1; i <= count; ++i) {
                    std::string new_filename = entry.path().stem().string() + "-" + std::to_string(i) + entry.path().extension().string();
                    fs::path dest_path = fs::path(dest_dir) / new_filename;
                    try {
                        fs::copy_file(entry.path(), dest_path, fs::copy_options::overwrite_existing);
                    } catch (const fs::filesystem_error& e) {
                        throw std::runtime_error("Failed to copy file " + entry.path().string() + " to " + dest_path.string() + ": " + e.what());
                    }
                }
            }
        }
    }
}

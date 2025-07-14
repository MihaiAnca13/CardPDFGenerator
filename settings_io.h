//
// Created by mihai on 13-7-25.
//

#ifndef SETTINGS_IO_H
#define SETTINGS_IO_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "CardPDFGenerator.h"

// Helper to write a setting to the file
template<typename T>
void write_setting(std::ofstream& ofs, const std::string& key, const T& value) {
    ofs << key << " = " << value << std::endl;
}

// Saves the settings struct to a text file.
inline void save_settings(const CardPDFGenerator::Settings& settings, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: Could not open settings file for writing: " << filename << std::endl;
        return;
    }

    write_setting(ofs, "pageWidth", settings.pageWidth);
    write_setting(ofs, "pageHeight", settings.pageHeight);
    write_setting(ofs, "cardWidth", settings.cardWidth);
    write_setting(ofs, "cardHeight", settings.cardHeight);
    write_setting(ofs, "bleed", settings.bleed);
    write_setting(ofs, "rows", settings.rows);
    write_setting(ofs, "columns", settings.columns);
    write_setting(ofs, "hasBorder", settings.hasBorder);
    write_setting(ofs, "borderWidth", settings.borderWidth);
    write_setting(ofs, "borderColor_r", settings.borderColor.r);
    write_setting(ofs, "borderColor_g", settings.borderColor.g);
    write_setting(ofs, "borderColor_b", settings.borderColor.b);
    write_setting(ofs, "guideLineWidth", settings.guideLineWidth);
    write_setting(ofs, "showGuideLines", settings.showGuideLines);
    write_setting(ofs, "backMode", static_cast<int>(settings.backMode));
}

// Loads settings from a text file into the settings struct.
inline void load_settings(CardPDFGenerator::Settings& settings, const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs) {
        // File doesn't exist, so we'll use defaults and save a new file later.
        return;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            key.erase(key.find_last_not_of(" \t") + 1);
            std::string value_str;
            if (std::getline(iss, value_str)) {
                value_str.erase(0, value_str.find_first_not_of(" \t"));

                if (key == "pageWidth") settings.pageWidth = std::stof(value_str);
                else if (key == "pageHeight") settings.pageHeight = std::stof(value_str);
                else if (key == "cardWidth") settings.cardWidth = std::stof(value_str);
                else if (key == "cardHeight") settings.cardHeight = std::stof(value_str);
                else if (key == "bleed") settings.bleed = std::stof(value_str);
                else if (key == "rows") settings.rows = std::stoi(value_str);
                else if (key == "columns") settings.columns = std::stoi(value_str);
                else if (key == "hasBorder") settings.hasBorder = std::stoi(value_str);
                else if (key == "borderWidth") settings.borderWidth = std::stof(value_str);
                else if (key == "borderColor_r") settings.borderColor.r = std::stof(value_str);
                else if (key == "borderColor_g") settings.borderColor.g = std::stof(value_str);
                else if (key == "borderColor_b") settings.borderColor.b = std::stof(value_str);
                else if (key == "guideLineWidth") settings.guideLineWidth = std::stof(value_str);
                else if (key == "showGuideLines") settings.showGuideLines = std::stoi(value_str);
                else if (key == "backMode") settings.backMode = static_cast<CardPDFGenerator::BackMode>(std::stoi(value_str));
            }
        }
    }
}

#endif //SETTINGS_IO_H

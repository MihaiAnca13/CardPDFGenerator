#include "CardPDFGenerator.h"
#include <iostream>


int main() {
    CardPDFGenerator::Settings settings;
    settings.cardWidth = 63.0f;    // mm
    settings.cardHeight = 88.0f;   // mm
    settings.bleed = 0.f;         // mm
    settings.hasBorder = true;
    settings.borderWidth = 1.0f;   // mm
    settings.borderColor = {0.0f, 0.0f, 0.0f};  // Black
    settings.rows = 3;
    settings.columns = 3;
    settings.showGuideLines = true;
    settings.guideLineWidth = 0.2f;  // mm
    settings.backMode = CardPDFGenerator::BackMode::SameBack;

    try {
        CardPDFGenerator generator(settings);
        generator.generatePDF("output.pdf", "front_images/", "back.jpg");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
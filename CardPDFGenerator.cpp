//
// Created by Mihai on 17/11/2024.
//

#include "CardPDFGenerator.h"


CardPDFGenerator::CardPDFGenerator(const CardPDFGenerator::Settings &settings) : settings_(settings) {
    pdf_ = HPDF_New(error_handler, nullptr);
    if (!pdf_) throw std::runtime_error("Failed to create PDF object");

    HPDF_SetCompressionMode(pdf_, HPDF_COMP_ALL);

    // Validate settings
    validateSettings();
}

CardPDFGenerator::~CardPDFGenerator() {
    if (pdf_) HPDF_Free(pdf_);
}

void CardPDFGenerator::generatePDF(const std::string &outputPath, const std::string &frontImagesPath,
                                   const std::string &backImagesPath) {
    std::vector<fs::path> frontImages = getImageFiles(frontImagesPath);
    std::vector<fs::path> backImages;

    if (settings_.backMode != BackMode::NoBack) {
        if (settings_.backMode == BackMode::SameBack) {
            if (fs::is_regular_file(backImagesPath)) {
                backImages.emplace_back(backImagesPath);
            } else
            {
                throw std::runtime_error("Invalid back image path.");
            }
        } else {
            backImages = getImageFiles(backImagesPath);
            // Validate that we have enough back images
            if (backImages.size() < frontImages.size()) {
                throw std::runtime_error("Not enough back images for unique backs mode");
            }
        }
    }

    size_t currentCard = 0;
    while (currentCard < frontImages.size()) {
        // Track the starting card index for this page
        size_t pageStartIndex = currentCard;

        // Create front page
        HPDF_Page page = HPDF_AddPage(pdf_);
        setupPage(page);
        drawGuideLines(page);  // Add guide lines before drawing cards

        // Add cards to page
        for (int row = 0; row < settings_.rows && currentCard < frontImages.size(); ++row) {
            for (int col = 0; col < settings_.columns && currentCard < frontImages.size(); ++col) {
                addCardToPage(page, frontImages[currentCard], row, col);
                currentCard++;
            }
        }

        // Create back page if needed
        if (settings_.backMode != BackMode::NoBack) {
            HPDF_Page backPage = HPDF_AddPage(pdf_);
            setupPage(backPage);
            drawGuideLines(backPage);  // Add guide lines to back page

            // Reset to start of current page for back images
            size_t backIndex = pageStartIndex;

            for (int row = 0; row < settings_.rows; ++row) {
                for (int col = 0; col < settings_.columns; ++col) {
                    // Only process if we haven't exceeded the number of cards
                    if (backIndex < currentCard) {
                        const auto &backImage = settings_.backMode == BackMode::SameBack ?
                                                backImages[0] : backImages[backIndex];
                        addCardToPage(backPage, backImage, row, col);
                        backIndex++;
                    }
                }
            }
        }
    }

    HPDF_SaveToFile(pdf_, outputPath.c_str());
}

void CardPDFGenerator::validateSettings() const {
    float totalCardWidth = settings_.cardWidth + (2 * settings_.bleed) + (2 * settings_.borderWidth);
    float totalCardHeight = settings_.cardHeight + (2 * settings_.bleed) + (2 * settings_.borderWidth);

    float totalWidth = totalCardWidth * settings_.columns;
    float totalHeight = totalCardHeight * settings_.rows;

    // if more than 1 column, don't count the bleed at each side
    if (settings_.columns > 1) {
        totalWidth -= (2 * settings_.bleed);
    }
    // if more than 1 row, don't count the bleed at the bottom and top
    if (settings_.rows > 1) {
        totalHeight -= (2 * settings_.bleed);
    }

    if (totalWidth > settings_.pageWidth || totalHeight > settings_.pageHeight) {
        throw std::runtime_error("Cards don't fit on page with current settings");
    }
}

void CardPDFGenerator::error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data) {
    throw std::runtime_error("PDF Error: " + std::to_string(error_no) +
                             ", Detail: " + std::to_string(detail_no));
}

std::vector<fs::path> CardPDFGenerator::getImageFiles(const std::string &dirPath) {
    std::vector<fs::path> images;
    for (const auto &entry: fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
                images.push_back(entry.path());
            }
        }
    }
    return images;
}

void CardPDFGenerator::setupPage(HPDF_Page page) const {
    // Convert mm to points (1 point = 1/72 inch, 1 inch = 25.4 mm)
    float pageWidthPt = settings_.pageWidth * 72.0f / 25.4f;
    float pageHeightPt = settings_.pageHeight * 72.0f / 25.4f;

    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);
    HPDF_Page_SetWidth(page, pageWidthPt);
    HPDF_Page_SetHeight(page, pageHeightPt);
}

void CardPDFGenerator::addCardToPage(HPDF_Page page, const fs::path &imagePath, int row, int col) {
    // Convert all measurements to points
    float cardWidthPt = settings_.cardWidth * 72.0f / 25.4f;
    float cardHeightPt = settings_.cardHeight * 72.0f / 25.4f;
    float bleedPt = settings_.bleed * 72.0f / 25.4f;
    float borderPt = settings_.borderWidth * 72.0f / 25.4f;

    // Calculate base position using grid start coordinates
    float baseX = getGridStartX() + (col * getTotalCardWidth());
    float baseY = getGridStartY() - ((row + 1) * getTotalCardHeight());

    // Load image
    HPDF_Image image = nullptr;
    std::string ext = imagePath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (ext == ".jpg" || ext == ".jpeg") {
        image = HPDF_LoadJpegImageFromFile(pdf_, imagePath.string().c_str());
    } else if (ext == ".png") {
        image = HPDF_LoadPngImageFromFile(pdf_, imagePath.string().c_str());
    } else {
        throw std::runtime_error("Unsupported image format: " + imagePath.string());
    }

    if (!image) {
        throw std::runtime_error("Failed to load image: " + imagePath.string());
    }

    // If border is enabled, draw it first
    if (settings_.hasBorder) {
        // Position for border (includes bleed)
        float borderX = baseX + bleedPt + (borderPt / 2);
        float borderY = baseY + bleedPt + (borderPt / 2);
        float borderWidth = cardWidthPt + borderPt;
        float borderHeight = cardHeightPt + borderPt;

        HPDF_Page_SetRGBStroke(page, settings_.borderColor.r, settings_.borderColor.g, settings_.borderColor.b);
        HPDF_Page_SetLineWidth(page, borderPt);
        HPDF_Page_Rectangle(page, borderX, borderY, borderWidth, borderHeight);
        HPDF_Page_Stroke(page);
    }

    // Calculate image position (inside border if it exists)
    float imageX = baseX + bleedPt + borderPt;
    float imageY = baseY + bleedPt + borderPt;

    // Draw the image
    HPDF_Page_DrawImage(page, image, imageX, imageY, cardWidthPt, cardHeightPt);
}

void CardPDFGenerator::drawGuideLines(HPDF_Page page) const {
    if (!settings_.showGuideLines) return;

    float guideLineWidthPt = settings_.guideLineWidth * 72.0f / 25.4f;
    float gridStartX = getGridStartX();
    float gridStartY = getGridStartY();
    float cardWidthPt = getTotalCardWidth();
    float cardHeightPt = getTotalCardHeight();
    float pageWidthPt = settings_.pageWidth * 72.0f / 25.4f;
    float pageHeightPt = settings_.pageHeight * 72.0f / 25.4f;

    HPDF_Page_SetLineWidth(page, guideLineWidthPt);
    HPDF_Page_SetRGBStroke(page, 0.5, 0.5, 0.5);  // Gray color for guide lines

    // Vertical lines
    for (int col = 0; col <= settings_.columns; col++) {
        float x = gridStartX + (col * cardWidthPt);
        // Extended lines beyond the grid
        HPDF_Page_MoveTo(page, x, 0);
        HPDF_Page_LineTo(page, x, pageHeightPt);
        HPDF_Page_Stroke(page);
    }

    // Horizontal lines
    for (int row = 0; row <= settings_.rows; row++) {
        float y = gridStartY - (row * cardHeightPt);
        // Extended lines beyond the grid
        HPDF_Page_MoveTo(page, 0, y);
        HPDF_Page_LineTo(page, pageWidthPt, y);
        HPDF_Page_Stroke(page);
    }

    // Reset stroke color to black for other drawing operations
    HPDF_Page_SetRGBStroke(page, 0, 0, 0);
}

float CardPDFGenerator::getTotalCardWidth() const {
    return (settings_.cardWidth + (2 * settings_.bleed) + (2 * settings_.borderWidth)) * 72.0f / 25.4f;
}

float CardPDFGenerator::getTotalCardHeight() const {
    return (settings_.cardHeight + (2 * settings_.bleed) + (2 * settings_.borderWidth)) * 72.0f / 25.4f;
}

float CardPDFGenerator::getGridStartX() const {
    float pageWidthPt = settings_.pageWidth * 72.0f / 25.4f;
    float totalGridWidth = getTotalCardWidth() * settings_.columns;
    return (pageWidthPt - totalGridWidth) / 2;
}

float CardPDFGenerator::getGridStartY() const {
    float pageHeightPt = settings_.pageHeight * 72.0f / 25.4f;
    float totalGridHeight = getTotalCardHeight() * settings_.rows;
    return pageHeightPt - ((pageHeightPt - totalGridHeight) / 2);
}

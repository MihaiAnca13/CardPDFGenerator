#ifndef CARD_PDF_GENERATOR_H
#define CARD_PDF_GENERATOR_H

#include <hpdf.h>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

class CardPDFGenerator {
public:
    /**
     * @brief Defines the mode for handling card backs
     */
    enum class BackMode {
        NoBack,    ///< No back pages will be generated
        SameBack,  ///< All cards will use the same back image
        UniqueBack ///< Each card will have its own unique back image
    };

    /**
     * @brief Settings for PDF generation
     */
    struct Settings {
        float pageWidth = 210.0f;  ///< Page width in mm (A4 default)
        float pageHeight = 297.0f; ///< Page height in mm (A4 default)
        float cardWidth = 63.0f;   ///< Card width in mm
        float cardHeight = 88.0f;  ///< Card height in mm
        float bleed = 0.0f;        ///< Bleed area in mm
        int rows = 3;              ///< Number of rows in the grid
        int columns = 3;           ///< Number of columns in the grid
        bool hasBorder = false;    ///< Whether to draw borders around cards
        float borderWidth = 0.0f;  ///< Border width in mm
        struct {
            float r = 0.0f;           ///< Red component (0-1)
            float g = 0.0f;           ///< Green component (0-1)
            float b = 0.0f;           ///< Blue component (0-1)
        } borderColor;
        float guideLineWidth = 0.1f;  ///< Width of cutting guide lines in mm
        bool showGuideLines = true;   ///< Whether to show cutting guide lines
        BackMode backMode = BackMode::NoBack; ///< Mode for handling card backs
    };

    /**
     * @brief Construct a new Card PDF Generator
     * 
     * @param settings Configuration settings for the PDF generator
     * @throw std::runtime_error if PDF object creation fails
     */
    explicit CardPDFGenerator(const Settings &settings);

    /**
     * @brief Destroy the Card PDF Generator
     */
    ~CardPDFGenerator();

    // Delete copy constructor and assignment operator
    CardPDFGenerator(const CardPDFGenerator &) = delete;

    CardPDFGenerator &operator=(const CardPDFGenerator &) = delete;

    /**
     * @brief Generate a PDF with cards
     * 
     * @param outputPath Path where the PDF will be saved
     * @param frontImagesPath Directory containing front images or path to single image
     * @param backImagesPath Directory containing back images or path to single image (optional)
     * @throw std::runtime_error if PDF generation fails
     */
    void generatePDF(const std::string &outputPath,
                     const std::string &frontImagesPath,
                     const std::string &backImagesPath = "");

private:
    HPDF_Doc pdf_;
    Settings settings_;

    /**
     * @brief Validate current settings
     * @throw std::runtime_error if settings are invalid
     */
    void validateSettings() const;

    /**
     * @brief Handle libharu errors
     * 
     * @param error_no Error number
     * @param detail_no Detail number
     * @param user_data User data pointer
     */
    static void error_handler(HPDF_STATUS error_no, HPDF_STATUS detail_no, void *user_data);

    /**
     * @brief Get list of image files from directory
     * 
     * @param dirPath Directory path
     * @return std::vector<fs::path> List of image file paths
     */
    static std::vector<fs::path> getImageFiles(const std::string &dirPath);

    /**
     * @brief Set up a new page in the PDF
     * 
     * @param page HPDF_Page object to set up
     */
    void setupPage(HPDF_Page page) const;

    /**
     * @brief Add a card to the page
     * 
     * @param page HPDF_Page object to add card to
     * @param imagePath Path to the image file
     * @param row Row position in the grid
     * @param col Column position in the grid
     * @param isBack Whether this is a back image
     */
    void addCardToPage(HPDF_Page page,
                       const fs::path &imagePath,
                       int row,
                       int col);

    /**
     * @brief Draw cutting guide lines on the page
     * @param page HPDF_Page object to draw on
     */
    void drawGuideLines(HPDF_Page page) const;

    // Helper methods for layout calculations
    /**
     * @brief Get total width of a card including bleed and border
     * @return float Total card width in mm
     */
    float getTotalCardWidth() const;  ///< Get total width including bleed and border
    /**
     * @brief Get total height of a card including bleed and border
     * @return float Total card height in mm
     */
    float getTotalCardHeight() const; ///< Get total height including bleed and border
    /**
     * @brief Get starting X position of the grid
     * @return float Starting X position in mm
     */
    float getGridStartX() const;      ///< Get starting X position of the grid
    /**
     * @brief Get starting Y position of the grid
     * @return float Starting Y position in mm
     */
    float getGridStartY() const;      ///< Get starting Y position of the grid
};

#endif // CARD_PDF_GENERATOR_H
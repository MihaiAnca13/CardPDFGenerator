//
// Created by mihai on 13-7-25.
//
#include "raylib.h"

// Helper macro for converting Raylib vectors to Clay vectors
#define RAYLIB_VECTOR2_TO_CLAY_VECTOR2(vector) (Clay_Vector2) { .x = (vector).x, .y = (vector).y }

#define CLAY_IMPLEMENTATION
#include "clay.h"
#include "renderers/raylib/clay_renderer_raylib.c"
#include "clay_utils.h"

#include "CardPDFGenerator.h"
#include "settings_io.h"

#include <string>
#include <vector>
#include <cstring> // Required for strlen
#include <cstdio>  // Required for snprintf
#include <map>

// --- UI State & Helper Data ---

/**
 * @struct UIState
 * @brief Holds the current state of the UI, including input field values and status messages.
 */
struct UIState {
    char frontImagesPath[256] = "front_images/";
    char backImagesPath[256] = "back.jpg";
    char outputPath[256] = "output.pdf";
    int activeTextInput = -1; // -1 for none, 0 for front, 1 for back, 2 for output
    char statusMessage[256] = "Ready";
    Color statusColor = LIME;
};

/**
 * @brief Creates a Clay_String from a C-style string.
 * @param c_str The null-terminated C-style string.
 * @return A Clay_String instance.
 */
Clay_String make_clay_string(const char* c_str) {
    return (Clay_String){
        .isStaticallyAllocated = false,
        .length = (int32_t)strlen(c_str),
        .chars = c_str
    };
}


// --- Custom UI Widgets ---

/**
 * @brief Renders a clickable button.
 * @param id The unique identifier for this element.
 * @param text The text to display on the button.
 * @return True if the button was clicked in the current frame, otherwise false.
 */
bool GuiButton(Clay_ElementId id, const char* text) {
    bool clicked = false;
    CLAY({
        .id = id,
        .layout = {.sizing = {CLAY_SIZING_FIXED(130), CLAY_SIZING_FIXED(40)}, .padding = {10, 0, 0, 20}, .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}},
        .backgroundColor = Clay_Hovered() ? (Clay_Color){100, 180, 250, 255} : (Clay_Color){0, 121, 241, 255},
        .cornerRadius = CLAY_CORNER_RADIUS(5),
    }) {
        CLAY_TEXT(make_clay_string(text), CLAY_TEXT_CONFIG({
            .textColor = Clay_Hovered() ? (Clay_Color){100, 100, 100, 255} : (Clay_Color){220, 220, 220, 255},
            .fontSize = 20
        }));
        if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    }
    return clicked;
}

// Forward declaration for GuiSliderFloat
void GuiFloatInput(Clay_ElementId id, float* value, float min, float max, int inputId, int* activeInputId, const char* format = "%.2f");

/**
 * @brief Renders a slider with a text input for a float value.
 * @param id The unique identifier for this element.
 * @param text The label to display for the slider.
 * @param value A pointer to the float value to be modified.
 * @param min The minimum value of the slider.
 * @param max The maximum value of the slider.
 * @param uiState A pointer to the global UI state.
 * @param rounded If true, the value will be rounded to the nearest integer.
 */
void GuiSliderFloat(Clay_ElementId id, const char* text, float* value, float min, float max, UIState* uiState, bool rounded = false) {
    constexpr int MAX_SLIDERS = 64; // Increased for more complex UIs
    static int sliderIndex = 0;

    // Use a different ID for the text input to avoid conflicts
    Clay_ElementId textInputId = id;
    textInputId.id += MAX_SLIDERS; // Offset the ID

    // Main container for the widget (Label + Slider + Value Text)
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW()},
            .childGap = 10,
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        }
    }) {
        CLAY_TEXT(make_clay_string(text), CLAY_TEXT_CONFIG({.textColor={100,100,100,255}, .fontSize=16}));

        CLAY({.layout = {.childGap = 10, .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}}}) {
            const float sliderWidth = 200;
            const float sliderHeight = 20;
            const float handleWidth = 16;
            const float handleHeight = 20;

            // Interactive slider area (the track)
            CLAY({
                .id = id,
                .layout = {.sizing = {CLAY_SIZING_FIXED(sliderWidth), CLAY_SIZING_FIXED(sliderHeight)}},
                .backgroundColor = {220, 220, 220, 255}, // The track color
                .cornerRadius = CLAY_CORNER_RADIUS(10)
            }) {
                if (Clay_Hovered() && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    Clay_ElementData data = Clay_GetElementData(id);
                    if (data.found) {
                        float mouseX = GetMousePosition().x;
                        float newPercent = (mouseX - data.boundingBox.x) / data.boundingBox.width;
                        *value = min + newPercent * (max - min);
                        if (*value < min) *value = min;
                        if (*value > max) *value = max;
                        if (rounded) *value = roundf(*value);
                    }
                }

                float percent = (*value - min) / (max - min);
                if (percent < 0) percent = 0;
                if (percent > 1) percent = 1;

                // The filled portion of the slider track
                if (percent > 0) {
                    CLAY({
                        .layout = {.sizing = {CLAY_SIZING_PERCENT(percent), CLAY_SIZING_GROW()}},
                        .backgroundColor = {0, 121, 241, 255},
                        .cornerRadius = CLAY_CORNER_RADIUS(10)
                    }) {}
                }

                // The slider handle
                CLAY({
                    .layout = {.sizing = {CLAY_SIZING_FIXED(handleWidth), CLAY_SIZING_FIXED(handleHeight)}},
                    .backgroundColor = {255, 255, 255, 255},
                    .cornerRadius = CLAY_CORNER_RADIUS(8),
                    .floating = {
                        .offset = {percent * (sliderWidth - handleWidth), 0},
                        .attachPoints = {.element = CLAY_ATTACH_POINT_LEFT_CENTER, .parent = CLAY_ATTACH_POINT_LEFT_CENTER},
                        .pointerCaptureMode = CLAY_POINTER_CAPTURE_MODE_PASSTHROUGH,
                        .attachTo = CLAY_ATTACH_TO_PARENT,
                    },
                    .border = {.color = {0, 121, 241, 255}, .width = {2, 2, 2, 2}}
                }) {}
            }

            // The float input field paired with the slider
            GuiFloatInput(textInputId, value, min, max, (int)textInputId.id, &uiState->activeTextInput, rounded ? "%.0f" : "%.2f");
        }
    }

    sliderIndex = (sliderIndex + 1) % MAX_SLIDERS;
}

/**
 * @brief Renders a slider for an integer value.
 * This is a wrapper around GuiSliderFloat.
 * @param id The unique identifier for this element.
 * @param text The label to display for the slider.
 * @param value A pointer to the integer value to be modified.
 * @param min The minimum value of the slider.
 * @param max The maximum value of the slider.
 * @param uiState A pointer to the global UI state.
 */
void GuiSliderInt(Clay_ElementId id, const char* text, int* value, int min, int max, UIState* uiState)
{
    auto v = static_cast<float>(*value);
    GuiSliderFloat(id, text, &v, (float)min, (float)max, uiState, true);
    *value = static_cast<int>(roundf(v));
}

/**
 * @brief Renders a checkbox for a boolean value.
 * @param id The unique identifier for this element.
 * @param text The label to display next to the checkbox.
 * @param value A pointer to the boolean value to be modified.
 */
void GuiCheckbox(Clay_ElementId id, const char* text, bool* value) {
    CLAY({.layout = {.childGap = 10, .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}}}) {
        // The interactive checkbox square
        CLAY({
            .id = id,
            .layout = {.sizing = {CLAY_SIZING_FIXED(22), CLAY_SIZING_FIXED(22)}, .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER}},
            .backgroundColor = Clay_Hovered() ? (Clay_Color){230, 230, 230, 255} : (Clay_Color){255, 255, 255, 255},
            .cornerRadius = CLAY_CORNER_RADIUS(4),
            .border = {.color = {150, 150, 150, 255}, .width = {2, 2, 2, 2}}
        }) {
            if (*value) {
                // The checkmark inside
                CLAY({
                    .layout = {.sizing = {CLAY_SIZING_FIXED(12), CLAY_SIZING_FIXED(12)}},
                    .backgroundColor = {0, 121, 241, 255},
                    .cornerRadius = CLAY_CORNER_RADIUS(2)
                }) {}
            }
            if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                *value = !*value;
            }
        }
        // The label next to the checkbox
        CLAY_TEXT(make_clay_string(text), CLAY_TEXT_CONFIG({.textColor = {100, 100, 100, 255}, .fontSize = 16}));
    }
}

/**
 * @brief Renders a text input field.
 * @param id The unique identifier for this element.
 * @param label The label to display for the input field.
 * @param buffer The character buffer to store the text.
 * @param bufferSize The size of the buffer.
 * @param inputId A unique integer ID for this specific input field.
 * @param activeInputId A pointer to the integer that stores the ID of the currently active input field.
 */
void GuiTextInput(Clay_ElementId id, const char* label, char* buffer, int bufferSize, int inputId, int* activeInputId) {
    bool isActive = (*activeInputId == inputId);

    // Deactivate if Enter is pressed or if the mouse is clicked anywhere outside of this specific text input
    if (isActive && (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !Clay_PointerOver(id)))) {
        *activeInputId = -1;
        isActive = false; // Update state immediately for visual feedback
    }

    CLAY({.layout = {.childGap = 30, .childAlignment = {.y = CLAY_ALIGN_Y_CENTER}, .layoutDirection = CLAY_TOP_TO_BOTTOM}}) {
        CLAY_TEXT(make_clay_string(label), CLAY_TEXT_CONFIG({.textColor = {100, 100, 100, 255}, .fontSize = 16}));

        // The main input box element
        CLAY({
            .id = id, // Use the passed-in ID for the interactive element
            .layout = {.sizing = {CLAY_SIZING_FIXED(300), CLAY_SIZING_FIXED(30)}, .padding = {8, 8, 5, 5}},
            .backgroundColor = {255, 255, 255, 255},
            .cornerRadius = CLAY_CORNER_RADIUS(5),
            .border = {.color = isActive ? (Clay_Color){0, 121, 241, 255} : (Clay_Color){200, 200, 200, 255}, .width = {2, 2, 2, 2}}
        }) {
            CLAY_TEXT(make_clay_string(buffer), CLAY_TEXT_CONFIG({.textColor = {30, 30, 30, 255}, .fontSize = 16}));

            // Activate on click
            if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                *activeInputId = inputId;
            }
        }
    }

    // Handle key presses only when active
    if (isActive) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125)) {
                int len = strlen(buffer);
                if (len < bufferSize - 1) {
                    buffer[len] = (char)key;
                    buffer[len + 1] = '\0';
                }
            }
            key = GetCharPressed(); // Process all characters from the queue
        }

        if (IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE)) {
            int len = strlen(buffer);
            if (len > 0) {
                buffer[len - 1] = '\0';
            }
        }
    }
}

/**
 * @brief Renders a text input field for a float value.
 * Manages its own internal string buffer.
 * @param id The unique identifier for this element.
 * @param value A pointer to the float value to be modified.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @param inputId A unique integer ID for this specific input field.
 * @param activeInputId A pointer to the integer that stores the ID of the currently active input field.
 * @param format The printf-style format string for displaying the float.
 */
void GuiFloatInput(Clay_ElementId id, float* value, float min, float max, int inputId, int* activeInputId, const char* format) {
    static std::map<int, std::string> buffers;

    bool isActive = (*activeInputId == inputId);
    std::string& buffer = buffers[inputId]; // Get or create a persistent buffer for this input

    // If the input is NOT active, it should always reflect the authoritative value from the slider.
    if (!isActive) {
        char temp[32];
        snprintf(temp, sizeof(temp), format, *value);
        buffer = temp;
    }

    // Deactivate and save value on Enter or click-away
    if (isActive && (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !Clay_PointerOver(id)))) {
        *activeInputId = -1;
        isActive = false;

        // Parse, clamp, and save
        float parsedValue = strtof(buffer.c_str(), nullptr);
        if (parsedValue < min) parsedValue = min;
        if (parsedValue > max) parsedValue = max;
        *value = parsedValue;

        // Update buffer to show the potentially clamped value
        char temp[32];
        snprintf(temp, sizeof(temp), format, *value);
        buffer = temp;
    }

    // The input box
    CLAY({
        .id = id,
        .layout = {.sizing = {CLAY_SIZING_FIXED(70), CLAY_SIZING_FIXED(25)}, .padding = {5, 5, 5, 5}},
        .backgroundColor = {255, 255, 255, 255},
        .cornerRadius = CLAY_CORNER_RADIUS(5),
        .border = {.color = isActive ? (Clay_Color){0, 121, 241, 255} : (Clay_Color){200, 200, 200, 255}, .width = {1, 1, 1, 1}}
    }) {
        CLAY_TEXT(make_clay_string(buffer.c_str()), CLAY_TEXT_CONFIG({.textColor = {30, 30, 30, 255}, .fontSize = 14}));

        // Activate on click. The buffer is already synced if not active, so we just set the ID.
        if (Clay_Hovered() && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            *activeInputId = inputId;
        }
    }

    // Handle key presses only when active
    if (isActive) {
        int key = GetCharPressed();
        while (key > 0) {
            // Allow numbers, decimal point, and negative sign
            if ((key >= '0' && key <= '9') || key == '.' || key == '-') {
                if (buffer.length() < 31) {
                    buffer += (char)key;
                }
            }
            key = GetCharPressed();
        }

        if (IsKeyPressedRepeat(KEY_BACKSPACE) || IsKeyPressed(KEY_BACKSPACE)) {
            if (!buffer.empty()) {
                buffer.pop_back();
            }
        }
    }
}

/**
 * @brief Renders a text input for an integer value.
 * This is a wrapper around GuiFloatInput.
 * @param id The unique identifier for this element.
 * @param value A pointer to the integer value to be modified.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @param inputId A unique integer ID for this specific input field.
 * @param activeInputId A pointer to the integer that stores the ID of the currently active input field.
 */
void GuiIntInput(Clay_ElementId id, int* value, int min, int max, int inputId, int* activeInputId) {
    GuiFloatInput(id, (float*)value, (float)min, (float)max, inputId, activeInputId, "%.0f");
}

// --- Main Application ---
int main() {
    const int screenWidth = 900; // Increased width for better layout
    const int screenHeight = 1050; // Increased height

    // --- Initialization ---
    Clay_Raylib_Initialize(screenWidth, screenHeight, "Card PDF Generator - UI", FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    uint64_t totalMemorySize = Clay_MinMemorySize();
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(totalMemorySize, malloc(totalMemorySize));
    Clay_Initialize(arena, (Clay_Dimensions){(float)screenWidth, (float)screenHeight}, (Clay_ErrorHandler){0});

    // --- Load Settings ---
    CardPDFGenerator::Settings settings;
    UIState uiState;
    load_settings(settings, "pdf_settings.txt");

    Font fonts[1];
    fonts[0] = LoadFont("fonts/static/FunnelDisplay-Light.ttf");

    // --- Main Loop ---
    while (!WindowShouldClose()) {
        // Update Clay layout and input state
        Clay_SetLayoutDimensions((Clay_Dimensions){(float)GetScreenWidth(), (float)GetScreenHeight()});
        Clay_SetPointerState(RAYLIB_VECTOR2_TO_CLAY_VECTOR2(GetMousePosition()), IsMouseButtonDown(MOUSE_BUTTON_LEFT));
        Clay_UpdateScrollContainers(true, RAYLIB_VECTOR2_TO_CLAY_VECTOR2(GetMouseWheelMoveV()), GetFrameTime());

        Clay_BeginLayout();

        // --- UI Declaration ---
        CLAY({ // Root container
            .layout = {
                .sizing = CLAY_SIZING_GROW(),
                .padding = CLAY_PADDING_ALL(24),
                .childGap = 20,
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = {245, 245, 245, 255}
        }) {
            CLAY_TEXT(CLAY_STRING("Card PDF Generator Settings"), CLAY_TEXT_CONFIG({.textColor={80,80,80,255}, .fontSize=28}));
            CLAY({.layout={.sizing={.height=CLAY_SIZING_FIXED(EL_SPACE)}}}){}; // Spacer
            
            // Main container for the two settings columns
            CLAY({
                .layout = {
                    .sizing = CLAY_SIZING_GROW(),
                    .childGap = 40, // Added a gap between the two columns
                    .layoutDirection = CLAY_LEFT_TO_RIGHT
                }
            }) {

                // Left Column: Page and Card Dimensions
                CLAY({
                    .layout = {
                        .sizing = {CLAY_SIZING_PERCENT(0.5)},
                        .childGap = 25,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    }
                }) {
                    CLAY_TEXT(CLAY_STRING("Page & Card Dimensions (mm)"), CLAY_TEXT_CONFIG({.textColor={100,100,100,255}, .fontSize=18}));
                    GuiSliderFloat(CLAY_ID("pageWidth"), "Page Width", &settings.pageWidth, 100.0f, 500.0f, &uiState);
                    GuiSliderFloat(CLAY_ID("pageHeight"), "Page Height", &settings.pageHeight, 100.0f, 500.0f, &uiState);
                    GuiSliderFloat(CLAY_ID("cardWidth"), "Card Width", &settings.cardWidth, 40.0f, 100.0f, &uiState);
                    GuiSliderFloat(CLAY_ID("cardHeight"), "Card Height", &settings.cardHeight, 60.0f, 120.0f, &uiState);
                    GuiSliderFloat(CLAY_ID("bleed"), "Bleed", &settings.bleed, 0.0f, 10.0f, &uiState);

                    CLAY({.layout={.sizing={.height=CLAY_SIZING_FIXED(5)}}}){}; // Spacer

                    CLAY_TEXT(CLAY_STRING("Grid Layout"), CLAY_TEXT_CONFIG({.textColor={100,100,100,255}, .fontSize=20}));
                    GuiSliderInt(CLAY_ID("rows"), "Rows", &settings.rows, 1, 10, &uiState);
                    GuiSliderInt(CLAY_ID("columns"), "Columns", &settings.columns, 1, 10, &uiState);
                }

                // Right Column: Appearance and Color
                CLAY({
                    .layout = {
                        .sizing = {CLAY_SIZING_PERCENT(0.5)},
                        .childGap = 25,
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    }
                }) {
                    CLAY_TEXT(CLAY_STRING("Appearance"), CLAY_TEXT_CONFIG({.textColor={100,100,100,255}, .fontSize=18}));
                    GuiCheckbox(CLAY_ID("showGuidelines"), "Show Guidelines", &settings.showGuideLines);
                    GuiSliderFloat(CLAY_ID("guideLineWidth"), "Guide Width", &settings.guideLineWidth, 0.0f, 2.0f, &uiState);
                    GuiCheckbox(CLAY_ID("hasBorder"), "Has Border", &settings.hasBorder);
                    GuiSliderFloat(CLAY_ID("borderWidth"), "Border Width", &settings.borderWidth, 0.0f, 10.0f, &uiState);

                    CLAY({.layout={.sizing={.height=CLAY_SIZING_FIXED(5)}}}){}; // Spacer

                    CLAY_TEXT(CLAY_STRING("Border Color (RGB)"), CLAY_TEXT_CONFIG({.textColor={100,100,100,255}, .fontSize=18}));
                    GuiSliderFloat(CLAY_ID("borderColorR"), "Red", &settings.borderColor.r, 0.0f, 1.0f, &uiState);
                    GuiSliderFloat(CLAY_ID("borderColorG"), "Green", &settings.borderColor.g, 0.0f, 1.0f, &uiState);
                    GuiSliderFloat(CLAY_ID("borderColorB"), "Blue", &settings.borderColor.b, 0.0f, 1.0f, &uiState);
                }
            }

            CLAY({.layout={.sizing={.height=CLAY_SIZING_FIXED(EL_SPACE)}}}){}; // Spacer

            // --- Back Mode & File Paths Section ---
            CLAY({
                .layout = {
                    .sizing = CLAY_SIZING_GROW(),
                    .padding = {.top = 20},
                    .childGap = 25,
                    .layoutDirection = CLAY_TOP_TO_BOTTOM,
                }
            }) {
                CLAY_TEXT(CLAY_STRING("Card Back Options"), CLAY_TEXT_CONFIG({.textColor={100,100,100,255}, .fontSize=18}));
                CLAY({.layout = {.childGap = 20}}) {
                    bool noBack = settings.backMode == CardPDFGenerator::BackMode::NoBack;
                    if (GuiButton(CLAY_ID("noBack"), noBack ? "[ No Back ]" : "No Back")) settings.backMode = CardPDFGenerator::BackMode::NoBack;

                    bool sameBack = settings.backMode == CardPDFGenerator::BackMode::SameBack;
                    if (GuiButton(CLAY_ID("sameBack"), sameBack ? "[ Same Back ]" : "Same Back")) settings.backMode = CardPDFGenerator::BackMode::SameBack;

                    bool uniqueBack = settings.backMode == CardPDFGenerator::BackMode::UniqueBack;
                    if (GuiButton(CLAY_ID("uniqueBack"), uniqueBack ? "[ Unique Backs ]" : "Unique Backs")) settings.backMode = CardPDFGenerator::BackMode::UniqueBack;
                }

                CLAY({.layout={.sizing={.height=CLAY_SIZING_FIXED(5)}}}){}; // Spacer

                GuiTextInput(CLAY_ID("frontPathInput"), "Front Images Path", uiState.frontImagesPath, 256, 0, &uiState.activeTextInput);
                GuiTextInput(CLAY_ID("backPathInput"), "Back Image Path", uiState.backImagesPath, 256, 1, &uiState.activeTextInput);
                GuiTextInput(CLAY_ID("outputPathInput"), "Output PDF Path", uiState.outputPath, 256, 2, &uiState.activeTextInput);
            }

            // --- Action Buttons & Status ---
            CLAY({.layout={.sizing={.height=CLAY_SIZING_GROW(0)}}}){}; // Spacer to push to bottom
            CLAY({.layout = {.childGap = 20, .childAlignment={.y=CLAY_ALIGN_Y_CENTER} }}) {
                if (GuiButton(CLAY_ID("generate"), "Generate PDF")) {
                    try {
                        CardPDFGenerator generator(settings);
                        generator.generatePDF(uiState.outputPath, uiState.frontImagesPath, uiState.backImagesPath);
                        strcpy(uiState.statusMessage, "Success! PDF generated.");
                        uiState.statusColor = LIME;
                    } catch (const std::exception& e) {
                        snprintf(uiState.statusMessage, sizeof(uiState.statusMessage), "Error: %s", e.what());
                        uiState.statusColor = RED;
                    }
                }
                if (GuiButton(CLAY_ID("save"), "Save Settings")) {
                    save_settings(settings, "pdf_settings.txt");
                    strcpy(uiState.statusMessage, "Settings saved successfully.");
                    uiState.statusColor = LIME;
                }
                CLAY_TEXT(make_clay_string(uiState.statusMessage), CLAY_TEXT_CONFIG({.textColor = ToClayColor(uiState.statusColor), .fontSize = 16}));
            }
        }

        Clay_RenderCommandArray renderCommands = Clay_EndLayout();

        // --- Drawing ---
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Clay_Raylib_Render(renderCommands, fonts);
        EndDrawing();
    }

    // --- Cleanup ---
    UnloadFont(fonts[0]);
    Clay_Raylib_Close();
    free(arena.memory);
    return 0;
}

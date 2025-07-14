#pragma once

#ifndef CLAY_UTILS_H
#define CLAY_UTILS_H

#include "clay.h"
#include "raylib.h"

#define EL_SPACE 0.1f

// Helper function to convert a raylib Color to a Clay_Color
static inline Clay_Color ToClayColor(const Color& color) {
    return {
        static_cast<float>(color.r),
        static_cast<float>(color.g),
        static_cast<float>(color.b),
        static_cast<float>(color.a)
    };
}

// Helper function to convert a Clay_Color to a raylib Color
static inline Color ToRaylibColor(const Clay_Color& color) {
    return {
        static_cast<unsigned char>(color.r),
        static_cast<unsigned char>(color.g),
        static_cast<unsigned char>(color.b),
        static_cast<unsigned char>(color.a)
    };
}

#endif

//
// Created by depau on 4/26/22.
//

#pragma once

#include "raylib-cpp.hpp"

namespace raylib_ext::text3d {

//--------------------------------------------------------------------------------------
// Module Functions Declaration
//--------------------------------------------------------------------------------------
// Draw a codepoint in 3D space
void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint);
// Draw a 2D text in 3D space
void DrawText3D(Font font,
                const char *text,
                Vector3 position,
                float fontSize,
                float fontSpacing,
                float lineSpacing,
                bool backface,
                Color tint);
// Measure a text in 3D. For some reason `MeasureTextEx()` just doesn't seem to work so i had to use this instead.
Vector3 MeasureText3D(Font font, const char *text, float fontSize, float fontSpacing, float lineSpacing);

} // namespace raylib_ext::text3d
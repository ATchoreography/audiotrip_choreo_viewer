//
// Created by depau on 4/26/22.
// Based on an example from raylib, of which the copyright header follows.
//

/*******************************************************************************************
 *
 *   raylib [text] example - Draw 2D text in 3D
 *
 *   Draw a 2D text in 3D space, each letter is drawn in a quad (or 2 quads if backface is set)
 *   where the texture coodinates of each quad map to the texture coordinates of the glyphs
 *   inside the font texture.
 *    A more efficient approach, i believe, would be to render the text in a render texture and
 *    map that texture to a plane and render that, or maybe a shader but my method allows more
 *    flexibility...for example to change position of each letter individually to make somethink
 *    like a wavy text effect.
 *
 *    Special thanks to:
 *        @Nighten for the DrawTextStyle() code https://github.com/NightenDushi/Raylib_DrawTextStyle
 *        Chris Camacho (codifies - http://bedroomcoders.co.uk/) for the alpha discard shader
 *
 *   This example has been created using raylib 3.5 (www.raylib.com)
 *   raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
 *
 *   Example contributed by Vlad Adrian (@Demizdor) and reviewed by Ramon Santamaria (@raysan5)
 *
 *   Copyright (C) 2021 Vlad Adrian (@Demizdor - https://github.com/Demizdor)
 *
 ********************************************************************************************/

#include "raylib-cpp.hpp"
#include "raylib_ext/text3d.h"

namespace {
extern "C" {
#include "rlgl.h"
}
} // namespace

#define LETTER_BOUNDRY_SIZE 0.25f
#define TEXT_MAX_LAYERS 32
#define LETTER_BOUNDRY_COLOR VIOLET

namespace raylib_ext::text3d {

bool SHOW_LETTER_BOUNDRY = false;
bool SHOW_TEXT_BOUNDRY = false;

// Configuration structure for waving the text
typedef struct {
  Vector3 waveRange;
  Vector3 waveSpeed;
  Vector3 waveOffset;
} WaveTextConfig;

// Draw codepoint at specified position in 3D space

void DrawTextCodepoint3D(Font font, int codepoint, Vector3 position, float fontSize, bool backface, Color tint) {
  // Character index position in sprite font
  // NOTE: In case a codepoint is not available in the font, index returned points to '?'
  int index = GetGlyphIndex(font, codepoint);
  float scale = fontSize / (float) font.baseSize;

  // Character destination rectangle on screen
  // NOTE: We consider charsPadding on drawing
  position.x += (float) (font.glyphs[index].offsetX - font.glyphPadding) / (float) font.baseSize * scale;
  position.z += (float) (font.glyphs[index].offsetY - font.glyphPadding) / (float) font.baseSize * scale;

  // Character source rectangle from font texture atlas
  // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
  Rectangle srcRec = { font.recs[index].x - (float) font.glyphPadding,
                       font.recs[index].y - (float) font.glyphPadding,
                       font.recs[index].width + 2.0f * font.glyphPadding,
                       font.recs[index].height + 2.0f * font.glyphPadding };

  float width = (float) (font.recs[index].width + 2.0f * font.glyphPadding) / (float) font.baseSize * scale;
  float height = (float) (font.recs[index].height + 2.0f * font.glyphPadding) / (float) font.baseSize * scale;

  if (font.texture.id > 0) {
    const float x = 0.0f;
    const float y = 0.0f;
    const float z = 0.0f;

    // normalized texture coordinates of the glyph inside the font texture (0.0f -> 1.0f)
    const float tx = srcRec.x / font.texture.width;
    const float ty = srcRec.y / font.texture.height;
    const float tw = (srcRec.x + srcRec.width) / font.texture.width;
    const float th = (srcRec.y + srcRec.height) / font.texture.height;

    if (SHOW_LETTER_BOUNDRY)
      DrawCubeWiresV((Vector3){ position.x + width / 2, position.y, position.z + height / 2 },
                     (Vector3){ width, LETTER_BOUNDRY_SIZE, height },
                     LETTER_BOUNDRY_COLOR);

    rlCheckRenderBatchLimit(4 + 4 * backface);
    rlSetTexture(font.texture.id);

    rlPushMatrix();
    rlTranslatef(position.x, position.y, position.z);

    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    // Front Face
    rlNormal3f(0.0f, 1.0f, 0.0f); // Normal Pointing Up
    rlTexCoord2f(tx, ty);
    rlVertex3f(x, y, z); // Top Left Of The Texture and Quad
    rlTexCoord2f(tx, th);
    rlVertex3f(x, y, z + height); // Bottom Left Of The Texture and Quad
    rlTexCoord2f(tw, th);
    rlVertex3f(x + width, y, z + height); // Bottom Right Of The Texture and Quad
    rlTexCoord2f(tw, ty);
    rlVertex3f(x + width, y, z); // Top Right Of The Texture and Quad

    if (backface) {
      // Back Face
      rlNormal3f(0.0f, -1.0f, 0.0f); // Normal Pointing Down
      rlTexCoord2f(tx, ty);
      rlVertex3f(x, y, z); // Top Right Of The Texture and Quad
      rlTexCoord2f(tw, ty);
      rlVertex3f(x + width, y, z); // Top Left Of The Texture and Quad
      rlTexCoord2f(tw, th);
      rlVertex3f(x + width, y, z + height); // Bottom Left Of The Texture and Quad
      rlTexCoord2f(tx, th);
      rlVertex3f(x, y, z + height); // Bottom Right Of The Texture and Quad
    }
    rlEnd();
    rlPopMatrix();

    rlSetTexture(0);
  }
}

void DrawText3D(Font font,
                const char *text,
                Vector3 position,
                float fontSize,
                float fontSpacing,
                float lineSpacing,
                bool backface,
                Color tint) {
  int length = TextLength(text); // Total length in bytes of the text, scanned by codepoints in loop

  float textOffsetY = 0.0f; // Offset between lines (on line break '\n')
  float textOffsetX = 0.0f; // Offset X to next character to draw

  float scale = fontSize / (float) font.baseSize;

  for (int i = 0; i < length;) {
    // Get next codepoint from byte string and glyph index in font
    int codepointByteCount = 0;
    int codepoint = GetCodepoint(&text[i], &codepointByteCount);
    int index = GetGlyphIndex(font, codepoint);

    // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
    // but we need to draw all of the bad bytes using the '?' symbol moving one byte
    if (codepoint == 0x3f)
      codepointByteCount = 1;

    if (codepoint == '\n') {
      // NOTE: Fixed line spacing of 1.5 line-height
      // TODO: Support custom line spacing defined by user
      textOffsetY += scale + lineSpacing / (float) font.baseSize * scale;
      textOffsetX = 0.0f;
    } else {
      if ((codepoint != ' ') && (codepoint != '\t')) {
        DrawTextCodepoint3D(font,
                            codepoint,
                            (Vector3){ position.x + textOffsetX, position.y, position.z + textOffsetY },
                            fontSize,
                            backface,
                            tint);
      }

      if (font.glyphs[index].advanceX == 0)
        textOffsetX += (float) (font.recs[index].width + fontSpacing) / (float) font.baseSize * scale;
      else
        textOffsetX += (float) (font.glyphs[index].advanceX + fontSpacing) / (float) font.baseSize * scale;
    }

    i += codepointByteCount; // Move text bytes counter to next codepoint
  }
}

Vector3 MeasureText3D(Font font, const char *text, float fontSize, float fontSpacing, float lineSpacing) {
  int len = TextLength(text);
  int tempLen = 0; // Used to count longer text line num chars
  int lenCounter = 0;

  float tempTextWidth = 0.0f; // Used to count longer text line width

  float scale = fontSize / (float) font.baseSize;
  float textHeight = scale;
  float textWidth = 0.0f;

  int letter = 0; // Current character
  int index = 0; // Index position in sprite font

  for (int i = 0; i < len; i++) {
    lenCounter++;

    int next = 0;
    letter = GetCodepoint(&text[i], &next);
    index = GetGlyphIndex(font, letter);

    // NOTE: normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
    // but we need to draw all of the bad bytes using the '?' symbol so to not skip any we set next = 1
    if (letter == 0x3f)
      next = 1;
    i += next - 1;

    if (letter != '\n') {
      if (font.glyphs[index].advanceX != 0)
        textWidth += (font.glyphs[index].advanceX + fontSpacing) / (float) font.baseSize * scale;
      else
        textWidth += (font.recs[index].width + font.glyphs[index].offsetX) / (float) font.baseSize * scale;
    } else {
      if (tempTextWidth < textWidth)
        tempTextWidth = textWidth;
      lenCounter = 0;
      textWidth = 0.0f;
      textHeight += scale + lineSpacing / (float) font.baseSize * scale;
    }

    if (tempLen < lenCounter)
      tempLen = lenCounter;
  }

  if (tempTextWidth < textWidth)
    tempTextWidth = textWidth;

  Vector3 vec = { 0 };
  vec.x = tempTextWidth + (float) ((tempLen - 1) * fontSpacing / (float) font.baseSize * scale); // Adds chars spacing
                                                                                                 // to measure
  vec.y = 0.25f;
  vec.z = textHeight;

  return vec;
}
} // namespace raylib_ext::text3d
//
// Created by depau on 5/29/22.
//

// This file compiles the implementation of raygui

#include <cmath> // Required for: roundf() [GuiColorPicker()]
#include <cstdarg> // Required for: va_list, va_start(), vfprintf(), va_end() [TextFormat()]
#include <cstdio> // Required for: FILE, fopen(), fclose(), fprintf(), feof(), fscanf(), vsprintf() [GuiLoadStyle(), GuiLoadIcons()]
#include <cstdlib> // Required for: malloc(), calloc(), free() [GuiLoadStyle(), GuiLoadIcons()]
#include <cstring> // Required for: strlen() [GuiTextBox(), GuiTextBoxMulti(), GuiValueBox()], memset(), memcpy()

namespace raygui {
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
} // namespace raygui
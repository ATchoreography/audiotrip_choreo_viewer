//
// Created by depau on 5/29/22.
//

#pragma once


#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
//#define MODELS_SUFFIX ""
#else // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

#define INITIAL_DISTANCE (-2.5f)
#define PLAYER_HEIGHT (1.381876)
#define MAX_RENDER_DISTANCE (300.0f)

// The idea was to use the high-definition models on desktop but in practice the difference is barely noticeable, so
// I'll use them everywhere.
#define MODELS_SUFFIX "_lowlod"

//
// Created by depau on 5/29/22.
//

#pragma once

#include <fmt/format.h>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

// Local includes
#include "GUIState.h"
#include "audiotrip/dtos.h"
#include "raylib_ext/scoped.h"
#include "raylib_ext/text3d.h"
#include "rendering/SkyBox.h"

struct hash_tuple {
  size_t operator()(const std::tuple<int, int, int, int> &x) const {
    return std::get<0>(x) ^ std::get<1>(x) ^ std::get<2>(x) ^ std::get<3>(x);
  }
};

class Application {
private:
  using RibbonKey = std::tuple<int, int, int, int>; // beat, numerator, denominator, RHS (bool as int)

  std::unique_ptr<raylib::Window> window;
  std::unique_ptr<raylib::Camera> camera;
  std::unique_ptr<raylib::Shader> shader;

  std::unique_ptr<raylib::Texture2D> floorTexture;

  std::unique_ptr<raylib::Model> barrierModel;

  std::unique_ptr<raylib::Model> gemModel;
  std::unique_ptr<raylib::Model> gemTrailModel;
  std::unique_ptr<raylib::Model> drumModel;
  std::unique_ptr<raylib::Model> dirgemModel;

  std::unique_ptr<SkyBox> skybox;

  Vector3 beatNumbersSize = { -1, -1, -1 };

  std::unique_ptr<audiotrip::AudioTripSong> ats;
  std::vector<audiotrip::Beat> beats;
  std::unordered_map<RibbonKey, std::pair<raylib::Model, raylib::Vector3>, hash_tuple> ribbons;

  bool mouseCaptured = true;
  bool debug = false;

  GUIState gui;

  audiotrip::Choreography &choreo() { return ats->choreographies.at(gui.choreoSelectorActive); }

public:
  Application(bool debug = false);

  ~Application() { ClearDroppedFiles(); }

  void main(std::optional<std::string> atsFile) {
    beatNumbersSize = raylib_ext::text3d::MeasureText3D(GetFontDefault(), "1", 8.0f, 1.0f, 0.0f);

    if (atsFile.has_value()) {
      openAts(*atsFile);
    } else {
      mouseCapture(false);
    }

#ifdef PLATFORM_WEB
    emscripten_set_main_loop_arg(emscriptenMainloop, this, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
      drawFrame();
    }
#endif
  }

private:
  void mouseCapture(std::optional<bool> val) {
    mouseCaptured = val.has_value() ? *val : !mouseCaptured;
    if (!debug && mouseCaptured)
      DisableCursor();
    else
      EnableCursor();
  }

  void openAts(const std::string &path);

  static void emscriptenMainloop(void *obj) {
    static_cast<Application *>(obj)->drawFrame();
  }

  void drawFrame();

  void drawSplash();

  float getBeatTime(float beatNum) {
    auto intBeat = static_cast<size_t>(beatNum);
    audiotrip::Beat beat = beats[intBeat];
    float beatTime = beat.time;
    float fraction = beatNum - static_cast<float>(intBeat);

    if (fraction != 1 && fraction != 0) {
      float secondsPerBeat = 60.0f / beat.bpm;
      beatTime += secondsPerBeat * fraction;
    }

    return beatTime;
  }

  void drawChoreo();

  void drawChoreoEventElement(const audiotrip::ChoreoEvent &event, float distance);

  std::pair<raylib::Model &, Vector3> genOrGetRibbon(const audiotrip::ChoreoEvent &event, float distance);
};
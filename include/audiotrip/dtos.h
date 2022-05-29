//
// Created by depau on 4/26/22.
//

/**
 * Mostly incomplete, I'll only add the stuff that I need (ish) because I'm lazy.
 */

#pragma once

#include <cassert>
#include <fstream>
#include <iostream>
#include <istream>
#include <tuple>

#include "Vector3.hpp"
#include "json/json.h"

namespace {

template<typename T>
std::vector<T> fromJsonArray(const Json::Value &j) {
  std::vector<T> result;
  assert(j.isArray());
  for (const Json::Value &element : j)
    result.push_back(T(element));
  return result;
}

} // namespace

namespace audiotrip {

struct BeatTime {
  int beat;
  int numerator;
  int denominator;

  BeatTime(const Json::Value &j);
};

class Position : public std::tuple<float, float, float> {
public:
  [[nodiscard]] float x() const { return std::get<0>(*this); }
  [[nodiscard]] float y() const { return std::get<1>(*this); }
  [[nodiscard]] float z() const { return std::get<2>(*this); }

  Position(const Json::Value &j);

  // X axis is inverted
  operator Vector3() const { return { -x(), y(), z() }; }

  Vector3 vectorWithDistance(float distance) const { return { -x(), y(), distance }; }
};

enum ChoreoEventType {
  ChoreoEventTypeBarrier = 0,
  ChoreoEventTypeGemL = 1,
  ChoreoEventTypeGemR = 2,
  ChoreoEventTypeRibbonL = 3,
  ChoreoEventTypeRibbonR = 4,
  ChoreoEventTypeDrumL = 5,
  ChoreoEventTypeDrumR = 6,
  ChoreoEventTypeDirGemL = 7,
  ChoreoEventTypeDirGemR = 8,
};

class ChoreoEvent {
public:
  ChoreoEventType type;
  bool hasGuide;
  BeatTime time;
  int beatDivision;
  Position position;
  std::vector<Position> subPositions;
  unsigned long broadcastEventID;

  ChoreoEvent(const Json::Value &j);

  [[nodiscard]] bool isLHS() const {
    switch (type) {
    case ChoreoEventTypeGemL:
    case ChoreoEventTypeRibbonL:
    case ChoreoEventTypeDrumL:
    case ChoreoEventTypeDirGemL:
      return true;
    default:
      return false;
    }
  }

  [[nodiscard]] bool isRHS() const {
    switch (type) {
    case ChoreoEventTypeGemR:
    case ChoreoEventTypeRibbonR:
    case ChoreoEventTypeDrumR:
    case ChoreoEventTypeDirGemR:
      return true;
    default:
      return false;
    }
  }
};

class Choreography {
public:
  std::string id;
  std::string name;
  BeatTime spawnAheadTime;
  int gemSpeed;
  std::vector<ChoreoEvent> events;

  Choreography(const Json::Value &j);

  [[nodiscard]] float secondsToMeters(float seconds) const { return seconds * static_cast<float>(gemSpeed); }
};

class TempoSection {
public:
  float startTimeInSeconds;
  int beatsPerMeasure;
  float beatsPerMinute;
  bool doesStartNewMeasure;

  TempoSection(const Json::Value &j);
};

struct Beat {
public:
  float time;
  float bpm;

  Beat(float time, float bpm) : time(time), bpm(bpm) {}
};

struct AuthorInfo {
public:
  std::string platformID;
  std::string displayName;
  std::string accountID;

  AuthorInfo(const Json::Value &j);
};

class AudioTripSong {
public:
  bool custom;
  AuthorInfo authorID;
  std::string songFilename;
  std::string songID;
  std::string title;
  std::string artist;
  std::string descriptor;
  std::string sceneName;
  float avgBPM;
  std::vector<TempoSection> tempoSections;

  float firstBeatTimeInSeconds;
  float songEndTimeInSeconds;
  float songShortLengthInSeconds;
  float songStartFadeTime;
  float songEndFadeTime;
  float leadingSilenceSeconds;

  std::vector<Choreography> choreographies;

  AudioTripSong(const Json::Value &j);

  static AudioTripSong fromJson(std::istream &is);

  static AudioTripSong fromFile(const std::string &path) {
    std::ifstream is;
    is.open(path);
    return fromJson(is);
  }

  [[nodiscard]] std::vector<Beat> computeBeats() const;
};

} // namespace audiotrip
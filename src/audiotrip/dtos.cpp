//
// Created by depau on 4/26/22.
//

#include "audiotrip/dtos.h"

namespace audiotrip {

BeatTime::BeatTime(const Json::Value &j) :
  beat(j["beat"].asInt()), numerator(j["numerator"].asInt()), denominator(j["denominator"].asInt()) {
}

Position::Position(const Json::Value &j) :
  std::tuple<float, float, float>(j["x"].asFloat(), j["y"].asFloat(), j["z"].asFloat()) {
}

ChoreoEvent::ChoreoEvent(const Json::Value &j) :
  type(static_cast<ChoreoEventType>(j["type"].asInt())),
  hasGuide(j["hasGuide"].asBool()),
  time(j["time"]),
  beatDivision(j["beatDivision"].asInt()),
  position(j["position"]),
  subPositions(fromJsonArray<Position>(j["subPositions"])),
  broadcastEventID(j["broadcastEventId"].asUInt64()) {
}

Choreography::Choreography(const Json::Value &j) :
  id(j["header"]["id"].asString()),
  name(j["header"]["name"].asString()),
  spawnAheadTime(j["header"]["spawnAheadTime"]),
  gemSpeed(j["header"]["gemSpeed"].asInt()),
  events(fromJsonArray<ChoreoEvent>(j["data"]["events"])) {
}

TempoSection::TempoSection(const Json::Value &j) :
  startTimeInSeconds(j["startTimeInSeconds"].asFloat()),
  beatsPerMeasure(j["beatsPerMeasure"].asInt()),
  beatsPerMinute(j["beatsPerMinute"].asFloat()),
  doesStartNewMeasure(j["doesStartNewMeasure"].asBool()) {
}

AuthorInfo::AuthorInfo(const Json::Value &j) :
  platformID(j["platformID"].asString()),
  displayName(j["displayName"].asString()),
  accountID(j["accountID"].asString()) {
}

AudioTripSong::AudioTripSong(const Json::Value &j) :
  custom(j["metadata"]["custom"].asBool()),
  authorID(j["metadata"]["authorID"]),
  songFilename(j["metadata"]["songFilename"].asString()),
  songID(j["metadata"]["songId"].asString()),
  title(j["metadata"]["title"].asString()),
  artist(j["metadata"]["artist"].asString()),
  descriptor(j["metadata"]["descriptor"].asString()),
  sceneName(j["metadata"]["sceneName"].asString()),
  avgBPM(j["metadata"]["avgBpm"].asFloat()),
  tempoSections(fromJsonArray<TempoSection>(j["metadata"]["tempoSections"])),
  firstBeatTimeInSeconds(j["metadata"]["firstBeatTimeInSeconds"].asFloat()),
  songEndTimeInSeconds(j["metadata"]["songEndTimeInSeconds"].asFloat()),
  songShortLengthInSeconds(j["metadata"]["songShortLengthInSeconds"].asFloat()),
  songStartFadeTime(j["metadata"]["songStartFadeTime"].asFloat()),
  songEndFadeTime(j["metadata"]["songEndFadeTime"].asFloat()),
  leadingSilenceSeconds(j["metadata"]["leadingSilenceSeconds"].asFloat()),
  choreographies(fromJsonArray<Choreography>(j["choreographies"]["list"])) {
}

AudioTripSong AudioTripSong::fromJson(std::istream &is) {
  Json::CharReaderBuilder builder;
  JSONCPP_STRING errs;

  Json::Value root;
  if (!Json::parseFromStream(builder, is, &root, &errs)) {
    // TODO: use exceptions maybe
    std::cerr << errs << std::endl;
    abort();
  }

  return root;
}

} // namespace audiotrip
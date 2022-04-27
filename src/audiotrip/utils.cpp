//
// Created by depau on 4/26/22.
//

#include "audiotrip/dtos.h"
#include "audiotrip/utils.h"

namespace audiotrip {

std::vector<Beat> AudioTripSong::computeBeats() const {
  assert(!tempoSections.empty());
  std::vector<Beat> result;

  auto it = tempoSections.begin();
  const TempoSection *ts;

  float accumulator = 0;
  while (it != tempoSections.end()) {
    ts = &*(it++);
    float sectionEndTime = it == tempoSections.end() ? songEndTimeInSeconds : it->startTimeInSeconds;
    float secondsPerBeat = 60.0f / ts->beatsPerMinute;

    while (accumulator < sectionEndTime) {
      result.emplace_back(accumulator, ts->beatsPerMinute);
      accumulator += secondsPerBeat;
    }
  }

  return result;
}

} // namespace audiotrip

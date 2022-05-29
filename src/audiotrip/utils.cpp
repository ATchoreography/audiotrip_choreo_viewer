//
// Created by depau on 4/26/22.
//

#include "audiotrip/dtos.h"
#include "audiotrip/utils.h"

namespace audiotrip {

std::vector<Beat> AudioTripSong::computeBeats() const {
  // Find the max beat used in the actual choreo since some choreos have out-of-bounds beats
  ssize_t maxBeat = 0;
  for (const Choreography &choreo : choreographies) {
    for (const ChoreoEvent &event : choreo.events) {
      if (event.time.beat > maxBeat)
        maxBeat = event.time.beat;
    }
  }

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

  // Always add an extra beat at the end just to be sure
  ssize_t toAdd = (maxBeat - static_cast<ssize_t>(result.size())) + 1;
  if (toAdd > 0) {
    for (int i = 0; i < toAdd; i++) {
      result.emplace_back(accumulator, tempoSections.back().beatsPerMinute);
      accumulator += 60.f / tempoSections.back().beatsPerMinute;
    }
  }

  return result;
}

} // namespace audiotrip

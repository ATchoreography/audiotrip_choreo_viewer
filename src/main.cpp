// STL includes
#include <iostream>
#include <optional>
#include <string_view>


// Local includes
// Libraries
#include "Application.h"

/*
 * Note: Y is UP! The song extends parallel to Z, arms point parallel to X
 */

// Barrier notes:
// - 20cm above the floor
// - 200cm wide
// - XY=0, angle 0 places the barrier facing down, bottom side at 180cm
// - XY=0, angle 90° places the barrier on the side, center tick is at 125cm
// - => reference point is in the middle, 55cm below the bottom side
// - Y position is subtracted, not added

int main(int argc, const char *argv[]) {
  std::optional<std::string> filename = std::nullopt;

  //  chdir("/home/depau/CLionProjects/AudioTrip-LevelViewer");

  if (argc > 1) {
    if (std::string_view(argv[1]) == "-h" || std::string_view(argv[1]) == "--help") {
      std::cout << "Usage: " << argv[0] << " [ats file] [--debug]" << std::endl;
      return 0;
    } else {
      filename = argv[1];
    }
  }

  bool debug = std::string_view(argv[argc - 1]) == "--debug";

  Application app(debug);
  app.main(filename);
  return 0;
}
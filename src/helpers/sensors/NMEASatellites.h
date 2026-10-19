#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// GGA counts satellites used for a fix; GSV counts satellites in view.
class NMEASatellites {
  struct View {
    char talker = 0;
    unsigned count = 0;
    uint32_t updated = 0;
  };
  View views[8];

public:
  // The caller must validate the NMEA checksum first.
  void update(const char* sentence, uint32_t now) {
    if (strlen(sentence) < 7 || sentence[0] != '$' ||
        (sentence[1] != 'G' && strncmp(sentence + 1, "BD", 2) != 0) ||
        strncmp(sentence + 3, "GSV,", 4) != 0) return;
    unsigned total, part, count;
    if (sscanf(sentence + 7, "%u,%u,%u", &total, &part, &count) != 3 ||
        total == 0 || part != 1 || count > 255) return;
    char talker = sentence[2] == 'D' ? 'B' : sentence[2]; // BeiDou: GB, GD or BD
    View* slot = nullptr;
    for (auto& view : views) {
      if (view.talker == talker) {
        slot = &view;
        break;
      }
      if (!slot && (view.talker == 0 || uint32_t(now - view.updated) > 10000)) slot = &view;
    }
    if (slot) {
      slot->talker = talker;
      slot->count = count;
      slot->updated = now;
    }
  }

  long count(uint32_t now) const {
    long sum = 0;
    bool seen = false;
    for (const auto& view : views) {
      if (view.talker == 0 || uint32_t(now - view.updated) > 10000) continue;
      if (view.talker == 'N') return view.count; // Combined GNSS, don't double count.
      sum += view.count;
      seen = true;
    }
    return seen ? sum : -1;
  }
};

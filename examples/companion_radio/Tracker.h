#pragma once

#include <stdint.h>
#include <stdio.h>

class Tracker {
  uint32_t last_location = 0;
  uint32_t changed_at = 0;
  uint32_t presses[2] = {};
  uint8_t press_count = 0;
  bool raw_pressed = false;
  bool stable_pressed = false;

public:
  void begin(uint32_t now) { last_location = now; }

  bool locationDue(uint32_t now) {
    if (uint32_t(now - last_location) < 3600000UL) return false;
    last_location = now;
    return true;
  }

  bool checkButton(bool pressed, uint32_t now) {
    if (pressed != raw_pressed) {
      raw_pressed = pressed;
      changed_at = now;
    }
    if (uint32_t(now - changed_at) < 30 || stable_pressed == raw_pressed) return false;
    stable_pressed = raw_pressed;
    if (!stable_pressed) return false;
    while (press_count && uint32_t(now - presses[0]) > 5000) {
      presses[0] = presses[1];
      press_count--;
    }
    if (press_count == 2) {
      press_count = 0;
      return true;
    }
    presses[press_count++] = now;
    return false;
  }

  static void formatLocation(char* out, size_t size, long lat, long lon, long hdop, long satellites) {
    char dilution[16], view[16];
    if (hdop < 0) {
      snprintf(dilution, sizeof(dilution), "unknown");
    } else {
      snprintf(dilution, sizeof(dilution), "%ld.%ld", hdop / 10, hdop % 10);
    }
    if (satellites < 0) {
      snprintf(view, sizeof(view), "unknown");
    } else {
      snprintf(view, sizeof(view), "%ld", satellites);
    }
    // Integer formatting also works on targets without printf float support.
    snprintf(out, size, "Location: lat=%s%ld.%06ld lon=%s%ld.%06ld HDOP=%s satellites_in_view=%s",
        lat < 0 ? "-" : "", (lat < 0 ? -lat : lat) / 1000000,
        (lat < 0 ? -lat : lat) % 1000000,
        lon < 0 ? "-" : "", (lon < 0 ? -lon : lon) / 1000000,
        (lon < 0 ? -lon : lon) % 1000000, dilution, view);
  }
};

#include "ClockfaceRegistry.h"

static std::vector<ClockfaceInfo> _registry;

void ClockfaceRegistry::registerClockface(const char* id, const char* name, ClockfaceFactory factory) {
  _registry.push_back({id, name, factory});
}

IClockface* ClockfaceRegistry::create(const char* id, Adafruit_GFX* display) {
  for (auto &e: _registry) {
    if (strcmp(e.id, id) == 0) return e.factory(display);
  }
  return nullptr;
}

const std::vector<ClockfaceInfo>& ClockfaceRegistry::list() {
  return _registry;
}
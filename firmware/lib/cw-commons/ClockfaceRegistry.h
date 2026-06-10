#pragma once
#include <vector>
#include <functional>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <IClockface.h>

using ClockfaceFactory = std::function<IClockface*(Adafruit_GFX*)>;

struct ClockfaceInfo {
  const char* id;
  const char* name;
  ClockfaceFactory factory;
};

class ClockfaceRegistry {
public:
  static void registerClockface(const char* id, const char* name, ClockfaceFactory factory);
  static IClockface* create(const char* id, Adafruit_GFX* display);
  static const std::vector<ClockfaceInfo>& list();
};
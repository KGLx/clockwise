#include "ClockfaceRegistry.h"

// 包含各个表盘的头（路径按项目实际位置）
// use relative path from src/ to clockfaces/
#include "../clockfaces/cw-cf-0x01/Clockface.h"
#include "../clockfaces/cw-cf-0x02/Clockface.h"
#include "../clockfaces/cw-cf-0x03/Clockface.h"
#include "../clockfaces/cw-cf-0x04/Clockface.h"
#include "../clockfaces/cw-cf-0x05/Clockface.h"
#include "../clockfaces/cw-cf-0x06/Clockface.h"

// 如果以后添加更多表盘，在此处包含并注册它们

void registerAllClockfaces() {
  ClockfaceRegistry::registerClockface("cw-cf-0x01", "Mario", [](Adafruit_GFX* d)->IClockface* {
    return new cw_cf_0x01::Clockface(d);
  });
  ClockfaceRegistry::registerClockface("cw-cf-0x02", "Words", [](Adafruit_GFX* d)->IClockface* {
    return new cw_cf_0x02::Clockface(d);
  });
  ClockfaceRegistry::registerClockface("cw-cf-0x03", "WorldMap", [](Adafruit_GFX* d)->IClockface* {
    return new cw_cf_0x03::Clockface(d);
  });
  ClockfaceRegistry::registerClockface("cw-cf-0x04", "ClockTower", [](Adafruit_GFX* d)->IClockface* {
    return new cw_cf_0x04::Clockface(d);
  });
  ClockfaceRegistry::registerClockface("cw-cf-0x05", "Pacman", [](Adafruit_GFX* d)->IClockface* {
    return new cw_cf_0x05::Clockface(d);
  });
  ClockfaceRegistry::registerClockface("cw-cf-0x06", "Pokedex", [](Adafruit_GFX* d)->IClockface* {
    return new cw_cf_0x06::Clockface(d);
  });
}
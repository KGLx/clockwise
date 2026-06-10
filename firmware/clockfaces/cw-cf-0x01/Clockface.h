#pragma once

#include <Arduino.h>

#include "gfx/Super_Mario_Bros__24pt7b.h"

#include <Adafruit_GFX.h>
#include <Tile.h>
#include <Locator.h>
#include <Game.h>
#include <Object.h>
#include <ImageUtils.h>
// Commons
#include <IClockface.h>
#include <CWDateTime.h>

#include "gfx/assets.h"
#include "gfx/mario.h"
#include "gfx/block.h"

namespace cw_cf_0x01 {
  class Clockface: public IClockface {
    private:
      Adafruit_GFX* _display;
      CWDateTime* _dateTime;
      void updateTime();

    public:
      Clockface(Adafruit_GFX* display);
      void setup(CWDateTime *dateTime);
      void update();
      void externalEvent(int type);

  };
} // namespace cw_cf_0x01

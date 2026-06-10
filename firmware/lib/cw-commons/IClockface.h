#pragma once

#include "CWDateTime.h"

class IClockface {
public:
  virtual ~IClockface() {}

  // 确保这些接口是 public
  virtual void setup(CWDateTime* dt) = 0;
  virtual void update() = 0;
  virtual void externalEvent(int type) = 0;
};

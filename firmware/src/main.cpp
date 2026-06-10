#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Clockface
#include <ClockfaceRegistry.h>
// Commons
#include <WiFiController.h>
#include <CWDateTime.h>
#include <CWPreferences.h>
#include <CWWebServer.h>
#include <StatusController.h>
#include <IClockface.h>

#define MIN_BRIGHT_DISPLAY_ON 4
#define MIN_BRIGHT_DISPLAY_OFF 0

#define ESP32_LED_BUILTIN 2

MatrixPanel_I2S_DMA *dma_display = nullptr;

IClockface *clockfaceInstance = nullptr;

WiFiController wifi;
CWDateTime cwDateTime;

bool autoBrightEnabled;
long autoBrightMillis = 0;
int currentBrightSlot = -1;
// 标记是否已经完成 cwDateTime.begin()
bool cwDateTimeInitialized = false;

bool isValidI2SSpeed(uint32_t speed) {
// -  return speed == 8000000 || speed == 16000000 || speed == 20000000;
  // 允许 2MHz / 4MHz / 8MHz / 16MHz / 20MHz（根据需要调整）
  return speed == 2000000 || speed == 4000000 || speed == 8000000
      || speed == 16000000 || speed == 20000000;
}

bool isValidDriver(uint32_t drv) {
  return drv >= 0 && drv <= 5;
}



void displaySetup(bool swapBlueGreen, bool swapBlueRed, uint8_t displayBright, uint8_t displayRotation, uint8_t driver, uint32_t i2cSpeed, uint8_t E_pin)
{
  HUB75_I2S_CFG mxconfig(64, 64, 1);

  if (swapBlueGreen)
  {
    // Swap Blue and Green pins because the panel is RBG instead of RGB.
    mxconfig.gpio.b1 = 26;
    mxconfig.gpio.b2 = 12;
    mxconfig.gpio.g1 = 27;
    mxconfig.gpio.g2 = 13;
  }

  if (swapBlueRed)
  {
    // Swap Blue and Red pins. 
    mxconfig.gpio.b1 = 25;
    mxconfig.gpio.b2 = 14;
    mxconfig.gpio.r1 = 27;
    mxconfig.gpio.r2 = 13;
  }

  mxconfig.gpio.e = E_pin;
  mxconfig.clkphase = false;

  if (isValidDriver(driver)) {
    mxconfig.driver = static_cast<HUB75_I2S_CFG::shift_driver>(driver);
  } else {
    Serial.printf("[ERROR] Invalid driver from config:%d\n", driver);
  }
  if (isValidI2SSpeed(i2cSpeed)) {
    mxconfig.i2sspeed = static_cast<HUB75_I2S_CFG::clk_speed>(i2cSpeed);
  } else {
    Serial.printf("[ERROR] Invalid I2S speed from config:%d\n", i2cSpeed);
  }

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(displayBright);
  dma_display->clearScreen();
  dma_display->setRotation(displayRotation);
}

void automaticBrightControl()
{
  if (autoBrightEnabled) {
    if (millis() - autoBrightMillis > 3000)
    {
      int16_t currentValue = analogRead(ClockwiseParams::getInstance()->ldrPin);

      uint16_t ldrMin = ClockwiseParams::getInstance()->autoBrightMin;
      uint16_t ldrMax = ClockwiseParams::getInstance()->autoBrightMax;

      const uint8_t minBright = (currentValue < ldrMin ? MIN_BRIGHT_DISPLAY_OFF : MIN_BRIGHT_DISPLAY_ON);
      uint8_t maxBright = ClockwiseParams::getInstance()->displayBright;

      uint8_t slots = 10; //10 slots
      uint8_t mapLDR = map(currentValue > ldrMax ? ldrMax : currentValue, ldrMin, ldrMax, 1, slots);
      uint8_t mapBright = map(mapLDR, 1, slots, minBright, maxBright);

      // Serial.printf("LDR: %d, mapLDR: %d, Bright: %d\n", currentValue, mapLDR, mapBright);
      if(abs(currentBrightSlot - mapLDR ) >= 2 || mapBright == 0){
           dma_display->setBrightness8(mapBright);
           currentBrightSlot=mapLDR;
          //  Serial.printf("setBrightness: %d , Update currentBrightSlot to %d\n", mapBright, mapLDR);
      }
      autoBrightMillis = millis();
    }
  }
}

// 立即应用亮度（可直接在 web 回调中调用）
void applyDisplayBrightness(uint8_t b) {
    ClockwiseParams::getInstance()->displayBright = b; // 更新偏好缓存（可选择同时 save）
    if (dma_display) {
        dma_display->setBrightness8(b); // 立即生效
        Serial.printf("Brightness applied: %u\n", b);
    }
    // 如果用户手动设置亮度，通常希望禁用自动亮度或重置其计时器
    autoBrightEnabled = false;           // 禁用自动亮度（可改为只重置计时器）
    currentBrightSlot = -1;              // 让之后的 auto 算法重新计算基线
    autoBrightMillis = millis();
}

static void createClockfaceFromPref() {
  if (clockfaceInstance) {
    delete clockfaceInstance;
    clockfaceInstance = nullptr;
  }

  const char* cfId = ClockwiseParams::getInstance()->clockfaceName.c_str();
  Serial.printf("[CF] trying to create clockface '%s'\n", cfId);

  // 打印 registry 内容，帮助调试
  const auto &reg = ClockfaceRegistry::list();
  Serial.printf("[CF] registry count: %u\n", (unsigned)reg.size());
  for (size_t i = 0; i < reg.size(); ++i) {
    Serial.printf("  [%u] id=%s name=%s\n", (unsigned)i, reg[i].id, reg[i].name);
  }

  // 先尝试用偏好里的 id 创建
  clockfaceInstance = ClockfaceRegistry::create(cfId, dma_display);
  if (!clockfaceInstance) {
    if (!reg.empty()) {
      // 回退到 registry 的第一个表盘（确保能创建）
      Serial.printf("[CF] unknown clockface id '%s', falling back to '%s'\n", cfId, reg[0].id);
      ClockwiseParams::getInstance()->clockfaceName = String(reg[0].id);
      ClockwiseParams::getInstance()->save();
      clockfaceInstance = ClockfaceRegistry::create(reg[0].id, dma_display);
      if (!clockfaceInstance) {
        Serial.println("[CF] fallback creation failed unexpectedly");
        return;
      }
    } else {
      Serial.println("[CF] registry is empty — no clockfaces registered. Aborting create.");
      return;
    }
  }

  // 只有当时间服务已就绪才调用 setup（避免依赖未初始化资源）
  if (clockfaceInstance && cwDateTimeInitialized) {
    clockfaceInstance->setup(&cwDateTime);
  } else if (clockfaceInstance) {
    Serial.println("[CF] created instance but cwDateTime not initialized yet");
  }
}

// 声明注册函数（定义在上面的新文件里）
extern void registerAllClockfaces();

void setup()
{
  Serial.begin(115200);
  pinMode(ESP32_LED_BUILTIN, OUTPUT);

  StatusController::getInstance()->blink_led(5, 100);

  ClockwiseParams::getInstance()->load();
  Serial.printf("DEBUG: forcing i2s=%u, driver=%u, E_pin=%u\n",
                ClockwiseParams::getInstance()->i2cSpeed,
                ClockwiseParams::getInstance()->driver,
                ClockwiseParams::getInstance()->E_pin);
  pinMode(ClockwiseParams::getInstance()->ldrPin, INPUT);

  uint8_t driver = ClockwiseParams::getInstance()->driver;
  uint32_t i2cSpeed = ClockwiseParams::getInstance()->i2cSpeed;
  uint8_t E_pin = ClockwiseParams::getInstance()->E_pin;
  
  displaySetup(ClockwiseParams::getInstance()->swapBlueGreen, ClockwiseParams::getInstance()->swapBlueRed, ClockwiseParams::getInstance()->displayBright, ClockwiseParams::getInstance()->displayRotation, driver, i2cSpeed, E_pin);
  // don't instantiate a global-named Clockface directly; use registry factory
  // createClockfaceFromPref will construct the selected clockface (from prefs)
  // and handle deletion of any previous instance.
  // (createClockfaceFromPref is called again later, but call it here to ensure display is initialized)
  // 显式注册所有表盘，保证 registry 非空
  registerAllClockfaces();

  // 之后再创建表盘实例
  createClockfaceFromPref();

  // 在 webserver 偏好变化时切换表盘（CWWebServer 中已声明 onPreferenceChanged）
  ClockwiseWebServer::getInstance()->onPreferenceChanged = [](const String& key, const String& val){
    Serial.printf("[PREF] Changed: %s = %s\n", key.c_str(), val.c_str());
    if (key == String(ClockwiseParams::getInstance()->PREF_CLOCKFACE)) {
      ClockwiseParams::getInstance()->clockfaceName = val;
      ClockwiseParams::getInstance()->save();
      createClockfaceFromPref();
    } else if (key == String(ClockwiseParams::getInstance()->PREF_DISPLAY_BRIGHT)) {
      uint8_t b = val.toInt();
      dma_display->setBrightness8(b);
      Serial.printf("[PREF] Brightness updated to %d\n", b);
    } else if (key == String(ClockwiseParams::getInstance()->PREF_TIME_ZONE)
            || key == String(ClockwiseParams::getInstance()->PREF_USE_24H_FORMAT)
            || key == String(ClockwiseParams::getInstance()->PREF_NTP_SERVER)
            || key == String(ClockwiseParams::getInstance()->PREF_MANUAL_POSIX)) {
      // Ensure in-memory prefs reflect saved values
      ClockwiseParams::getInstance()->load();

      // Reinitialize time subsystem with new settings
      Serial.println("[PREF] Applying time settings at runtime...");
      cwDateTime.begin(
          ClockwiseParams::getInstance()->timeZone.c_str(),
          ClockwiseParams::getInstance()->use24hFormat,
          ClockwiseParams::getInstance()->ntpServer.c_str(),
          ClockwiseParams::getInstance()->manualPosix.c_str()
      );

      // Re-run clockface setup so it uses updated cwDateTime formatting/settings immediately
      if (clockfaceInstance) {
        clockfaceInstance->setup(&cwDateTime);
        Serial.println("[PREF] Clockface re-setup with new time settings.");
      }
    }
  };

  autoBrightEnabled = (ClockwiseParams::getInstance()->autoBrightMax > 0);

  StatusController::getInstance()->clockwiseLogo();
  delay(1000);

  StatusController::getInstance()->wifiConnecting();
  if (wifi.begin())
  {
    StatusController::getInstance()->ntpConnecting();
    cwDateTime.begin(ClockwiseParams::getInstance()->timeZone.c_str(), 
        ClockwiseParams::getInstance()->use24hFormat, 
        ClockwiseParams::getInstance()->ntpServer.c_str(),
        ClockwiseParams::getInstance()->manualPosix.c_str());
    // 标记时间服务已就绪，并确保已创建的表盘完成 setup
    cwDateTimeInitialized = true;
    if (clockfaceInstance) {
      clockfaceInstance->setup(&cwDateTime);
    }
  }

}

void loop()
{
  wifi.handleImprovWiFi();

  if (wifi.isConnected())
  {
    ClockwiseWebServer::getInstance()->handleHttpRequest();
    ezt::events();
  }

  if (wifi.connectionSucessfulOnce)
  {
    if (clockfaceInstance) clockfaceInstance->update();
  }

  automaticBrightControl();
}
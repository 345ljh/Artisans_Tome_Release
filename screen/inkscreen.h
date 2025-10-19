#ifndef _INKSCREEN_H
#define _INKSCREEN_H

#include "Arduino.h"
#include "SPI.h"
#include "stdint.h"

#define INKSCREEN_WIDTH 400
#define INKSCREEN_HEIGHT 300

class InkScreen{
private:
  uint8_t dc_pin;
  uint8_t rst_pin;
  uint8_t mosi_pin;
  uint8_t clk_pin;
  uint8_t cs_pin;
  uint8_t busy_pin;
  
  SPIClass spi;
  void SoftSpiWrite(uint8_t data);

  void WaitBusy();
  void WriteCmd(uint8_t cmd);
  void WriteData(uint8_t data);
  void TurnOnDisplay();

public:
  InkScreen(uint8_t mosi_pin, uint8_t clk_pin, uint8_t cs_pin, uint8_t dc_pin, uint8_t rst_pin, uint8_t busy_pin);
  void Reset();
  void Sleep();
  void Init();
  void Clear();
  void SetRandom();
  void SetImage(uint8_t* buffer);
};

#endif
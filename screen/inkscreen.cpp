#include "inkscreen.h"

InkScreen::InkScreen(uint8_t mosi_pin, uint8_t clk_pin, uint8_t cs_pin, 
  uint8_t dc_pin, uint8_t rst_pin, uint8_t busy_pin){
    this->dc_pin = dc_pin;
    this->rst_pin = rst_pin;
    this->mosi_pin = mosi_pin;
    this->clk_pin = clk_pin;
    this->cs_pin = cs_pin;
    this->busy_pin = busy_pin;

    // gpio设置
    pinMode(this->dc_pin, OUTPUT);
    digitalWrite(this->dc_pin, HIGH);
    pinMode(this->rst_pin, OUTPUT);
    digitalWrite(this->rst_pin, HIGH);
    pinMode(this->mosi_pin, OUTPUT);
    digitalWrite(this->mosi_pin, HIGH);
    pinMode(this->clk_pin, OUTPUT);
    digitalWrite(this->clk_pin, LOW);
    pinMode(this->cs_pin, OUTPUT);
    digitalWrite(this->cs_pin, HIGH);
    pinMode(this->busy_pin, INPUT);
    // digitalWrite(this->busy_pin, LOW);

    // // spi初始化
    // spi = SPIClass(VSPI);
    // spi.begin(this->clk_pin, 16, this->mosi_pin, this->cs_pin);
}

void InkScreen::SoftSpiWrite(uint8_t data){
  digitalWrite(clk_pin, LOW);
  for(int i = 7; i >= 0; i--){
    digitalWrite(mosi_pin, (data & (1 << i)) ? HIGH : LOW);
    // delayMicroseconds(1);
    digitalWrite(clk_pin, HIGH);
    delayMicroseconds(1);
    digitalWrite(clk_pin, LOW);
    delayMicroseconds(1);
  }
  delayMicroseconds(1);
}

void InkScreen::Reset(){
  digitalWrite(rst_pin, HIGH);
  delay(100);
  digitalWrite(rst_pin, LOW);
  delay(2);
  digitalWrite(rst_pin, HIGH);
  delay(100);
}

void InkScreen::WaitBusy(){
  while(digitalRead(busy_pin) == 1){
    delay(10);
  }
  // delay(200);
}

void InkScreen::WriteCmd(uint8_t cmd){
  digitalWrite(dc_pin, LOW);
  digitalWrite(cs_pin, LOW);
  SoftSpiWrite(cmd);
  digitalWrite(cs_pin, HIGH);
}

void InkScreen::WriteData(uint8_t data){
  digitalWrite(dc_pin, HIGH);
  digitalWrite(cs_pin, LOW);
  SoftSpiWrite(data);
  digitalWrite(cs_pin, HIGH);
}

void InkScreen::Sleep(){
  WriteCmd(0x10);
  WriteData(0x01);
}

void InkScreen::Init(){
  Reset();
  WaitBusy();

  WriteCmd(0x12);
  WaitBusy();

  WriteCmd(0x21);
  WriteData(0x40);
  WriteData(0x00);

  WriteCmd(0x3c);
  WriteData(0x05);

  WriteCmd(0x11);
  WriteData(0x03);

  WriteCmd(0x44);
  WriteData(0x00);
  WriteData(0x31);

  WriteCmd(0x45);
  WriteData(0x00);
  WriteData(0x00);
  WriteData(0x2b);
  WriteData(0x01);

  WriteCmd(0x4e);
  WriteData(0x00);

  WriteCmd(0x4f);
  WriteData(0x00);
  WriteData(0x00);

  WaitBusy();
}

void InkScreen::TurnOnDisplay(){
  WriteCmd(0x22);
  WriteData(0xf7);
  WriteCmd(0x20);
  delay(1);
  WaitBusy();
}

void InkScreen::SetRandom(){
  uint16_t width = (INKSCREEN_WIDTH % 8 == 0)? (INKSCREEN_WIDTH / 8 ): (INKSCREEN_WIDTH / 8 + 1);
  uint16_t height = INKSCREEN_HEIGHT;

  WriteCmd(0x24);
  for(uint16_t i = 0; i < width * height; i++){
    WriteData(rand() % 256);
  }
  WriteCmd(0x26);
  for(uint16_t i = 0; i < width * height; i++){
    WriteData(rand() % 256);
  }

  TurnOnDisplay();

}

void InkScreen::Clear(){
  uint16_t width = (INKSCREEN_WIDTH % 8 == 0)? (INKSCREEN_WIDTH / 8 ): (INKSCREEN_WIDTH / 8 + 1);
  uint16_t height = INKSCREEN_HEIGHT;

  WriteCmd(0x24);
  for(uint16_t i = 0; i < width * height; i++){
    WriteData(0xFF);
  }
  WriteCmd(0x26);
  for(uint16_t i = 0; i < width * height; i++){
    WriteData(0xFF);
  }

  TurnOnDisplay();
}

void InkScreen::SetImage(uint8_t* buffer){
  uint16_t width = (INKSCREEN_WIDTH % 8 == 0)? (INKSCREEN_WIDTH / 8 ): (INKSCREEN_WIDTH / 8 + 1);
  uint16_t height = INKSCREEN_HEIGHT;

  WriteCmd(0x24);
  for(uint16_t i = 0; i < width * height; i++){
    // Serial.println(*(buffer + i));
    WriteData(*(buffer + i));
  }
  WriteCmd(0x26);
  for(uint16_t i = 0; i < width * height; i++){
    // WriteData(~(*(buffer + i)));
    // WriteData(~(*(buffer + i + 15000)));
    WriteData(0xFF);
  }

  TurnOnDisplay();
}
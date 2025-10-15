#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2C_SDA 21
#define I2C_SCL 22

uint8_t addr = 0x27;
LiquidCrystal_I2C* lcd = nullptr;

bool probe(uint8_t a) {
  Wire.beginTransmission(a);
  return (Wire.endTransmission() == 0);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println(F("\nLCD 1602 I2C Test (LiquidCrystal_I2C)"));

  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.printf("I2C on SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

  if (!probe(0x27) && !probe(0x3F)) {
    Serial.println(F("❌ No LCD at 0x27 or 0x3F. Check wiring/contrast or run an I2C scanner."));
    while (true) delay(1000);
  }
  addr = probe(0x27) ? 0x27 : 0x3F;

  lcd = new LiquidCrystal_I2C(addr, 16, 2);
  lcd->init();
  lcd->backlight();

  lcd->clear();
  lcd->setCursor(0, 0); lcd->print("Aquarium Ctrl");
  lcd->setCursor(0, 1); lcd->print("LCD @ 0x"); lcd->print(addr, HEX);
  delay(1500);
  lcd->clear();
}

void loop() {
  static uint32_t t0 = millis();
  static uint32_t n = 0;

  lcd->setCursor(0, 0); lcd->print("Hello 1602 I2C ");
  lcd->setCursor(0, 1); lcd->print("Count: "); lcd->print(n++); lcd->print("     ");

  while (millis() - t0 < 500) delay(1);
  t0 = millis();
}
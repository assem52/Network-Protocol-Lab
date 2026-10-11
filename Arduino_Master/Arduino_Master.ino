/*
 * Arduino_Master.ino
 * 
 * Arduino Uno = SPI Master + I2C Master (RTC DS1307)
 * 
 * Connections:
 *   DS1307 RTC  ->  Arduino Uno
 *     SDA (pin 6 on RTC) -> A4 (SDA)
 *     SCL (pin 5 on RTC) -> A5 (SCL)
 *     VCC -> 5V,  GND -> GND
 * 
 *   ATmega32 (Slave) -> Arduino Uno
 *     MOSI (PB5) <- Arduino D11 (MOSI)
 *     MISO (PB6) -> Arduino D12 (MISO)
 *     SCK  (PB7) <- Arduino D13 (SCK)
 *     SS   (PB4) <- Arduino D10 (SS)
 *     GND  <->    GND
 * 
 * Flow:
 *   1. Read date/time from DS1307 over I2C.
 *   2. Send all 7 bytes (seconds, minutes, hours, day, date, month, year)
 *      to the ATmega32 slave one byte at a time over SPI.
 *   3. Print the time on the Serial Monitor over UART.
 */

#include <Wire.h>          // I2C - communicates with RTC module
#include <SPI.h>           // SPI  - communicates with ATmega32 slave

// ---- DS1307 I2C Address ----
#define DS1307_ADDR   0x68

// ---- SPI Slave Select pin ----
#define SLAVE_SS_PIN  10

// ---- RTC Global Variables ----
uint8_t rtc_seconds = 0;
uint8_t rtc_minutes = 0;
uint8_t rtc_hours   = 0;
uint8_t rtc_day     = 1;      // 1=Mon ... 7=Sun
uint8_t rtc_date    = 1;
uint8_t rtc_month   = 1;
uint8_t rtc_year    = 26;     // e.g. 26 for 2026

// ---- BCD helper ----
// DS1307 stores values in BCD (packed), convert to normal decimal
uint8_t bcd2dec(uint8_t bcd) {
  return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

// ---- Read time from DS1307 via I2C ----
void readRTC() {
  Wire.beginTransmission(DS1307_ADDR);
  Wire.write(0x00);              // start from register 0
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDR, 7);
  if (Wire.available() >= 7) {
    rtc_seconds = bcd2dec(Wire.read() & 0x7F);  // mask CH bit
    rtc_minutes = bcd2dec(Wire.read());
    rtc_hours   = bcd2dec(Wire.read() & 0x3F);  // mask 12/24 bit
    rtc_day     = bcd2dec(Wire.read());
    rtc_date    = bcd2dec(Wire.read());
    rtc_month   = bcd2dec(Wire.read());
    rtc_year    = bcd2dec(Wire.read());
  }
}

// ---- Send one byte to ATmega32 via SPI ----
uint8_t spiTransfer(uint8_t data) {
  digitalWrite(SLAVE_SS_PIN, LOW);
  delayMicroseconds(10);               // small settling time
  uint8_t received = SPI.transfer(data);
  delayMicroseconds(10);
  digitalWrite(SLAVE_SS_PIN, HIGH);
  delay(5);                            // gap between bytes for slave
  return received;
}

// ---- Send full RTC packet to ATmega32 via SPI ----
void sendRTCtoSlave() {
  spiTransfer(rtc_seconds);
  spiTransfer(rtc_minutes);
  spiTransfer(rtc_hours);
  spiTransfer(rtc_day);
  spiTransfer(rtc_date);
  spiTransfer(rtc_month);
  spiTransfer(rtc_year);
}

// ---- Day-of-week lookup ----
const char* dayName(uint8_t d) {
  const char* days[] = { "---", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
  if (d >= 1 && d <= 7) return days[d];
  return days[0];
}

// ===================================================================
void setup() {
  Serial.begin(9600);         // UART -> Serial Monitor
  Wire.begin();               // I2C  -> RTC
  SPI.begin();                // SPI  -> ATmega32 Slave
  SPI.setClockDivider(SPI_CLOCK_DIV16);   // Fosc/16 -> matches ATmega32 slave config
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  pinMode(SLAVE_SS_PIN, OUTPUT);
  digitalWrite(SLAVE_SS_PIN, HIGH);       // deselect slave initially

  Serial.println("=== Arduino SPI Master + DS1307 RTC ===");
}

void loop() {
  // 1. Read from RTC module via I2C
  readRTC();

  // 2. Print over UART (Serial Monitor)
  Serial.print("Time: ");
  if (rtc_hours   < 10) Serial.print('0'); Serial.print(rtc_hours);   Serial.print(':');
  if (rtc_minutes < 10) Serial.print('0'); Serial.print(rtc_minutes); Serial.print(':');
  if (rtc_seconds < 10) Serial.print('0'); Serial.print(rtc_seconds);

  Serial.print("  |  Date: ");
  Serial.print(dayName(rtc_day)); Serial.print(' ');
  if (rtc_date  < 10) Serial.print('0'); Serial.print(rtc_date);  Serial.print('/');
  if (rtc_month < 10) Serial.print('0'); Serial.print(rtc_month); Serial.print("/20");
  if (rtc_year  < 10) Serial.print('0'); Serial.println(rtc_year);

  // 3. Send to ATmega32 Slave over SPI
  sendRTCtoSlave();

  delay(1000);   // update every second
}

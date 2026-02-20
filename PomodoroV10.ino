#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <SPI.h>

// Smooth Fonts
#include <Fonts/FreeSansBold24pt7b.h> 
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#define TFT_DC  0
#define TFT_RST 1
#define TFT_SCL 4
#define TFT_SDA 6
#define TFT_CS  7
#define I2C_SDA 8
#define I2C_SCL 9
#define BUZZER_PIN 10

Adafruit_GC9A01A tft = Adafruit_GC9A01A(TFT_CS, TFT_DC, TFT_RST);
Adafruit_MPU6050 mpu;

long remainingTime = 5 * 60; 
long totalStartTime = 5 * 60; 
unsigned long lastTick = 0;
int lastRotation = -1;

// Alarm Control
bool alarmActive = false;
int beepCount = 0;
unsigned long lastBeepTime = 0;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  Wire.begin(I2C_SDA, I2C_SCL);
  mpu.begin();
  SPI.begin(TFT_SCL, -1, TFT_SDA, TFT_CS); 
  tft.begin();
  
  // Set initial rotation to 3 (270 degrees)
  tft.setRotation(3);
  tft.fillScreen(GC9A01A_BLACK);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  int currentRotation = lastRotation;

  /* 
     Rotation Logic shifted by 270 degrees:
     Original Y+ is now X-
     Original Y- is now X+
     Original X+ is now Y-
     Original X- is now Y+
  */
  if (a.acceleration.x < -7.0)       currentRotation = 0; 
  else if (a.acceleration.x > 7.0)  currentRotation = 2; 
  else if (a.acceleration.y < -7.0)  currentRotation = 1; 
  else if (a.acceleration.y > 7.0)   currentRotation = 3; 

  if (currentRotation != lastRotation && currentRotation != -1) {

    lastRotation = currentRotation;
    tft.setRotation(lastRotation);
    
    // Timer values mapped to the new orientations
    if (lastRotation == 0)      totalStartTime = 25 * 60;
    else if (lastRotation == 2) totalStartTime = 5 * 60;
    else if (lastRotation == 1) totalStartTime = 50 * 60;
    else if (lastRotation == 3) totalStartTime = 15 * 60;
    
    remainingTime = totalStartTime;
    alarmActive = false;
    beepCount = 0; 
    noTone(BUZZER_PIN);
    tft.fillScreen(GC9A01A_BLACK);
  }

  unsigned long currentMillis = millis();

  if (remainingTime > 0) {
    if (currentMillis - lastTick >= 1000) {
      lastTick = currentMillis;
      remainingTime--;
      drawTimer();
    }
  } else {
    triggerAlarm(currentMillis);
  }
}

void drawPerimeterBar() {
  int centerX = 120, centerY = 120;
  int outerRadius = 120, innerRadius = 108; 
  
  float progress = (float)remainingTime / (float)totalStartTime;
  float endAngle = progress * 360.0;

  for (float i = 0; i < 360; i += 0.1) {
    float rad = (i - 90) * 0.0174533; 
    float cosRad = cos(rad);
    float sinRad = sin(rad);
    int xStart = centerX + (cosRad * innerRadius);
    int yStart = centerY + (sinRad * innerRadius);
    int xEnd = centerX + (cosRad * outerRadius);
    int yEnd = centerY + (sinRad * outerRadius);
    
    uint16_t color = (i <= endAngle) ? GC9A01A_CYAN : 0x10A2; 
    tft.drawLine(xStart, yStart, xEnd, yEnd, color);
  }
}

void drawTimer() {
  drawPerimeterBar();

  char buf[10];
  sprintf(buf, "%02d:%02d", (int)(remainingTime / 60), (int)(remainingTime % 60));

  int16_t x1, y1;
  uint16_t w, h;
  
  tft.setFont(&FreeSansBold24pt7b);
  tft.getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
  
  tft.fillRect(120 - (w/2) - 8, 120 - (h/2) - 8, w + 16, h + 16, GC9A01A_BLACK);
  
  tft.setCursor(120 - (w / 2), 120 + (h / 2) - 5); 
  tft.setTextColor(GC9A01A_WHITE);
  tft.print(buf);

  tft.setFont(&FreeSans9pt7b);
  tft.getTextBounds("RUNNING", 0, 0, &x1, &y1, &w, &h);
  tft.fillRect(120 - (w/2) - 5, 160, w + 10, h + 10, GC9A01A_BLACK);
  tft.setCursor(120 - (w / 2), 175);
  tft.setTextColor(GC9A01A_CYAN);
  tft.print("RUNNING");
}

void triggerAlarm(unsigned long now) {
  if (!alarmActive) {
    tft.fillScreen(GC9A01A_RED);
    const char* msg = "TIME UP!";
    
    int16_t x1, y1;
    uint16_t w, h;
    tft.setFont(&FreeSansBold18pt7b);
    tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
    
    tft.setCursor(120 - (w / 2), 120 + (h / 2));
    tft.setTextColor(GC9A01A_WHITE);
    tft.print(msg);
    
    alarmActive = true;
    lastBeepTime = now;
  }
  
  if (beepCount < 10) {
    if (now - lastBeepTime >= 1000) {
      lastBeepTime = now;
      tone(BUZZER_PIN, 2000, 500); 
      beepCount++;
    }
  } else {
    noTone(BUZZER_PIN);
  }
}
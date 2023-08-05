#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "bitmaps.h"
#include "ringtones.h"
#include "time.h"
#include "nokia4pt7b.h"
#include <Fonts/Picopixel.h>
#include "SIM800L.h"

// Constants for WiFi and NTP
#define WIFI_SSID "TP-LINK_E924"
#define WIFI_PASS "76605567"
#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 7200
#define UTC_OFFSET_DST 0

// Pins and ID's for the buttons, buzzer and speaker
#define SELECT_BTN_PIN 22
#define SELECT_BTN 0
#define UP_BTN_PIN 21
#define UP_BTN 1
#define DOWN_BTN_PIN 25
#define DOWN_BTN 2
#define DELETE_BTN_PIN 26
#define DELETE_BTN 3
#define BUZZER_PIN 19

// LCD and GSM module setup
#define LCD_SCLK 18
#define LCD_DIN 23
#define LCD_DC 4
#define LCD_CS 15
#define LCD_RST 2
#define GSM_TX 17
#define GSM_RX 16

// Objects and variables
Adafruit_PCD8544 display(LCD_SCLK, LCD_DIN, LCD_DC, LCD_CS, LCD_RST);
GSM gsm(Serial2);
int currentScreen = 0;
int currentMenuPage = 1;
int currentApp = 0;
int animProgress = 0;
bool isTextBoxActive = false;
uint8_t wifiConnectTries = 0;
const char compile_date[] = __DATE__ " " __TIME__;

#define homeScreen 0
#define menuScreen 1
#define activeAppS 2

void setup() {
  pinMode(SELECT_BTN_PIN, INPUT_PULLUP);
  pinMode(UP_BTN_PIN, INPUT_PULLUP);
  pinMode(DOWN_BTN_PIN, INPUT_PULLUP);
  pinMode(DELETE_BTN_PIN, INPUT_PULLUP);
  tone(BUZZER_PIN, 400, 100);

  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Connect to WiFi and get the current time from NTP server
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Trying to connect to WiFi...");
    delay(500);
    wifiConnectTries++;
    if (wifiConnectTries > 10) {
      break;
    }
  }
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  Serial2.begin(115200);

  // Initialize the display
  display.begin();
  display.setContrast(60);
  display.setBias(4);
  display.setRotation(2);
  display.setFont(&nokia4pt7b);
  display.setTextSize(1);
  display.setTextColor(1);

  // Display the startup animation
  for (int i = 140; i < 147; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, bitmaps[i], 84, 48, 1);
    display.display();
    delay(125);
  }

  // Display the startup message
  display.clearDisplay();
  display.setCursor(0, 6);
  display.println("ONPinit");
  display.setCursor(45, 6);
  display.println(getFormattedTime());

  // Check GSM module availability and put it to sleep if available
  if (gsm.ping())
    gsm.begin();
  
  setCpuFrequencyMhz(80);
}

void drawSelectButton(char* text = "Select") {
  // int16_t tmpX = display.getCursorX();
  // int16_t tmpY = display.getCursorY();

  // if (text == "Menu") int width = 42 - 13 // custom rules because i can't deal with variable length fonts :joy:

  display.setCursor(42 - (strlen(text) * 6) / 2, 47);
  display.println(text);

  // display.setCursor(tmpX, tmpY)
}

void drawCurrentMenuPageNumber() {
  // int16_t tmpX = display.getCursorX();
  // int16_t tmpY = display.getCursorY();

  display.setCursor(76, 6);
  display.println(currentMenuPage);

  // display.setCursor(tmpX, tmpY)
}

unsigned long previousMillis = 0;
bool nonBlockingDelay(unsigned long delayTime) {
  return (millis() - previousMillis >= delayTime) ? (previousMillis = millis(), true) : false;
}

void drawHomeScreen() {
  // draw the wallpaper
  display.drawBitmap(0, 0, wallpaper, 84, 48, 1);

  // connectivity and battery icons
  display.drawBitmap(0,  42, cell_icon,      5, 6, 1);
  display.drawBitmap(6,  42, wifi_icon,      6, 6, 1);
  display.drawBitmap(13, 42, battery_icon,   4, 6, 1);
  display.drawBitmap(18, 42, bluetooth_icon, 5, 6, 1);

  // bluetooth cross icon
  display.drawBitmap(18, 36, nosig_bitmap, 5, 5, 1);

  // bluetooth sleep icon
  // display.drawBitmap(18, 22, sleep_bitmap, 5, 19, 1);

  // wifi signal bars
  if (WiFi.getSleep() == 2)
    display.drawBitmap(6, 22, sleep_bitmap, 5, 19, 1);
  else {
    int wifiSignalStrength = WiFi.RSSI();
    if (wifiSignalStrength < -90 || wifiSignalStrength > 0)
      display.drawBitmap(6, 36, nosig_bitmap, 5, 5, 1);
    else {
      if (wifiSignalStrength > -80)
        display.drawBitmap(6, 35, bitmap, 2, 6, 1);
      if (wifiSignalStrength > -70)
        display.drawBitmap(6, 26, bitmap, 2, 8, 1);
      if (wifiSignalStrength > -55)
        display.drawBitmap(6, 17, bitmap, 3, 8, 1);
      if (wifiSignalStrength > -40)
        display.drawBitmap(6, 8, bitmap, 4, 8, 1);
    }
  }

  // cell signal bars
  if (gsm.ping()) {
    if (gsm.getSleep() != 0)
      display.drawBitmap(0, 22, sleep_bitmap, 5, 19, 1);
    else {
      int8_t cellSignStrg = gsm.getCsq();
      if (cellSignStrg == 0 || cellSignStrg > 40)
        display.drawBitmap(0, 36, nosig_bitmap, 5, 5, 1);
      else {
        if (cellSignStrg > 1)
          display.drawBitmap(0, 35, bitmap, 2, 6, 1);
        if (cellSignStrg > 10)
          display.drawBitmap(0, 26, bitmap, 2, 8, 1);
        if (cellSignStrg > 15)
          display.drawBitmap(0, 17, bitmap, 3, 8, 1);
        if (cellSignStrg > 20)
          display.drawBitmap(0, 8, bitmap, 4, 8, 1);
      }
    }
  } else
    display.drawBitmap(0, 36, nosig_bitmap, 5, 5, 1);


  // battery power
  int batteryPercentage, voltage;
  gsm.getBatteryData(batteryPercentage, voltage);
  if (batteryPercentage > 90)
    display.drawBitmap(13, 8,  bitmap, 3, 8, 1);
  if (batteryPercentage > 70)
    display.drawBitmap(13, 17, bitmap, 3, 8, 1);
  if (batteryPercentage > 45)
    display.drawBitmap(13, 26, bitmap, 2, 8, 1);
  if (batteryPercentage > 10)
    display.drawBitmap(13, 35, bitmap, 2, 6, 1);
  if (batteryPercentage == 0)
    display.drawBitmap(13, 36, nosig_bitmap, 5, 5, 1);
  
  if (voltage > 0) {
    display.setCursor(0, 6);
    display.print(voltage);
    display.print("mV");
  }

  // menu "button"
  drawSelectButton("Menu");

  // ntp time
  display.setCursor(45, 6);
  display.println(getFormattedTime());

  // CPU frequency
  display.setCursor(47, 16);
  display.print(getCpuFrequencyMhz());
  display.println("MHz");
}

void drawMenuScreen(int bitmapIndex, int frames, char* titleText, char* selectBtnText = "Select") {
  display.clearDisplay();
  if (bitmapIndex > 0)
    display.drawBitmap(10, 20, bitmaps[bitmapIndex + animProgress], 64, 14, 1);

  display.setCursor(70 - (strlen(titleText) * 6), 16);
  display.println(titleText);
  drawCurrentMenuPageNumber();
  drawSelectButton(selectBtnText);

  if (nonBlockingDelay(250)) {
    animProgress++;
  }

  if (animProgress > frames - 1)
    animProgress = 0;
}

void drawMenu() {
  if (currentMenuPage > 3) currentMenuPage = 1;
  if (currentMenuPage < 1) currentMenuPage = 3;

  switch(currentMenuPage) {
    case 1:
      drawMenuScreen(99, 8, "SysInfo");
      break;
    case 2:
      drawMenuScreen(124, 6, "SysTest");
      break;
    case 3:
      drawMenuScreen(41, 8, "Settings");
      break;
    default:
      break;
  }
}

void sysInfoApp() {
  display.setFont(&Picopixel);

  display.setCursor(0, 4);
  display.println("OpenNokiaProject");
  display.println(compile_date);

  if (gsm.ping()) {
    display.print("GSM info: ");
    display.println(gsm.getModuleInfo());
    if (gsm.getSleep() != 0) 
      display.println("GSM is sleeping");
    else {
      display.print("GSM signal quality: ");
      display.println(gsm.getCsq());
    }
  }
  
  display.print("WiFi sleep status: ");
  display.println(WiFi.getSleep());

  display.print("WiFi SSID: ");
  display.println(WiFi.SSID());

  display.print("WiFi IP: ");
  display.println(WiFi.localIP());

  display.setFont(&nokia4pt7b); // reset font to default
}

void sysTestApp() {
  display.setFont(&Picopixel);

  display.setCursor(0, 4);
  display.println("ONP3310 SysTest");

  if (gsm.ping()) {
    display.print("SMS: ");
    display.println(gsm.sendSMS("+48501501501", "Hello, world!") ? "Sent" : "Error"); // REMEMBER TO REMOVE YOUR PHONE NUMBER FROM HERE WHEN CONTRIBUTING 😂
  }

  display.setFont(&nokia4pt7b); // reset font to default
  drawSelectButton("Home");
}

void settingsApp() {
  display.setCursor(0, 0);
  display.println("Settings");
}

typedef void (*FunctionPointer)();
FunctionPointer AppList[] = { sysInfoApp, sysTestApp, settingsApp };

void drawApp() {
  AppList[currentApp]();
}

void handleButtonPress(uint16_t button) {
  if (currentScreen > activeAppS) // should never happen I guess
    currentScreen = homeScreen; // go home

  switch(button) {
    case 0: // Select
      switch(currentScreen) {
        case homeScreen: // if we're on the home screen
          currentScreen = menuScreen; // go to menu
          break;
        case menuScreen: // if we're in the menu
          currentScreen = activeAppS; // change screen to app
          currentApp = currentMenuPage - 1; // set currently active app ID
          return;
          break;
        case 2: // if we're in an app
          // handleInAppSelect();
          currentScreen = homeScreen; // the delete button isn't there yet...
          break;
      }
      break;
    
    case 1: // UP
      if (currentScreen == menuScreen) currentMenuPage++;
      break;

    case 2: // DOWN
      if (currentScreen == menuScreen) currentMenuPage--;
      break;

    case 3: // Delete
      if (currentScreen == menuScreen) currentScreen == homeScreen;
      if (currentScreen == activeAppS && !isTextBoxActive) currentScreen == menuScreen;
      // TODO rest
      break;
    
    default:
      break;
  }

  return;
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return "UNK_TIME";
  else {
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return String(buffer);
  }
}

void loop() {
  if (!digitalRead(SELECT_BTN_PIN)) { // LOW because of internal pullup resistor
    if (nonBlockingDelay(250))
      handleButtonPress(SELECT_BTN);
  }

  if (!digitalRead(UP_BTN_PIN)) { // same as above
    if (nonBlockingDelay(250))
      handleButtonPress(UP_BTN);
  }

  // uncomment when buttons added
  /*if (!digitalRead(DOWN_BTN_PIN)) {
    if (nonBlockingDelay(250))
      handleButtonPress(DOWN_BTN);
  }

  //if (!digitalRead(DELETE_BTN_PIN)) {
    if (nonBlockingDelay(250))
      handleButtonPress(DELETE_BTN);
  } */

  display.clearDisplay();

  switch (currentScreen) {
    case homeScreen:
      drawHomeScreen();
      break;
    case menuScreen:
      drawMenu();
      break;
    case activeAppS:
      drawApp();
      break;
    default:
      break;
  }

  display.display();
}
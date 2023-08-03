#include <SPI.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "bitmaps.h"
#include "ringtones.h"
#include "time.h"
#include "nokia4pt7b.h"
#include <Fonts/Picopixel.h>

// Serial definitions
#define SerialMon Serial
#define SerialAT Serial2

// Required definition for TinyGSM
#define TINY_GSM_MODEM_SIM800
#include <TinyGSM.h>

// Constants for WiFi and NTP
#define WIFI_SSID "TP-LINK_E924"
#define WIFI_PASS "76605567"
#define NTP_SERVER "pool.ntp.org"
#define UTC_OFFSET 7200
#define UTC_OFFSET_DST 0

// Pins for buttons and speaker
#define SELECT_BTN_PIN 22
#define UP_BTN_PIN 21
#define DOWN_BTN_PIN 25
#define DELETE_BTN_PIN 26
#define SPEAKER_PIN 19

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
TinyGsm modem(SerialAT);
int currentScreen = 0;
int currentMenuPage = 1;
int currentApp = 0;
int animProgress = 0;
bool isTextBoxActive = false;
bool isGSMAvailible = true;
bool isGSMAsleep = false;
uint8_t wifiConnectTries = 0;
const char compile_date[] = __DATE__ " " __TIME__;

void setup() {
  pinMode(SELECT_BTN_PIN, INPUT_PULLUP);
  pinMode(UP_BTN_PIN, INPUT_PULLUP);
  pinMode(DOWN_BTN_PIN, INPUT_PULLUP);
  pinMode(DELETE_BTN_PIN, INPUT_PULLUP);
  tone(SPEAKER_PIN, 400, 100);

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

  // Initialize the display
  SerialAT.begin(115200);
  display.begin();
  display.setContrast(63); // 60
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
  if (!modem.testAT(2000)) {
    isGSMAvailible = false;
    display.println("No GSM modem");
    display.display();
  } else {
    display.println("Init. modem");
    display.display();
    modem.init();
    display.println("Disable GSM");
    display.display();
    if (modem.setPhoneFunctionality(0)) {
      isGSMAsleep = true;
    }
  }

  // Set modem sleep and WiFi sleep
  // setModemSleep();
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
  display.drawBitmap(0, 42, cell_icon, 5, 6, 1);
  display.drawBitmap(6, 42, wifi_icon, 6, 6, 1);
  display.drawBitmap(13, 42, battery_icon, 4, 6, 1);
  display.drawBitmap(18, 42, bluetooth_icon, 5, 6, 1);

  // bluetooth cross icon
  display.drawBitmap(18, 36, nosig_bitmap, 5, 5, 1);

  // bluetooth sleep icon
  // display.drawBitmap(18, 22, sleep_bitmap, 5, 19, 1);

  // wifi signal bars
  if (WiFi.getSleep() != 2)
    display.drawBitmap(6, 22, sleep_bitmap, 5, 19, 1);
  else {
    int8_t wifiSignStrg = WiFi.RSSI();
    if (wifiSignStrg < -90 || wifiSignStrg > 0)
      display.drawBitmap(6, 36, nosig_bitmap, 5, 5, 1);
    else {
      if (wifiSignStrg > -80)
        display.drawBitmap(6, 35, bitmap, 2, 6, 1);
      if (wifiSignStrg > -70)
        display.drawBitmap(6, 26, bitmap, 2, 8, 1);
      if (wifiSignStrg > -55)
        display.drawBitmap(6, 17, bitmap, 3, 8, 1);
      if (wifiSignStrg > -40)
        display.drawBitmap(6, 8, bitmap, 4, 8, 1);
    }
  }

  // cell signal bars
  if (isGSMAvailible) {
    if (isGSMAsleep)
      display.drawBitmap(0, 22, sleep_bitmap, 5, 19, 1);
    else {
      int8_t cellSignStrg = modem.getSignalQuality();
      if (cellSignStrg == 0 || cellSignStrg > 40)
        display.drawBitmap(0, 36, nosig_bitmap, 5, 5, 1);
      else {
        if (cellSignStrg > 1)
          display.drawBitmap(0, 35, bitmap, 2, 6, 1);  // extreme nesting LOL
        if (cellSignStrg > 10)
          display.drawBitmap(0, 26, bitmap, 2, 8, 1);
        if (cellSignStrg > 14)
          display.drawBitmap(0, 17, bitmap, 3, 8, 1);
        if (cellSignStrg > 20)
          display.drawBitmap(0, 8, bitmap, 4, 8, 1);
      }
    }
  } else {
    display.drawBitmap(0, 36, nosig_bitmap, 5, 5, 1);
  }


  // battery power
  display.drawBitmap(13, 35, bitmap, 2, 6, 1);
  display.drawBitmap(13, 26, bitmap, 2, 8, 1);
  display.drawBitmap(13, 17, bitmap, 3, 8, 1);
  //display.drawBitmap(13, 8,  bitmap, 4, 8, 1);

  // menu "button"
  drawSelectButton("Menu");

  // ntp time
  display.setCursor(45, 6);
  display.println(getFormattedTime());

  // CPU frequency
  display.setCursor(48, 16);
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

  if (isGSMAvailible) {
    display.print("GSM info: ");
    display.println(modem.getModemInfo());
    if (isGSMAsleep) 
      display.println("GSM is sleeping");
    else {
      display.print("GSM signal quality: ");
      display.println(modem.getSignalQuality());
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
  display.setCursor(0, 0);
  display.println("SysTest");
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
  if (currentScreen > 2) // should never happen I guess
    currentScreen = 0; // go home

  switch(button) {
    case 0: // Select
      switch(currentScreen) {
        case 0: // if we're on the home screen
          currentScreen = 1; // go to menu
          break;
        case 1: // if we're in the menu
          currentScreen = 2; // change screen to app
          currentApp = currentMenuPage - 1; // set app function
          return;
          break;
        case 2: // if we're in an app
          // handleInAppSelect();
          currentScreen = 0; // the delete button isn't there yet...
          break;
      }
      break;
    
    case 1: // UP
      if (currentScreen == 1) currentMenuPage++;
      break;

    case 2: // DOWN
      if (currentScreen == 1) currentMenuPage--;
      break;

    case 3: // Delete
      if (currentScreen == 1) currentScreen == 0;
      if (currentScreen == 2 && !isTextBoxActive) currentScreen == 1;
      // TODO rest
      break;
    
    default:
      break;
  }

  return;
}

void setModemSleep() {
  WiFi.disconnect(true);
  WiFi.setSleep(true);
  if (isGSMAvailible) {
    if (modem.setPhoneFunctionality(0))
      isGSMAsleep = true;
  }
  setCpuFrequencyMhz(10);
}

void wakeModemSleep() {
  WiFi.setSleep(false);
  setCpuFrequencyMhz(80);
  if (isGSMAvailible) {
    if (modem.setPhoneFunctionality(0))
      isGSMAsleep = true;
  }
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "UNK_TIME";
  } else {
    char buffer[9];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return String(buffer);
  }
}

void loop() {
  if (!digitalRead(SELECT_BTN_PIN)) { // LOW because of internal pullup resistor
    if (nonBlockingDelay(250))
      handleButtonPress(0);
  }

  if (!digitalRead(UP_BTN_PIN)) { // same as above
    if (nonBlockingDelay(250))
      handleButtonPress(1);
  }

  // uncomment when buttons added
  /*if (!digitalRead(DOWN_BTN_PIN)) {
    if (nonBlockingDelay(250))
      handleButtonPress(2);
  }

  //if (!digitalRead(DELETE_BTN_PIN)) {
    if (nonBlockingDelay(250))
      handleButtonPress(3);
  }*/

  display.clearDisplay();

  switch (currentScreen) {
    case 0: // home screen
      drawHomeScreen();
      break;
    case 1: // menu
      drawMenu();
      break;
    case 2: // app
      drawApp();
      break;
    default:
      break;
  }

  display.display();
}
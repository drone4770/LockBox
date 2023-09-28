#include <Bounce2.h>  // Bounce library makes button change detection easy
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>
#include <Preferences.h>
#include "RTClib.h"
#include "time.h"
#include "SPIFFS.h"
//#include <SD.h>
#include <ESPmDNS.h>

#include "Chasterclient.h"
#include "ChasterData.h"

#include "lockbox.h"

Preferences preferences;

//#undef RAWHID_USAGE_PAGE
//#define RAWHID_USAGE_PAGE       0xFF06  // recommended: 0xFF00 to 0xFFFF
//#undef RAWHID_USAGE
//#define RAWHID_USAGE            0x0606  // recommended: 0x0100 to 0xFFFF

// Replace with your network credentials


#define COUNTDOWN 5
#define INCREMENT 60*1000

#define BUTTON_PIN 21
#define LID_PIN 17
#define TOUCHPIN 12

#define DISPLAY_DEBUG 0

// ESP32 settings
#define SD_CS       14
#define SRAM_CS     32
#define EPD_CS      15
#define EPD_DC      33
#define LEDPIN      13

#define EPD_RESET   -1 // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY    -1 // can set to -1 to not use a pin (will wait a fixed delay)

#define FILE_NAME "/logfile.txt"

//std::mutex file_mutex;

// What fonts do you want to use?
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
//#include <Fonts/FreeSans12pt7b.h> // a font option for larger screens

// qfont is the font for the quote
const GFXfont *tfont = &FreeSans9pt7b;
const GFXfont *qfont = &FreeSans12pt7b;

// afont is the font for the author's name
//const GFXfont *afont = &FreeSans9pt7b;
//const GFXfont *afont = &FreeSans12pt7b;

// ofont is the font for the origin of the feed
const GFXfont *ofont = NULL;

// font to use if qfont is too large
// NULL means use the system font (which is small)
const GFXfont *smallfont = NULL;

Adafruit_SSD1675 epd(250, 122, EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

unsigned long timer = 10000; // timer for screen shutdown when paused
unsigned long timerMsg = 20000; // timer for screen shutdown when no message is received

// Set web server port number to 80
AsyncWebServer server(80);
// Variable to store the HTTP request
String header;
// Assign output variables to GPIO pins
const int outputrelay = 27;

unsigned long startTimer;
unsigned long timerDuration = 0;
unsigned long remainingTime = 0;
unsigned long lastPrint = ULONG_MAX;
unsigned long buttonTimer = 0;
unsigned long lastTimerChange;

Bounce debouncer = Bounce(); // Instantiate a Bounce object
Bounce debouncerLid = Bounce(); // Instantiate a Bounce object

boolean unlockAuthorizedLed = false;
boolean ledOn = false;
boolean lockOpen = false;
boolean unlockOnce = true;
unsigned long unlockTimer = 0;
String securitypin;
unsigned long timeBlink = 1000;
unsigned long timeFastBlink = 300;
unsigned long lastBlink = millis();
unsigned long lastUpdate = millis();
boolean lidOpen = false;
int rtcFound = 0;
int needAdjust = 0;
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;
char * ntpServerValue = "pool.ntp.org";
uint8_t lasthour = 0;
uint8_t lastminute = 0;
uint8_t lastsecond = 0;
uint8_t lastminutedisplayed = 0;
String message = "";
String chasterLock = "";
ChasterAuth auth;
ChasterData data;
ChasterClient cclient = ChasterClient();
String lockCode;
String line2;
int8_t lastChasterM;
String newSharedLock = "";
String newSharedLockPwd = "";
String chasterMessage = "";

bool sdinit= false;

File myFile;

unsigned long lastTouchCheck = 0;
bool keyInBox = false;

bool wifiConnected = false;
int firstRun = 1;

typedef enum {
  FREE,
  ONCE,
  LOCKED,
  TIMER,
  TIMERONCE
} lockModeType;

lockModeType lockMode = FREE;

RTC_DS3231 rtc;

const char* PARAM_INPUT_PIN = "pin";
const char* PARAM_INPUT_TIME = "time";
const char* PARAM_INPUT_NEWPIN = "newpin";
const char* PARAM_INPUT_MESSAGE = "message";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  return String();
}

void startScreen () {
  Serial.println("startScreen()");
  epd.begin();
  Serial.println("ePaper display initialized");
  epd.setTextWrap(false);

  epd.setTextColor(EPD_BLACK);
  epd.setFont(qfont);
  epd.setTextSize(1);
  epd.clearBuffer();
  epd.setCursor(2, 20);
  epd.print("LockBox");
  epd.setCursor(2, 40);
  epd.print("v0.5");
  epd.display();
}

void writeLog(String message) {
  /*if (sdinit) {
  //std::lock_guard<std::mutex> lck(file_mutex);
  // open file for writing
  myFile = SD.open(FILE_NAME, FILE_WRITE);

  if (myFile) {
  Serial.println(F("Writing log to SD Card"));
  //myFile.seek(EOF);
  // write timestamp
  DateTime now = rtc.now();
  myFile.print(now.year(), DEC);
  myFile.print('-');
  myFile.print(now.month(), DEC);
  myFile.print('-');
  myFile.print(now.day(), DEC);
  myFile.print(' ');
  myFile.print(now.hour(), DEC);
  myFile.print(':');
  myFile.print(now.minute(), DEC);
  myFile.print(':');
  myFile.print(now.second(), DEC);

  myFile.print(" - "); // delimiter between timestamp and data

  // write data
  myFile.println(message);

  //myFile.print("\n"); // new line

  myFile.close();
  } else {
  Serial.print(F("SD Card: error on opening file "));
  Serial.println(FILE_NAME);
  }
  }*/
}

String modeString() {
  String out;
  switch(lockMode) {
    case FREE:
      out = "Free";
      break;
    case ONCE:
      out = "Once";
      break;
    case LOCKED:
      out = "Locked";
      break;
    case TIMER:
      out = "Timer";
      break;
    case TIMERONCE:
      out = "Timer Once";
      break;
    default:
      out = "Unknown";
  }
  return out;
}

String remainString() {
  lastPrint = remainingTime;
  int hour = remainingTime / (3600 * 1000);
  int minutes = (remainingTime % (3600 * 1000)) / (60 * 1000);
  int seconds = (remainingTime % (60 * 1000)) / (1000);
  String remain = "";
  remain += hour;
  remain += "h ";
  remain += minutes;
  remain += "m ";
  //remain += seconds;
  //remain += "s";
  return remain;
}

void changeModeDisplay() {
  epd.setTextWrap(false);

  epd.setTextColor(EPD_BLACK);
  epd.setFont(qfont);
  epd.setTextWrap(false);
  epd.setTextSize(2);
  epd.clearBuffer();
  epd.setCursor(1, 40);
  epd.print(modeString());
  epd.setTextSize(1);
  epd.setCursor(1, 64);
  if (lidOpen && lockMode != FREE && lockMode != ONCE) {
    epd.print("PLEASE CLOSE LID!!");
  } else if (unlockAuthorizedLed || data.canBeUnlocked) {
    epd.print("Ready to open");
  } else if (data.toVerify) {
    epd.print("Verification code:");
  } else if (lockMode == TIMER || lockMode == TIMERONCE) {
    epd.print(remainString() + " remaining");
  } else if (!data.canBeUnlocked && data.isLockActive) {
    epd.print(line2);
  }
   if (data.toVerify) {
    epd.setTextSize(2);
    epd.setCursor(1, 120);
    epd.print(data.verificationCode);
  } else if (!wifiConnected) {
    epd.setTextSize(1);
    epd.setCursor(1, 120);
    epd.print("No WiFi");
  } else if (data.isLockActive) {
    epd.setFont(tfont);
    epd.setTextWrap(true);
    epd.setTextSize(1);
    epd.setCursor(0, 90);
    epd.print(chasterMessage);
  } else if (message != "") {
    epd.setFont(tfont);
    epd.setTextWrap(true);
    epd.setTextSize(1);
    epd.setCursor(0, 90);
    epd.print(message);
  }

  epd.display();
}


void changeMode(lockModeType mode) {
  Serial.println("changeMode(" + String(mode) + ")");
  if (lockMode != mode) {
    lockMode = mode;
    if (lockMode > 1) {
      unlockAuthorizedLed = false;
    }
    preferences.putChar("lockmode", (uint8_t)mode );
    Serial.println("Status: " + modeString());
    changeModeDisplay ();
  }
}

void writeLCD( String text) {
  if (DISPLAY_DEBUG) {

  }
}

void printLines(uint8_t lstart, uint8_t * buffer) {
  for (int l = 0; l < 2; l++) {
    uint8_t line[21];
    memset(line, 0x20, 20);
    line[20] = 0x00;
    for (int i = 0; i < 20; i++) {
      uint8_t chr = buffer[2 + (20 * l) + i];
      if (chr != 0x00) {
        line[i] = chr;
      } else {
        line[i] = ' ';
      }
    }
    String Str = (char *)line;

  }
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void reconnect() {
  int tries = 0;
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    Serial.println("Attempting WiFi reconnect");
    WiFi.disconnect();
    WiFi.begin(wifissid, wifipassword);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if (++tries > 5) break;
    }
    if (WiFi.status() == WL_CONNECTED) {
      changeModeDisplay ();
      Serial.println("Wifi Connected: " + WiFi.localIP() );
      wifiConnected = true;
    } else {
      Serial.println("Wifi Not Connectd");
    }
  }
}

void startWifi () {
  Serial.println("startWifi");
  int tries = 0;
  WiFi.begin(wifissid, wifipassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (++tries > 5) break;
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(2000);
    WiFi.disconnect();
    delay(100);
    WiFi.begin(wifissid, wifipassword);
    Serial.println("Trying to connect");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if (++tries > 5) break;
    }
    if (++tries > 10) break;
  }
  wifiConnected = false;
  // Print local IP address and start web server
  if (WiFi.status() == WL_CONNECTED) {
    epd.setTextColor(EPD_BLACK);
    epd.setFont(qfont);
    epd.setTextSize(1);
    epd.clearBuffer();
    epd.setCursor(2, 20);
    epd.print("Wifi Connected");
    epd.setCursor(2, 40);
    epd.print("IP address: ");
    epd.setCursor(2, 60);
    epd.print(WiFi.localIP() );
    epd.display();
    wifiConnected = true;
    Serial.println("Wifi Connected: " + WiFi.localIP() );
  } else {
    epd.setTextColor(EPD_BLACK);
    epd.setFont(qfont);
    epd.setTextSize(1);
    epd.clearBuffer();
    epd.setCursor(2, 20);
    epd.print("Wifi Not Connectd");
    epd.display();
    Serial.println("Wifi Not Connectd");
  }


}


void unlock() {
  Serial.println("Unlock");
  writeLog("Box opened");
  unlockAuthorizedLed = false;
  digitalWrite(outputrelay, LOW);
  changeModeDisplay();
  unlockTimer = millis();
  lockOpen = true;
}

void closeLock() {
  Serial.println("CloseLock");
  digitalWrite(outputrelay, HIGH);
  lockOpen = false;
}

void startNewTimer (unsigned long time, boolean once) {
  DateTime now = getTime();
  startTimer = now.secondstime();;
  remainingTime = timerDuration = time;
  lastPrint = 0;
  closeLock();
  if (once) {
    changeMode(TIMERONCE);
  } else {
    changeMode(TIMER);
  }
  preferences.putULong("timerduration", timerDuration);
  preferences.putULong("starttimer", startTimer);
}

void endTimer () {
  if (lockMode == TIMERONCE) {
    changeMode(ONCE);
    writeLog("entered mode ONCE after end of timer");
  } else {
    changeMode(FREE);
    writeLog("entered mode FREE after end of timer");
  }
  unlockAuthorizedLed = true;
}

String getStateString() {
  String s = "";
  if (lidOpen) {
    s += "1;";
  } else {
    s += "0;";
  }
  String c = "";
  if (data.isLockActive) {
    c = "Chaster: ";
  }
  s += String(lockMode) + ";" + c + modeString() + ";";
  if (lockMode == TIMER || lockMode == TIMERONCE) {
    s += String(remainingTime);
  }
  s+= ";";
  if (keyInBox) {
    s += "1";
  } else {
    s += "0";
  }
  s+= ";";
  s+= message;
  s+= ";";
  if (data.isLockActive) {
    s+= "1";
  } else {
    s += "0";
  }
  return s;
}

void adjustNTP() {
  struct tm nowntp;
  if(!getLocalTime(&nowntp)){
    Serial.println("Failed to obtain time");
  }
  Serial.print("Time Adjust: ");
  Serial.println(&nowntp, "%A, %B %d %Y %H:%M:%S");
  Serial.print("nowntp.tm_year:");
  Serial.println(nowntp.tm_year);
  if (nowntp.tm_year >= 122) {
    rtc.adjust(DateTime(nowntp.tm_year-100, nowntp.tm_mon+1, nowntp.tm_mday, nowntp.tm_hour, nowntp.tm_min, nowntp.tm_sec));
    needAdjust = 0;
  } else {
    needAdjust = 1;
  }
}

DateTime getTime() {
  DateTime now;
  if (rtcFound) {
    now = rtc.now();
  } else if (wifiConnected) {
    struct tm nowntp;
    if(!getLocalTime(&nowntp)){
      Serial.println("Failed to obtain time");
    }
    now = DateTime(nowntp.tm_year-100, nowntp.tm_mon+1, nowntp.tm_mday, nowntp.tm_hour, nowntp.tm_min, nowntp.tm_sec);
  } else {
    now = DateTime(1907, 1, 1, 0, 0, 0);
  }
  return now;
}

void doChasterAuth() {
  String code = "";
  String grantType = "";
  if (auth.refreshToken == "") {
    Serial.println("No refresh token found. Requesting through browser");

    Serial.println( "Open browser at http:///lockbox.local\n");

      message = "connect to \nhttp://lockbox.local/auth";

    changeModeDisplay();

    while (auth.accessToken == "" && auth.refreshToken == "") {
      delay(500);
    }
    if (auth.refreshToken != "") {
      Serial.println("Saving refresh token in preferences");
      preferences.putString("cRefreshToken", auth.refreshToken);
    }
    message = "";
    changeModeDisplay();
  } else {
    Serial.println("Using refresh token found in preferences");
    code = auth.refreshToken;
    grantType = "refresh_token";
    cclient.getToken(&auth, grantType, code);
    Serial.printf("Refresh token: %s\nAccess Token: %s\n", auth.refreshToken.c_str(), auth.accessToken.c_str());

  }
}

void updateChasterState(DateTime now) {
  if (data.isLockActive) {
    chasterMessage = "Lock (" + lockCode + "):\n" + data.title;
    if (data.locked) {
      if (data.hygieneOpenAllowed == 1) {
        if (!lidOpen) {
          changeMode(ONCE);
        }
      } else {
        if (data.canBeUnlocked && data.extensionsAllowUnlocking) {
          line2 = "Ready to unlock";
          if (lastChasterM !=  97) {
            lastChasterM = 97;
            changeModeDisplay();
          }

        } else if (data.frozen && data.frozenAt < data.endDateTime && data.displayRemainingTime) {
          TimeSpan ts = data.endDateTime - data.frozenAt;
          if (lastChasterM !=  ts.minutes()) {
            lastChasterM = ts.minutes();
            String h = ts.hours() > 9 ? String(ts.hours()) : "0" + String(ts.hours()) ;
            String m = ts.minutes() > 9 ? String(ts.minutes()) : "0" + String(ts.minutes()) ;
            line2 = String(ts.days()) + "d " + h + ":" + m;
            line2 = "** " + line2 + " **";
            changeModeDisplay();
          }
        } else if (now < data.endDateTime && data.displayRemainingTime) {
          TimeSpan ts = data.endDateTime - now;
          if (lastChasterM !=  ts.minutes()) {
            lastChasterM = ts.minutes();
            String h = ts.hours() > 9 ? String(ts.hours()) : "0" + String(ts.hours()) ;
            String m = ts.minutes() > 9 ? String(ts.minutes()) : "0" + String(ts.minutes()) ;
            line2 = String(ts.days()) + "d " + h + ":" + m;
            changeModeDisplay();
          }
        } else if (!data.displayRemainingTime) {
          line2 = "??d ??:??";
          if (lastChasterM !=  96) {
            lastChasterM = 96;
            changeModeDisplay();
          }
        } else if (!data.extensionsAllowUnlocking) {
          line2 = "Missing actions";
          if (lastChasterM !=  99) {
            lastChasterM = 99;
            changeModeDisplay();
          }
        } else {
          line2 = "";
          if (lastChasterM !=  98) {
            lastChasterM = 98;
            changeModeDisplay();
          }
        }
        changeMode(LOCKED);
      }
    } else {
      line2 = "";
      changeMode(FREE);
    }
  } else {
    line2 = "";
    changeMode(FREE);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup");

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    needAdjust = 0;
  } else {
    rtcFound=1;
  }

  if (rtc.lostPower() && rtcFound) {
    Serial.println("RTC lost power, let's set the time!");
    needAdjust = 1;
  }

  DateTime now = getTime();

  /*if (!SD.begin(SD_CS)) {
    Serial.println("SD CARD FAILED, OR NOT PRESENT!");
    } else {
    sdinit = true;
    //File root = SD.open("/");
    //printDirectory(root, 0);
    }*/

  Serial.print("now.year(): ");
  Serial.println(now.year());
  if (now.year() < 2022 || now.year() > 2069) {
    needAdjust = 1;
  }

  preferences.begin("lockbox", false);
  securitypin = preferences.getString("pin", "1234");
  message = preferences.getString("message", "");
  lockMode = (lockModeType)preferences.getChar("lockmode", 0);
  timerDuration = preferences.getULong("timerduration", 0);
  startTimer = preferences.getULong("starttimer", 0);
  chasterLock = preferences.getString("chasterLock", "");
  auth.refreshToken = preferences.getString("cRefreshToken", "");
  lockCode = preferences.getString("lockCode", "");

  // put your setup code here, to run once:
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, LOW);

  debouncer.attach(BUTTON_PIN,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  debouncer.interval(25); // Use a debounce interval of 25 milliseconds

  debouncerLid.attach(LID_PIN,INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  debouncerLid.interval(25); // Use a debounce interval of 25 milliseconds
  debouncerLid.update();
  if (debouncerLid.read() == HIGH) {
    lidOpen=true;
  }
  Serial.print("Initial Status :");
  Serial.println(modeString());

  //for wifi test
  pinMode(outputrelay, OUTPUT);
  // Set outputs to LOW
  digitalWrite(outputrelay, HIGH);

  startScreen();


  startWifi();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServerValue);

  if(!MDNS.begin("lockbox")) {
    Serial.println("Error starting mDNS");
    return;
  }

  MDNS.addService("http", "tcp", 80);



  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", String(), false, processor);
    });
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", String(), false, processor);
    });
  server.on("/lockbox.js", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/lockbox.js", String(), false, processor);
    });
  server.on("/lockbox.css", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/lockbox.css", String(), false, processor);
    });
  server.on("/log", HTTP_GET, [](AsyncWebServerRequest *request){
      Serial.println("Web server got request for /log");
      /*if (sdinit) {
      //std::lock_guard<std::mutex> lck(file_mutex);
      request->send(SD, FILE_NAME, "text/plain");
      Serial.println("log sent");
      myFile = SD.open(FILE_NAME);
      if (myFile) {
      // read from the file until there's nothing else in it:
      while (myFile.available()) {
      Serial.write(myFile.read());
      }
      // close the file:
      myFile.close();
      } else {
      // if the file didn't open, print an error:
      Serial.println("error opening file");
      }
      } else {*/
      request->send(200, "text/plain", "No Log to display");
      //}
    });

  server.on("/clearlog", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /clearlog");
      /*String pin;
        if (request->hasParam(PARAM_INPUT_PIN)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
        }
        if (strcmp(securitypin.c_str(), pin.c_str()) == 0) {
        SD.remove(FILE_NAME);*/
      request->send(200, "text/plain", "Cleared");
      /*} else {
        request->send(403, "text/plain", "Forbidden");
        }*/
    });


  server.on("/isAlive", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /isAlive");
      request->send(200, "text/plain", getStateString());
    });

  server.on("/changePin", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /changePin");
      String pin;
      String newPin;
      if (request->hasParam(PARAM_INPUT_PIN)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
        newPin = request->getParam(PARAM_INPUT_NEWPIN)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        securitypin = newPin;
        preferences.putString("pin", securitypin);
        Serial.println("Pin changed to " + securitypin);
        writeLog("PIN changed");
        request->send(200, "text/plain", "Pin changed");
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/getState", HTTP_GET, [] (AsyncWebServerRequest *request) {
      //Serial.println("Web server got request for /getState");
      request->send(200, "text/plain", getStateString());
    });

  server.on("/reset", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /reset");
      String pin;
      if (request->hasParam(PARAM_INPUT_PIN)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0) {
        lockMode = FREE;
        unlockAuthorizedLed = false;
        unlock();
        writeLog("reset from interface");
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/lock", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String pin;
      if (request->hasParam(PARAM_INPUT_PIN)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        changeMode(LOCKED);
        unlockAuthorizedLed = false;
        writeLog("entered mode LOCKED from interface");
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/once", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /once");
      String pin;
      if (request->hasParam(PARAM_INPUT_PIN)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        if (lockMode != FREE && lockMode != ONCE && !lidOpen) {
          unlockAuthorizedLed = true;
        }
        changeMode(ONCE);
        writeLog("entered mode ONCE from interface");
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/free", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /free");
      String pin;
      if (request->hasParam(PARAM_INPUT_PIN)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        if (lockMode != FREE && lockMode != ONCE && !lidOpen) {
          unlockAuthorizedLed = true;
        }
        changeMode(FREE);
        writeLog("entered mode FREE from interface");
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/setTimer", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /setTimer");
      String pin;
      String timer;
      if (request->hasParam(PARAM_INPUT_PIN) && request->hasParam(PARAM_INPUT_TIME)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
        timer = request->getParam(PARAM_INPUT_TIME)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        Serial.print(", timer: ");
        Serial.print(timer);
        Serial.println("");
        startNewTimer(timer.toInt(), false);

        writeLog("Start New TIMERONCE");
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/setTimerOnce", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /setTimerOnce");
      String pin;
      String timer;
      if (request->hasParam(PARAM_INPUT_PIN) && request->hasParam(PARAM_INPUT_TIME)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
        timer = request->getParam(PARAM_INPUT_TIME)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        Serial.print(", timer: ");
        Serial.print(timer);
        Serial.println("");
        startNewTimer(timer.toInt(), true);

        writeLog("Start New TIMERONCE");
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/setMessage", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /setMessage");
      String pin;
      String msg;
      if (request->hasParam(PARAM_INPUT_PIN) && request->hasParam(PARAM_INPUT_MESSAGE)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
        msg = request->getParam(PARAM_INPUT_MESSAGE)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        Serial.print(", message: ");
        Serial.print(msg);
        message = msg;
        preferences.putString("message", message);
        writeLog("Set Message: " + message);
        changeModeDisplay();
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/clearMessage", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /setMessage");
      String pin;
      String msg;
      if (request->hasParam(PARAM_INPUT_PIN)) {
        pin = request->getParam(PARAM_INPUT_PIN)->value();
      }
      if (strcmp(securitypin.c_str(), pin.c_str()) == 0 && !data.isLockActive) {
        message = "";
        writeLog("Cleared Message");
        preferences.putString("message", message);
        changeModeDisplay();
        request->send(200, "text/plain", getStateString());
      } else {
        request->send(403, "text/plain", "Forbidden");
      }
    });

  server.on("/getMessage", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /getMessage");
      request->send(200, "text/plain", message);
    });

  server.on("/auth", HTTP_GET, [](AsyncWebServerRequest *request){
      cclient.chasterAuth(request);
    });
  server.on("/callback", HTTP_GET, [](AsyncWebServerRequest *request){
      cclient.chasterAuthCallback(&auth, request);
    });

  server.on("/chasterLock", HTTP_GET, [] (AsyncWebServerRequest *request) {
      Serial.println("Web server got request for /chasterLock");
      if (request->hasParam("sharedlock")) {
        newSharedLock = request->getParam("sharedlock")->value();
      } else {
        request->send(400, "text/plain", "Bad Request");
        return;
      }
      if (request->hasParam("password")) {
        newSharedLockPwd = request->getParam("password")->value();
      } else {
        newSharedLockPwd = "";
      }
      request->send(200, "text/plain", "Lock queued");
    });


  // Start server
  server.begin();

  Serial.println("Setup finished");
  writeLog("Box started");
  changeModeDisplay();
}

void loop() {
  DateTime now = getTime();
  unsigned long timeNow = now.secondstime();
  unsigned long millisNow = millis();

  if (now.hour() != lasthour) {
    lasthour = now.hour();
    if(rtcFound && WiFi.status() == WL_CONNECTED) {
      if (firstRun == 0) {
        adjustNTP();
      } else {
        firstRun = 0;
      }
    }
  }
  if (now.minute() != lastminute) {
    lastminute = now.minute();
    Serial.println("New minute");
    if (!wifiConnected) {
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.print("Wifi Now Connected: ");
        Serial.println( WiFi.localIP() );
        changeModeDisplay ();
        if (auth.refreshToken == "") {
          doChasterAuth();
        }
      }
    }

    if (WiFi.status() != WL_CONNECTED) {
      changeModeDisplay ();
      Serial.println("Wifi NOT Connected");
      reconnect();
    } else {
      if (needAdjust == 1  && rtcFound) {
        adjustNTP();
      }
    }
  }

  if (lockMode == TIMER || lockMode == TIMERONCE) {
    remainingTime = timerDuration + (startTimer - timeNow)*1000;
    if(remainingTime > 0 && remainingTime < (3600 * 1000 * 100)) {
      if ( remainingTime < lastPrint - 10000 ) {
        String remain = remainString();
        int minutes = (remainingTime % (3600 * 1000)) / (60 * 1000);
        if (minutes != lastminutedisplayed) {
          changeModeDisplay();
          lastminutedisplayed = minutes;
        }

        writeLCD(remain);
        /* lcd.clear(); */
        /* lcd.setCursor( 0, 1); */
        /* lcd.print( "Remaining time:"); */
        /* lcd.setCursor( 0, 2); */
        /* lcd.print( remain ); */
      }
    } else {
      endTimer();
    }
  }

  if (newSharedLock != "" && securitypin == "1234") {
    lockCode = "";
    lockCode = cclient.newlock(&data, &auth, newSharedLock, newSharedLockPwd);
    if (lockCode != "") {
      preferences.putString("lockCode", lockCode);
      Serial.println("New lock created with code " + lockCode);
      changeMode(LOCKED);
      newSharedLock = "";
      lastUpdate=0;
    }
  }

  if (lastUpdate + 5000 < millisNow) {
    if (auth.refreshToken != "" && auth.accessToken == "") {
      Serial.println("need token refresh");
      doChasterAuth();
      lastUpdate = millisNow;
    } else if (auth.accessToken != "" && securitypin == "1234") {
      Serial.println("update");
      int ret = cclient.update(&data, &auth);
      if (lidOpen || !data.isLockActive || !data.locked) {
        lastUpdate = millisNow + 25000;
      } else {
        lastUpdate = millisNow;
      }
      if (ret == 401) {
        Serial.println("Token expired");
        auth.accessToken = "";
      } else {
        Serial.println("update return code: " + String(ret));
        updateChasterState(now);
      }
    } else {
      Serial.println("waiting");
      lastUpdate = millisNow;
    }
  }

  if (lockOpen && (unlockTimer + 5000) < millisNow) {
    closeLock();
  }

  if (lockMode == FREE || lockMode == ONCE) {
    debouncer.update(); // Update the Bounce instance
    if ( debouncer.fell() ) {  // Call code if button transitions from HIGH to LOW
      unlock();
    }
  }

  debouncerLid.update(); // Update the Bounce instance
  if ( debouncerLid.fell() ) {  // Call code if button transitions from HIGH to LOW
    Serial.println("Lid Closed");
    writeLog("Lid Closed");
    digitalWrite(LEDPIN, LOW);
    lidOpen = false;
    if (data.isLockActive && data.hygieneOpenAllowed == 1) {
      lockCode = cclient.relock(&data, &auth);
      if (lockCode != "") {
        preferences.putString("lockCode", lockCode);
        Serial.println("Hygiene opening finished with code " + lockCode);
        changeMode(LOCKED);
      } else {
        Serial.println("Failed to relock");
      }
    }
    if (lockMode != FREE) {
      changeModeDisplay();
    }
  }
  if ( debouncerLid.rose() ) {  // Call code if button transitions from LOW to HIGH
    Serial.println("Lid Opened");
    writeLog("Lid Opened");
    lidOpen = true;
    if (lockMode == ONCE || lockMode == TIMERONCE) {
      changeMode(LOCKED);
      writeLog("entered mode LOCKED after box opening");
    }
    changeModeDisplay();
  }


  if (lidOpen && lockMode != FREE && lockMode != ONCE) {
    digitalWrite(LEDPIN, HIGH);
  } else if (lidOpen && lockMode == ONCE) {
    changeMode(LOCKED);
    writeLog("entered mode LOCKED after box opening");
  }

  if (unlockAuthorizedLed) {
    //Serial.print(".");
    if (timeBlink < millisNow - lastBlink) {
      //Serial.println("blink");
      lastBlink = millisNow;
      if (ledOn) {
        digitalWrite(LEDPIN, LOW);
        ledOn = false;
      } else {
        digitalWrite(LEDPIN, HIGH);
        ledOn = true;
      }
    }
  } else {
    if (ledOn) {
      digitalWrite(LEDPIN, LOW);
      ledOn = false;
    }
  }

  if (lastTouchCheck + 10000 < millisNow) {

    lastTouchCheck = millisNow;
    int touchpin_val = touchRead(TOUCHPIN);
    Serial.print("touch ");
    Serial.println(touchpin_val);
    if (touchpin_val < 20) {
      if (!keyInBox) {
        writeLog("Key in box");
        keyInBox = true;
      }
    } else {
      if (keyInBox) {
        keyInBox = false;
        writeLog("Key removed from box");
      }
    }
  }
}

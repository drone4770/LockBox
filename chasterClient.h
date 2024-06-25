#pragma once
#include <Arduino.h>
#include <SPIFFS.h>
#include "ChasterData.h"
#include <ESPAsyncWebServer.h>


class ChasterClient {
private:

  String currentKey;
  String currentParent;
  ChasterData *data;
  ChasterAuth *auth;
  bool isDataCall;
  bool isPlaylistsCall;
  bool isUserCall;
  bool inArray;
  String rootPath[10];
  uint8_t level = 0;
  uint8_t arrayLevel = 0;
  uint16_t currentImageHeight;
  DrawingCallback *drawingCallback;
  String clientId;
  String clientSecret;
  String redirectUri;
  String stateKey;
  String authCode;
  String trackID;

  void executeCallback();

  // root certificate for api.chaster.app
  const char* letsencrypt_root_ca= \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n" \
    "VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n" \
    "A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n" \
    "WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n" \
    "IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n" \
    "AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n" \
    "QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n" \
    "HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n" \
    "BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n" \
    "9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n" \
    "p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n" \
    "-----END CERTIFICATE-----\n";

public:
  ChasterClient();

  uint16_t update(ChasterData *data, ChasterAuth *auth);

  String relock(ChasterData *data, ChasterAuth *auth) ;

  boolean temporaryOpen(ChasterData *data, ChasterAuth *auth) ;

  String newlock(ChasterData *data, ChasterAuth *auth, String sharedLock, String password);

  int getToken(ChasterAuth *auth, String grantType, String code);

  void setDrawingCallback(DrawingCallback *drawingCallback) {
    this->drawingCallback = drawingCallback;
  }

  String waitForAuthCode();

  String randomString(int ch, bool numcode);

  void chasterAuth(AsyncWebServerRequest *request);

  void chasterAuthCallback(ChasterAuth *auth, AsyncWebServerRequest *request);



};

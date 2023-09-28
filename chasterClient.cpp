//#include <WiFi.h>
#include <HTTPClient.h>
#include "ChasterClient.h"
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "RTClib.h"

#include "lockbox.h"

#define min(X, Y) (((X)<(Y))?(X):(Y))

#define MBEDTLS_SSL_SERVER_NAME_INDICATION
#define MBEDTLS_SSL_VERIFY_REQUIRED

const char* fmtMemCk = "Free: %d\tMaxAlloc: %d\t PSFree: %d\n";
#define MEMCK Serial.printf(fmtMemCk,ESP.getFreeHeap(),ESP.getMaxAllocHeap(),ESP.getFreePsram())

ChasterClient::ChasterClient() {
  this->authCode = "";
  this->redirectUri = "http%3A%2F%2Flockbox.local%2Fcallback";
}

uint16_t ChasterClient::update(ChasterData *data, ChasterAuth *auth) {
  level = 0;
  isDataCall = true;
  isPlaylistsCall = false;
  isUserCall = false;
  currentParent = "";
  WiFiClientSecure client;
  HTTPClient http;
  DynamicJsonDocument doc(20480);
  String url = "https://api.chaster.app/locks";

  client.setCACert(letsencrypt_root_ca);
  http.useHTTP10(true);
  http.begin(client, url);
  http.addHeader("Authorization", "Bearer " + auth->accessToken);
  int httpCode = http.GET();

  String ret = http.getString();
  Serial.println(ret);

  DeserializationError err = deserializeJson(doc, ret);
  if (err) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(err.c_str());
    return 500;
  }
  if (doc.overflowed()) {
    Serial.println("Overflowed");
  }
  Serial.println("Memory Usage: " + String(doc.memoryUsage()) + " - " + ret.length());

  JsonArray array = doc.as<JsonArray>();
  //Serial.println("Array size: " + String(array.size()));
  if (array.size() > 0){
    data->isLockActive = true;
    JsonObject obj = array[0].as<JsonObject>();
    if (obj.containsKey("status")) {
      const char * str = obj["status"];
      data->lockedString = String(str);
      Serial.println("Status: " + data->lockedString);
      if (data->lockedString == "locked") {
        data->locked = true;
      } else {
        data->locked = false;
      }
    }
    if (obj.containsKey("title")) {
      const char * str = obj["title"];
      data->title = String(str);
      //Serial.println("Title: " + data->title);
    }
    if (obj.containsKey("_id")) {
      const char * str = obj["_id"];
      data->lockId = String(str);
      //Serial.println("LockId: " + data->title);
    }

    if (obj.containsKey("displayRemainingTime")) {
      data->displayRemainingTime = obj["displayRemainingTime"];
    }
    if (obj.containsKey("isFrozen")) {
      data->frozen = obj["isFrozen"];
      if (data->frozen && data->displayRemainingTime && obj.containsKey("frozenAt")) {
        const char * str = obj["frozenAt"];
        int yr, mnth, d, h, m, s, ms;
        sscanf( str, "%4d-%2d-%2dT%2d:%2d:%2d.%3dZ", &yr, &mnth, &d, &h, &m, &s, &ms);
        data->frozenAt = DateTime(yr, mnth, d, h, m, s);
      }
    }
    if (obj.containsKey("endDate")) {
      if (data->displayRemainingTime) {
        const char * str = obj["endDate"];
        data->endDate = String(str);
        //Serial.println("endDate: " + data->endDate);
        int yr, mnth, d, h, m, s, ms;
        sscanf( str, "%4d-%2d-%2dT%2d:%2d:%2d.%3dZ", &yr, &mnth, &d, &h, &m, &s, &ms);
        data->endDateTime = DateTime(yr, mnth, d, h, m, s);
      }
    }
    if (obj.containsKey("canBeUnlocked")) {
      data->canBeUnlocked = obj["canBeUnlocked"];
    }
    if (obj.containsKey("extensionsAllowUnlocking")) {
      data->extensionsAllowUnlocking = obj["extensionsAllowUnlocking"];
    }
    if (obj.containsKey("totalDuration")) {
      data->totalDuration = obj["totalDuration"];
      //Serial.println("totalDuration: " + String(data->totalDuration));
    }
    if (obj.containsKey("sharedLock")) {
      JsonObject sl = obj["sharedLock"].as<JsonObject>();
      if (sl.containsKey("_id")) {
        const char * str = sl["_id"];
        data->sharedLockId = String(str);
        //Serial.println("sharedLockId: " + data->sharedLockId);
      }
      if (sl.containsKey("name")) {
        const char * str = sl["name"];
        data->sharedLockName = String(str);
        //Serial.println("sharedLockName: " + data->sharedLockName);
      }
    }
    if (obj.containsKey("keyholder")) {
      JsonObject kh = obj["keyholder"].as<JsonObject>();
      if (kh.containsKey("username")) {
        const char * str = kh["username"];
        data->keyholder = String(str);
        //Serial.println("keyholder: " + data->keyholder);
      }
    }
    data->hygieneOpenAllowed = 0;
    data->toVerify = false;
    if (obj.containsKey("extensions")) {
      JsonArray ea = obj["extensions"].as<JsonArray>();
      for(JsonVariant v : ea) {
        JsonObject ext = v.as<JsonObject>();
        if (ext.containsKey("slug")) {
          const char * str = ext["slug"];
          //Serial.println("extension : " + String(str));
          if(String(str) == "temporary-opening") {
            JsonObject ud = ext["userData"].as<JsonObject>();
            if(ud.containsKey("openedAt")) {
              if (!ud["openedAt"].isNull()) {
                const char * str = ud["openedAt"];
                //Serial.print("openedAt: ");
                //Serial.println(String(str));
                data->hygieneOpenAllowed = 1;
              }
            }
          } else if(String(str) == "verification-picture") {
            JsonObject ud = ext["userData"].as<JsonObject>();
            if(ud.containsKey("currentVerificationCode")) {
              if (!ud["currentVerificationCode"].isNull()) {
                const char * str = ud["currentVerificationCode"];
                data->verificationCode = String(str);
                if (data->verificationCode != "") {
                  data->toVerify = true;
                }
              }
            }
          }
        }
      }
    }
    data->unlockReason = "";
    if (obj.containsKey("reasonsPreventingUnlocking")) {
      JsonArray ra = obj["reasonsPreventingUnlocking"].as<JsonArray>();
      for(JsonVariant v : ra) {
        JsonObject reason = v.as<JsonObject>();
        if (reason.containsKey("reason")) {
          const char * str = reason["reason"];
          if (String(str) == "link_not_enough_votes") {
              data->unlockReason += "votes ";
          } else if (String(str) == "tasks") {
              data->unlockReason += "tasks ";
          } else if (String(str) == "temporary-opening") {
              data->unlockReason += "hygiene ";
          }
        }
      }
    }
  } else {
    Serial.println("No Lock Set");
    data->isLockActive = false;
  }
  doc.garbageCollect();

  Serial.println("HTTP Code: " + String(httpCode));

  return httpCode;
}

String ChasterClient::relock(ChasterData *data, ChasterAuth *auth) {
  Serial.println("Relock()");
  DynamicJsonDocument doc(2048);
  WiFiClientSecure client;
  HTTPClient http;
  client.setCACert(letsencrypt_root_ca);

  String url = "https://api.chaster.app/combinations/code";

  String lbn = lockBoxChasterName;
  String lockCode = this->randomString(4, true);
  String content = "{\"code\": \"" + lbn + "-" + lockCode + "\"}";

  http.useHTTP10(true);
  http.begin(client, url);
  http.addHeader("Authorization", "Bearer " + auth->accessToken);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(content);

  if (httpCode == 201) {
    deserializeJson(doc, http.getStream());
    if (doc.containsKey("combinationId")) {
      const char * str = doc["combinationId"];
      String combinationId = String(str);
      http.end();
      url = "https://api.chaster.app/extensions/temporary-opening/" + data->lockId + "/combination";
      content = "{\"combinationId\": \"" + combinationId + "\"}";

      http.useHTTP10(true);
      http.begin(client, url);
      http.addHeader("Authorization", "Bearer " + auth->accessToken);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(content);

       if (httpCode == 201) {
         return lockCode;
       } else {
         Serial.println("Failed to lock with received combination");
         Serial.print(http.getString());
         return "";
       }
    } else {
      Serial.println("No combinationId received");
      Serial.print(http.getString());
      return "";
    }
  } else {
    Serial.println("Could not set new combination");
    Serial.print(http.getString());
    return "";
  }
}

String ChasterClient::newlock(ChasterData *data, ChasterAuth *auth, String sharedLock, String password) {
  Serial.println("Relock()");
  DynamicJsonDocument doc(2048);
  WiFiClientSecure client;
  HTTPClient http;
  client.setCACert(letsencrypt_root_ca);

  String url = "https://api.chaster.app/combinations/code";

  String lbn = lockBoxChasterName;
  String lockCode = this->randomString(4, true);
  String content = "{\"code\": \"" + lbn + "-" + lockCode + "\"}";

  http.useHTTP10(true);
  http.begin(client, url);
  http.addHeader("Authorization", "Bearer " + auth->accessToken);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(content);

  if (httpCode == 201) {
    deserializeJson(doc, http.getStream());
    if (doc.containsKey("combinationId")) {
      const char * str = doc["combinationId"];
      String combinationId = String(str);
      http.end();
      url = "https://api.chaster.app/public-locks/" + sharedLock + "/create-lock";
      if (password != "") {
        content = "{\"combinationId\": \"" + combinationId + "\", \"isTestLock\": false}";
      } else {
        content = "{\"combinationId\": \"" + combinationId + "\", \"password\": \"" + password + "\", \"isTestLock\": false}";
      }
      Serial.println(content);

      http.useHTTP10(true);
      http.begin(client, url);
      http.setTimeout(60000);
      http.addHeader("Authorization", "Bearer " + auth->accessToken);
      http.addHeader("Content-Type", "application/json");
      httpCode = http.POST(content);

       if (httpCode == 201) {
         return lockCode;
       } else {
         Serial.println("Failed to lock with received combination");
         Serial.print(http.getString());
         return "";
       }
    } else {
      Serial.println("No combinationId received");
      Serial.print(http.getString());
      return "";
    }
  } else {
    Serial.println("Could not set new combination");
    Serial.print(http.getString());
    return "";
  }
}


void ChasterClient::getToken(ChasterAuth *auth, String grantType, String code) {
  isDataCall = false;
  DynamicJsonDocument doc(2048);
  WiFiClientSecure client;
  HTTPClient http;
  client.setCACert(letsencrypt_root_ca);
  //https://accounts.chaster.com/api/token
  String url = "https://sso.chaster.app/auth/realms/app/protocol/openid-connect/token";

  //Serial.print("Requesting URL: ");
  //Serial.println(url);
  String codeParam = "code";
  if (grantType == "refresh_token") {
    codeParam = "refresh_token";
  }
  // This will send the request to the server
  String content = "grant_type=" + grantType + "&" + codeParam + "=" + code + "&client_id=" + clientId + "&client_secret=" + clientSecret + "&redirect_uri=" + redirectUri;
  Serial.println(content);
  http.useHTTP10(true);
  http.begin(client, url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(content);

  //Serial.print(http.getString());

  deserializeJson(doc, http.getStream());
  if (doc.containsKey("access_token")) {
    const char * token = doc["access_token"];
    auth->accessToken = String(token);
    Serial.println("access_token: " + auth->accessToken);
  }
  // "token_type":"Bearer", String tokenType;
  if (doc.containsKey("token_type")) {
    const char * str = doc["token_type"];
    auth->tokenType = String(str);
  }
  // "expires_in":3600, uint16_t expiresIn;
  if (doc.containsKey("expires_in")) {
    auth->expiresIn = doc["expires_in"];
  }
  // "refresh_token":"XX", String refreshToken;
  if (doc.containsKey("refresh_token")) {
    const char * str = doc["refresh_token"];
    auth->refreshToken = String(str);
  }
  // "scope":"user-modify-playback-state user-read-playback-state user-read-currently-playing user-read-private String scope;
  if (doc.containsKey("scope")) {
    const char * str = doc["scope"];
    auth->scope = String(str);
  }
  Serial.println("HTTP Code: " + String(httpCode));

}

void ChasterClient::executeCallback() {
  if (drawingCallback != nullptr) {
    (*this->drawingCallback)();
  }
}

String ChasterClient::randomString(int ch, bool numcode) {
  String result = "";
  if (numcode) {
    const int ch_MAX = 10;
    char num[ch_MAX] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    for (int i = 0; i<ch; i++) {
      result = result + num[(millis() + std::rand()) % ch_MAX];
    }
  } else {
    const int ch_MAX = 36;
    char alpha[ch_MAX] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g',
      'h', 'i', 'j', 'k', 'l', 'm', 'n',
      'o', 'p', 'q', 'r', 's', 't', 'u',
      'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    for (int i = 0; i<ch; i++) {
      result = result + alpha[(millis() + std::rand()) % ch_MAX];
    }
  }

  return result;
}

String ChasterClient::waitForAuthCode() {
  while (this->authCode == "") {
    yield();
  }
  return this->authCode;
}

void ChasterClient::chasterAuth(AsyncWebServerRequest *request) {
  this->stateKey = this->randomString(20, false);
  request->redirect( String("https://sso.chaster.app/auth/realms/app/protocol/openid-connect/auth?response_type=code&client_id="
                            + String(clientId)
                            + "&redirect_uri="
                            + this->redirectUri
                            + "&scope=profile%20locks%20shared_locks%20messaging&state="
                            + this->stateKey));
}

void ChasterClient::chasterAuthCallback(ChasterAuth *auth, AsyncWebServerRequest *request ) {
  String returnedKey;
  if (request->hasParam("state") && request->hasParam("code")) {
    returnedKey = request->getParam("state")->value();
    if (returnedKey == this->stateKey) {
      this->authCode = request->getParam("code")->value();
      Serial.println("Received auth code " + this->authCode);
      this->getToken(auth, "authorization_code", this->authCode);
      request->redirect( "http://lockbox.local/" );
    } else {
      request->send(403, "text/plain", "Forbidden");
    }
  } else {
    request->send(400, "text/plain", "Bad Request");
  }
}

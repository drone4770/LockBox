#pragma once

#include "RTClib.h"

typedef void (*DrawingCallback)();

typedef struct ChasterAuth {
  // "access_token": "XXX"
  String accessToken;
  // "token_type":"Bearer",
  String tokenType;
  // "expires_in":3600,
  uint16_t expiresIn;
  // "refresh_token":"XX",
  String refreshToken;
  // "scope":"user-modify-playback-state user-read-playback-state user-read-currently-playing user-read-private
  String scope;

} ChasterAuth;

typedef struct ChasterData {
  boolean isLockActive = false;
  boolean locked = false;
  String lockedString;
  String unlockReason;
  String title;
  String lockId;
  String endDate;
  DateTime endDateTime;
  DateTime frozenAt;
  uint32_t totalDuration;
  String keyholder;
  String sharedLockId;
  String sharedLockName;
  bool displayRemainingTime;
  uint16_t hygieneOpenAllowed;
  bool extensionsAllowUnlocking = false;
  bool frozen = false;
  bool canBeUnlocked = false;
  String verificationCode;
  bool toVerify = false;
} ChasterData;

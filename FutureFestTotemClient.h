#pragma once

#include <Arduino.h>

struct TotemStatus {
  bool ok = false;
  bool connected = false;
  bool gameOver = false;
  int lives = -1;
  int maxLives = 3;
  int currentRound = 0;
  int httpCode = 0;
  String sessionStatus = "";
  String error = "";
  String raw = "";
};

class FutureFestTotemClient {
public:
  explicit FutureFestTotemClient(const String& baseUrl);

  bool setManagerId(const String& value);
  bool setSessionId(const String& value);
  void clearSession();
  void setDeviceId(const String& value);

  String managerId() const;
  String managerLabel() const;
  String sessionId() const;
  String deviceId() const;
  bool hasManager() const;
  bool hasSession() const;
  bool isConnected() const;

  TotemStatus connect();
  TotemStatus disconnect();
  TotemStatus status();

private:
  String _baseUrl;
  String _managerId;
  String _sessionId;
  String _deviceId;
  bool _connected = false;

  String normalizeManagerId(const String& value) const;
  String normalizeSessionId(const String& value) const;
  String endpoint(const String& action) const;
  TotemStatus request(const String& method, const String& action, const String& payload);
  void parseStatusPayload(TotemStatus& status);
};

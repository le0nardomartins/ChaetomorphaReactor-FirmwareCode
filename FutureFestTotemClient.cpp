#include "FutureFestTotemClient.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

FutureFestTotemClient::FutureFestTotemClient(const String& baseUrl) {
  _baseUrl = baseUrl;
  _baseUrl.trim();
  while (_baseUrl.endsWith("/")) {
    _baseUrl.remove(_baseUrl.length() - 1);
  }
}

bool FutureFestTotemClient::setManagerId(const String& value) {
  String normalized = normalizeManagerId(value);
  if (normalized.length() == 0) return false;
  _managerId = normalized;
  return true;
}

bool FutureFestTotemClient::setSessionId(const String& value) {
  String normalized = normalizeSessionId(value);
  if (normalized.length() == 0) return false;
  _sessionId = normalized;
  return true;
}

void FutureFestTotemClient::clearSession() {
  _sessionId = "";
  _connected = false;
}

void FutureFestTotemClient::setDeviceId(const String& value) {
  _deviceId = value;
  _deviceId.trim();
}

String FutureFestTotemClient::managerId() const {
  return _managerId;
}

String FutureFestTotemClient::managerLabel() const {
  if (_managerId == "01") return "Pessoas";
  if (_managerId == "02") return "Climaticos";
  if (_managerId == "03") return "Plantas";
  if (_managerId == "04") return "Animais";
  return "Nao definido";
}

String FutureFestTotemClient::sessionId() const {
  return _sessionId;
}

String FutureFestTotemClient::deviceId() const {
  return _deviceId;
}

bool FutureFestTotemClient::hasManager() const {
  return _managerId.length() == 2;
}

bool FutureFestTotemClient::hasSession() const {
  return _sessionId.length() == 4;
}

bool FutureFestTotemClient::isConnected() const {
  return _connected;
}

TotemStatus FutureFestTotemClient::connect() {
  String payload = "{}";
  if (_deviceId.length() > 0) {
    payload = "{\"device_id\":\"" + _deviceId + "\"}";
  }
  TotemStatus result = request("POST", "connect", payload);
  _connected = result.ok && result.connected;
  return result;
}

TotemStatus FutureFestTotemClient::disconnect() {
  TotemStatus result = request("POST", "disconnect", "{}");
  _connected = false;
  return result;
}

TotemStatus FutureFestTotemClient::status() {
  TotemStatus result = request("GET", "status", "");
  if (result.ok) {
    _connected = result.connected;
  } else if (result.httpCode == 404) {
    _connected = false;
  }
  return result;
}

String FutureFestTotemClient::normalizeManagerId(const String& value) const {
  String text = value;
  text.trim();
  text.toLowerCase();

  if (text == "1" || text == "01" || text == "pessoa" || text == "pessoas" || text == "social") return "01";
  if (text == "2" || text == "02" || text == "clima" || text == "climatico" || text == "climaticos" || text == "climatica") return "02";
  if (text == "3" || text == "03" || text == "planta" || text == "plantas" || text == "flora") return "03";
  if (text == "4" || text == "04" || text == "animal" || text == "animais" || text == "fauna") return "04";

  return "";
}

String FutureFestTotemClient::normalizeSessionId(const String& value) const {
  String text = value;
  text.trim();
  text.toUpperCase();
  if (text.length() != 4) return "";

  for (int index = 0; index < text.length(); index += 1) {
    char current = text.charAt(index);
    bool valid = (current >= 'A' && current <= 'Z') || (current >= '0' && current <= '9');
    if (!valid) return "";
  }

  return text;
}

String FutureFestTotemClient::endpoint(const String& action) const {
  return _baseUrl + "/totems/" + _managerId + "/" + _sessionId + "/" + action;
}

TotemStatus FutureFestTotemClient::request(const String& method, const String& action, const String& payload) {
  TotemStatus result;

  if (WiFi.status() != WL_CONNECTED) {
    result.error = "WIFI_DISCONNECTED";
    return result;
  }

  if (!hasManager()) {
    result.error = "MANAGER_ID_NOT_SET";
    return result;
  }

  if (!hasSession()) {
    result.error = "SESSION_ID_NOT_SET";
    return result;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = endpoint(action);

  if (!http.begin(client, url)) {
    result.error = "HTTP_BEGIN_FAILED";
    return result;
  }

  http.setTimeout(8000);
  http.addHeader("Content-Type", "application/json");

  int code = 0;
  if (method == "GET") {
    code = http.GET();
  } else {
    code = http.POST(payload);
  }

  result.httpCode = code;
  result.raw = http.getString();
  http.end();

  if (code < 200 || code >= 300) {
    result.error = "HTTP_" + String(code);
    return result;
  }

  parseStatusPayload(result);
  result.ok = true;
  return result;
}

void FutureFestTotemClient::parseStatusPayload(TotemStatus& status) {
  String raw = status.raw;
  raw.replace("\n", "");
  raw.replace("\r", "");
  raw.replace(" ", "");

  int connectedIndex = raw.indexOf("\"connected\":true");
  status.connected = connectedIndex >= 0;

  int gameOverIndex = raw.indexOf("\"game_over\":true");
  status.gameOver = gameOverIndex >= 0;

  int livesIndex = raw.indexOf("\"lives\":");
  if (livesIndex >= 0) {
    int valueStart = livesIndex + 8;
    if (!raw.substring(valueStart).startsWith("null")) {
      status.lives = raw.substring(valueStart).toInt();
    }
  }

  int maxLivesIndex = raw.indexOf("\"max_lives\":");
  if (maxLivesIndex >= 0) {
    int valueStart = maxLivesIndex + 12;
    status.maxLives = raw.substring(valueStart).toInt();
  }

  int roundIndex = raw.indexOf("\"current_round\":");
  if (roundIndex >= 0) {
    int valueStart = roundIndex + 16;
    status.currentRound = raw.substring(valueStart).toInt();
  }

  int sessionStatusIndex = raw.indexOf("\"session_status\":\"");
  if (sessionStatusIndex >= 0) {
    int valueStart = sessionStatusIndex + 18;
    int valueEnd = raw.indexOf("\"", valueStart);
    if (valueEnd > valueStart) {
      status.sessionStatus = raw.substring(valueStart, valueEnd);
    }
  }
}

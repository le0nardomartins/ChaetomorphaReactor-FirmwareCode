#include <WiFi.h>

// =========================
// PINOS
// =========================
const int PINO_VERDE    = 4;
const int PINO_VERMELHO = 21;
const int PINO_AZUL     = 19;
const int PINO_BOMBA    = 25;

// =========================
// WIFI e DATABASE
// =========================
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_SENHA = "SUA_SENHA";
const unsigned long WIFI_TIMEOUT_MS = 15000;
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000;
const int ColumnID = 1; // Mude isso sempre ao conectar em diferentes partidas (disponíveis 1, 2, 3 e 4)

// =========================
// CONFIG PWM
// =========================
const int FREQUENCIA_PWM = 5000;
const int RESOLUCAO_PWM  = 8; // 0 a 255
const int BRILHO_MAXIMO  = 255;
const unsigned long INTERVALO_PISCA_MS = 500;
const int VEZES_PISCA_VERMELHO = 5;

// =========================
// CONFIGURAÇÃO DA FITA RGB
// =========================
const bool FITA_COMUM_ANODO = false;

// =========================
// FUNÇÕES AUXILIARES
// =========================
void escreverPWM(int pino, int valor) {
  valor = constrain(valor, 0, 255);

  if (FITA_COMUM_ANODO) {
    valor = 255 - valor;
  }

  ledcWrite(pino, valor);
}

void setCorRGB(int vermelho, int verde, int azul) {
  escreverPWM(PINO_VERMELHO, vermelho);
  escreverPWM(PINO_VERDE, verde);
  escreverPWM(PINO_AZUL, azul);
}

// =========================
// CORES
// =========================
void corVerde() {
  setCorRGB(0, BRILHO_MAXIMO, 0);
}

void corAmarela() {
  setCorRGB(BRILHO_MAXIMO, BRILHO_MAXIMO, 0);
}

void corVermelha() {
  setCorRGB(BRILHO_MAXIMO, 0, 0);
}

void piscarVermelho() {
  for (int i = 0; i < VEZES_PISCA_VERMELHO; i++) {
    corVermelha();
    delay(INTERVALO_PISCA_MS);
    setCorRGB(0, 0, 0);
    delay(INTERVALO_PISCA_MS);
  }

  corVermelha();
}

// =========================
// WIFI
// =========================
bool conectarWiFi(const char* ssid, const char* senha, unsigned long timeoutMs) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, senha);

  Serial.print("Conectando ao WiFi");
  unsigned long inicio = millis();

  while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < timeoutMs) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado com sucesso.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println();
  Serial.println("Falha ao conectar no WiFi.");
  return false;
}

void manterWiFiConectado() {
  static unsigned long ultimaTentativa = 0;

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long agora = millis();
  if (agora - ultimaTentativa >= WIFI_RECONNECT_INTERVAL_MS) {
    ultimaTentativa = agora;
    conectarWiFi(WIFI_SSID, WIFI_SENHA, WIFI_TIMEOUT_MS);
  }
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  pinMode(PINO_BOMBA, OUTPUT);
  digitalWrite(PINO_BOMBA, HIGH);

  bool okR = ledcAttach(PINO_VERMELHO, FREQUENCIA_PWM, RESOLUCAO_PWM);
  bool okG = ledcAttach(PINO_VERDE,    FREQUENCIA_PWM, RESOLUCAO_PWM);
  bool okB = ledcAttach(PINO_AZUL,     FREQUENCIA_PWM, RESOLUCAO_PWM);

  if (!okR || !okG || !okB) {
    while (true) {
      delay(1000);
    }
  }

  corVerde();

  conectarWiFi(WIFI_SSID, WIFI_SENHA, WIFI_TIMEOUT_MS);
}

// =========================
// LOOP
// =========================
void loop() {
  manterWiFiConectado();

  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    comando.toLowerCase();

    if (comando == "verde") {
      corVerde();
    }
    else if (comando == "amarelo") {
      corAmarela();
    }
    else if (comando == "vermelho") {
      corVermelha();
    }
    else if (comando == "piscar") {
      piscarVermelho();
    }
  }
}

#include <WiFi.h>

// =========================
// PINOS
// =========================
const int PINO_VERDE    = 4;
const int PINO_VERMELHO = 21;
const int PINO_AZUL     = 19;
const int PINO_BOMBA    = 25;

// =========================
// WIFI
// =========================
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_SENHA = "SUA_SENHA";
const unsigned long WIFI_TIMEOUT_MS = 15000;

// =========================
// CONFIG PWM
// =========================
const int FREQUENCIA_PWM = 5000;
const int RESOLUCAO_PWM  = 8; // 0 a 255

// =========================
// CONFIGURAÇÃO DO RELÉ
// =========================
const bool RELE_ATIVO_EM_LOW = true;

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
  setCorRGB(0, 255, 0);
}

void corAmarela() {
  setCorRGB(255, 255, 0);
}

void corVermelha() {
  setCorRGB(255, 0, 0);
}

void apagarFita() {
  setCorRGB(0, 0, 0);
}

// =========================
// BOMBA
// =========================
void ligarBomba() {
  digitalWrite(PINO_BOMBA, RELE_ATIVO_EM_LOW ? LOW : HIGH);
}

void desligarBomba() {
  digitalWrite(PINO_BOMBA, RELE_ATIVO_EM_LOW ? HIGH : LOW);
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

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);

  pinMode(PINO_BOMBA, OUTPUT);
  desligarBomba();

  bool okR = ledcAttach(PINO_VERMELHO, FREQUENCIA_PWM, RESOLUCAO_PWM);
  bool okG = ledcAttach(PINO_VERDE,    FREQUENCIA_PWM, RESOLUCAO_PWM);
  bool okB = ledcAttach(PINO_AZUL,     FREQUENCIA_PWM, RESOLUCAO_PWM);

  if (!okR || !okG || !okB) {
    Serial.println("Erro ao configurar PWM LEDC.");
    while (true) {
      delay(1000);
    }
  }

  conectarWiFi(WIFI_SSID, WIFI_SENHA, WIFI_TIMEOUT_MS);

  corVerde();

  Serial.println("Sistema iniciado.");
  Serial.println("Comandos:");
  Serial.println("verde");
  Serial.println("amarelo");
  Serial.println("vermelho");
  Serial.println("apagar");
  Serial.println("on  -> liga bomba");
  Serial.println("off -> desliga bomba");
}

// =========================
// LOOP
// =========================
void loop() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    comando.toLowerCase();

    if (comando == "verde") {
      corVerde();
      Serial.println("Cor: VERDE");
    }
    else if (comando == "amarelo") {
      corAmarela();
      Serial.println("Cor: AMARELO");
    }
    else if (comando == "vermelho") {
      corVermelha();
      Serial.println("Cor: VERMELHO");
    }
    else if (comando == "apagar") {
      apagarFita();
      Serial.println("Fita apagada");
    }
    else if (comando == "on") {
      ligarBomba();
      Serial.println("Bomba LIGADA");
    }
    else if (comando == "off") {
      desligarBomba();
      Serial.println("Bomba DESLIGADA");
    }
    else {
      Serial.println("Comando invalido");
    }
  }
}

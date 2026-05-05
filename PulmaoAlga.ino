#include <WiFi.h>

#include "FutureFestTotemClient.h"

// =========================
// PINOS
// =========================
const int PINO_VERDE = 4;
const int PINO_VERMELHO = 21;
const int PINO_AZUL = 19;
const int PINO_BOMBA = 25;

// =========================
// WIFI
// =========================
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_SENHA = "SUA_SENHA";
const unsigned long WIFI_TIMEOUT_MS = 15000;

// =========================
// FUTUREFEST SERVER
// =========================
const String FUTUREFEST_API_BASE_URL = "https://ffcvg-production-f333.up.railway.app/api";
const String DEVICE_ID = "pulmao-alga-01";
const unsigned long STATUS_INTERVAL_MS = 5000;
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;

FutureFestTotemClient futureFest(FUTUREFEST_API_BASE_URL);
unsigned long ultimoStatusMs = 0;
unsigned long ultimoHeartbeatMs = 0;
int ultimaVidaExibida = -99;

// =========================
// CONFIG PWM
// =========================
const int FREQUENCIA_PWM = 5000;
const int RESOLUCAO_PWM = 8; // 0 a 255

// =========================
// CONFIGURACAO DO RELE
// =========================
const bool RELE_ATIVO_EM_LOW = true;

// =========================
// CONFIGURACAO DA FITA RGB
// =========================
const bool FITA_COMUM_ANODO = false;

// =========================
// FUNCOES AUXILIARES
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

void corAzul() {
  setCorRGB(0, 70, 255);
}

void corRoxa() {
  setCorRGB(160, 0, 255);
}

void apagarFita() {
  setCorRGB(0, 0, 0);
}

void piscarCor(void (*cor)(), int vezes, int intervaloMs) {
  for (int index = 0; index < vezes; index += 1) {
    cor();
    delay(intervaloMs);
    apagarFita();
    delay(intervaloMs);
  }
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
// FUTUREFEST
// =========================
void imprimirStatusTotem(const TotemStatus& status) {
  if (!status.ok) {
    Serial.print("Falha na API: ");
    Serial.print(status.error);
    Serial.print(" HTTP=");
    Serial.println(status.httpCode);
    if (status.raw.length() > 0) {
      Serial.print("Resposta: ");
      Serial.println(status.raw);
    }
    return;
  }

  Serial.print("Sessao ");
  Serial.print(futureFest.sessionId());
  Serial.print(" | gestor ");
  Serial.print(futureFest.managerId());
  Serial.print(" ");
  Serial.print(futureFest.managerLabel());
  Serial.print(" | conectado=");
  Serial.print(status.connected ? "sim" : "nao");
  Serial.print(" | status=");
  Serial.print(status.sessionStatus);
  Serial.print(" | rodada=");
  Serial.print(status.currentRound);
  Serial.print(" | vidas=");
  Serial.print(status.lives);
  Serial.print("/");
  Serial.println(status.maxLives);
}

void aplicarVidaNoHardware(const TotemStatus& status) {
  if (!status.ok || status.lives < 0) {
    corAzul();
    desligarBomba();
    return;
  }

  if (status.gameOver || status.lives <= 0) {
    ligarBomba();
    if (ultimaVidaExibida != status.lives) {
      piscarCor(corVermelha, 4, 120);
    }
    corVermelha();
  } else if (status.lives == 1) {
    ligarBomba();
    corVermelha();
  } else if (status.lives == 2) {
    desligarBomba();
    corAmarela();
  } else {
    desligarBomba();
    corVerde();
  }

  if (ultimaVidaExibida >= 0 && status.lives < ultimaVidaExibida) {
    piscarCor(corVermelha, 3, 100);
  }

  ultimaVidaExibida = status.lives;
}

bool garantirConfiguracaoTotem() {
  if (!futureFest.hasManager()) {
    Serial.println("Defina o gestor primeiro. Exemplo: id flora");
    return false;
  }

  if (!futureFest.hasSession()) {
    Serial.println("Defina ou informe a sessao. Exemplo: connect A7K2");
    return false;
  }

  return true;
}

void conectarTotem() {
  if (!garantirConfiguracaoTotem()) return;

  Serial.println("Conectando totem ao servidor...");
  TotemStatus status = futureFest.connect();
  imprimirStatusTotem(status);
  aplicarVidaNoHardware(status);

  if (status.ok) {
    ultimoHeartbeatMs = millis();
    ultimoStatusMs = millis();
  }
}

void desconectarTotem() {
  if (!futureFest.hasManager() || !futureFest.hasSession()) {
    Serial.println("Nao ha gestor/sessao configurados para desconectar.");
    return;
  }

  Serial.println("Desconectando totem...");
  TotemStatus status = futureFest.disconnect();
  imprimirStatusTotem(status);
  corRoxa();
}

void executarDebugHardware(const String& comando) {
  if (comando == "verde") {
    corVerde();
    Serial.println("Debug cor: VERDE");
  } else if (comando == "amarelo") {
    corAmarela();
    Serial.println("Debug cor: AMARELO");
  } else if (comando == "vermelho") {
    corVermelha();
    Serial.println("Debug cor: VERMELHO");
  } else if (comando == "azul") {
    corAzul();
    Serial.println("Debug cor: AZUL");
  } else if (comando == "apagar") {
    apagarFita();
    Serial.println("Debug: fita apagada");
  } else if (comando == "on") {
    ligarBomba();
    Serial.println("Debug: bomba LIGADA");
  } else if (comando == "off") {
    desligarBomba();
    Serial.println("Debug: bomba DESLIGADA");
  } else {
    Serial.println("Debug invalido. Use: debug verde, debug amarelo, debug vermelho, debug azul, debug apagar, debug on, debug off.");
  }
}

void consultarStatusTotem() {
  if (!garantirConfiguracaoTotem()) return;

  TotemStatus status = futureFest.status();
  imprimirStatusTotem(status);
  aplicarVidaNoHardware(status);
  ultimoStatusMs = millis();
}

void tickFutureFest() {
  if (!futureFest.isConnected()) return;
  if (WiFi.status() != WL_CONNECTED) return;

  unsigned long agora = millis();

  if (agora - ultimoStatusMs >= STATUS_INTERVAL_MS) {
    consultarStatusTotem();
  }

  if (agora - ultimoHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
    TotemStatus status = futureFest.connect();
    if (!status.ok) {
      Serial.println("Falha no heartbeat do totem.");
      imprimirStatusTotem(status);
    }
    ultimoHeartbeatMs = agora;
  }
}

void imprimirAjuda() {
  Serial.println();
  Serial.println("Comandos FutureFest:");
  Serial.println("id 01 | id pessoas  -> gestor Pessoas/Social");
  Serial.println("id 02 | id clima    -> gestor Climaticos");
  Serial.println("id 03 | id flora    -> gestor Plantas/Flora");
  Serial.println("id 04 | id fauna    -> gestor Animais/Fauna");
  Serial.println("sess A7K2           -> salva o codigo da sessao");
  Serial.println("connect A7K2        -> define a sessao e conecta");
  Serial.println("connect             -> conecta usando a sessao salva");
  Serial.println("status              -> consulta vida do gestor");
  Serial.println("disconnect          -> desconecta o totem da sessao atual");
  Serial.println("trocar B9Q4         -> desconecta, troca sessao e conecta");
  Serial.println("info                -> mostra configuracao atual");
  Serial.println();
  Serial.println("Debug manual de hardware:");
  Serial.println("debug verde | debug amarelo | debug vermelho | debug azul | debug apagar");
  Serial.println("debug on  -> liga bomba");
  Serial.println("debug off -> desliga bomba");
  Serial.println("help -> mostra esta ajuda");
  Serial.println();
}

void imprimirInfo() {
  Serial.print("WiFi: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado");
  Serial.print("Gestor: ");
  Serial.print(futureFest.managerId().length() ? futureFest.managerId() : "--");
  Serial.print(" ");
  Serial.println(futureFest.managerLabel());
  Serial.print("Sessao: ");
  Serial.println(futureFest.sessionId().length() ? futureFest.sessionId() : "----");
  Serial.print("API: ");
  Serial.println(FUTUREFEST_API_BASE_URL);
}

String argumentoDepoisDoEspaco(const String& comando) {
  int espaco = comando.indexOf(' ');
  if (espaco < 0) return "";
  String arg = comando.substring(espaco + 1);
  arg.trim();
  return arg;
}

void processarComando(String comando) {
  comando.trim();
  if (comando.length() == 0) return;

  String lower = comando;
  lower.toLowerCase();

  if (lower.startsWith("id ")) {
    String valor = argumentoDepoisDoEspaco(comando);
    if (futureFest.setManagerId(valor)) {
      Serial.print("Gestor definido: ");
      Serial.print(futureFest.managerId());
      Serial.print(" ");
      Serial.println(futureFest.managerLabel());
    } else {
      Serial.println("Gestor invalido. Use 01, 02, 03, 04, pessoas, clima, flora ou fauna.");
    }
  } else if (lower.startsWith("sess ")) {
    String valor = argumentoDepoisDoEspaco(comando);
    if (futureFest.setSessionId(valor)) {
      Serial.print("Sessao definida: ");
      Serial.println(futureFest.sessionId());
    } else {
      Serial.println("Sessao invalida. Use codigo de 4 caracteres.");
    }
  } else if (lower.startsWith("connect")) {
    String valor = argumentoDepoisDoEspaco(comando);
    if (valor.length() > 0 && !futureFest.setSessionId(valor)) {
      Serial.println("Sessao invalida. Use codigo de 4 caracteres.");
      return;
    }
    conectarTotem();
  } else if (lower == "disconnect") {
    desconectarTotem();
    futureFest.clearSession();
    ultimaVidaExibida = -99;
  } else if (lower.startsWith("trocar ")) {
    String valor = argumentoDepoisDoEspaco(comando);
    if (!futureFest.hasManager()) {
      Serial.println("Defina o gestor antes de trocar de sessao. Exemplo: id flora");
      return;
    }
    if (futureFest.hasSession()) {
      desconectarTotem();
    }
    if (!futureFest.setSessionId(valor)) {
      Serial.println("Sessao invalida. Use codigo de 4 caracteres.");
      return;
    }
    ultimaVidaExibida = -99;
    conectarTotem();
  } else if (lower == "status") {
    consultarStatusTotem();
  } else if (lower == "info") {
    imprimirInfo();
  } else if (lower.startsWith("debug ")) {
    executarDebugHardware(argumentoDepoisDoEspaco(lower));
  } else if (lower == "help" || lower == "ajuda") {
    imprimirAjuda();
  } else {
    Serial.println("Comando invalido. Digite help para ver os comandos.");
  }
}

// =========================
// SETUP
// =========================
void setup() {
  Serial.begin(115200);
  delay(600);

  pinMode(PINO_BOMBA, OUTPUT);
  desligarBomba();

  bool okR = ledcAttach(PINO_VERMELHO, FREQUENCIA_PWM, RESOLUCAO_PWM);
  bool okG = ledcAttach(PINO_VERDE, FREQUENCIA_PWM, RESOLUCAO_PWM);
  bool okB = ledcAttach(PINO_AZUL, FREQUENCIA_PWM, RESOLUCAO_PWM);

  if (!okR || !okG || !okB) {
    Serial.println("Erro ao configurar PWM LEDC.");
    while (true) {
      delay(1000);
    }
  }

  futureFest.setDeviceId(DEVICE_ID);
  corAzul();
  conectarWiFi(WIFI_SSID, WIFI_SENHA, WIFI_TIMEOUT_MS);

  Serial.println("Sistema iniciado.");
  imprimirAjuda();
  imprimirInfo();
}

// =========================
// LOOP
// =========================
void loop() {
  if (Serial.available()) {
    String comando = Serial.readStringUntil('\n');
    processarComando(comando);
  }

  tickFutureFest();
}

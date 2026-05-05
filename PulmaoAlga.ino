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
const char* WIFI_SSID = "FIAP-IOT";
const char* WIFI_SENHA = "F!@p25.IOT";
const unsigned long WIFI_TIMEOUT_MS = 15000;

// =========================
// FUTUREFEST SERVER
// =========================
const String FUTUREFEST_API_BASE_URL = "https://ffcvg-production-f333.up.railway.app/api";
const String DEVICE_ID = "pulmao-alga-01";
const String MANAGER_ID = "02";
const unsigned long STATUS_INTERVAL_MS = 5000;
const unsigned long HEARTBEAT_INTERVAL_MS = 15000;
const unsigned long ALERTA_MORTE_INTERVALO_MS = 70;
const unsigned long APAGADO_IDLE_INTERVALO_MS = 250;

FutureFestTotemClient futureFest(FUTUREFEST_API_BASE_URL);
unsigned long ultimoStatusMs = 0;
unsigned long ultimoHeartbeatMs = 0;
unsigned long ultimoAlertaMorteMs = 0;
unsigned long ultimoApagadoIdleMs = 0;
int ultimaVidaExibida = -99;
bool alertaMorteAtivo = false;
bool alertaMorteLigado = false;

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
const bool FITA_COMUM_ANODO = true;
const bool CANAL_VERMELHO_ATIVO_EM_LOW = true;
const bool CANAL_VERDE_ATIVO_EM_LOW = true;
const bool CANAL_AZUL_ATIVO_EM_LOW = true;

// =========================
// FUNCOES AUXILIARES
// =========================
bool canalAtivoEmLow(int pino) {
  if (pino == PINO_VERMELHO) return CANAL_VERMELHO_ATIVO_EM_LOW;
  if (pino == PINO_VERDE) return CANAL_VERDE_ATIVO_EM_LOW;
  if (pino == PINO_AZUL) return CANAL_AZUL_ATIVO_EM_LOW;
  if (FITA_COMUM_ANODO) return true;
  return false;
}

int valorDesligadoDoCanal(int pino) {
  return canalAtivoEmLow(pino) ? HIGH : LOW;
}

void escreverPWM(int pino, int valor) {
  valor = constrain(valor, 0, 255);

  if (canalAtivoEmLow(pino)) {
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

void corVermelhaIntensa() {
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

void forcarFitaApagada() {
  apagarFita();
  digitalWrite(PINO_VERMELHO, valorDesligadoDoCanal(PINO_VERMELHO));
  digitalWrite(PINO_VERDE, valorDesligadoDoCanal(PINO_VERDE));
  digitalWrite(PINO_AZUL, valorDesligadoDoCanal(PINO_AZUL));
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

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Conectando ao WiFi");
    WiFi.disconnect(true);
    delay(200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, senha);

    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - inicio) < timeoutMs) {
      Serial.print(".");
      forcarFitaApagada();
      delay(500);
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println();
      Serial.println("WiFi ainda indisponivel. Nova tentativa em 2 segundos.");
      forcarFitaApagada();
      delay(2000);
    }
  }

  Serial.println();
  Serial.println("WiFi conectado com sucesso.");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  forcarFitaApagada();
  return true;
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
    alertaMorteAtivo = false;
    alertaMorteLigado = false;
    if (status.ok && status.connected) {
      ligarBomba();
      corVerde();
    } else {
      forcarFitaApagada();
      desligarBomba();
    }
    return;
  }

  if (status.gameOver || status.lives <= 0) {
    alertaMorteAtivo = true;
    ultimoAlertaMorteMs = 0;
    ligarBomba();
  } else if (status.lives == 1) {
    alertaMorteAtivo = false;
    alertaMorteLigado = false;
    ligarBomba();
    corVermelha();
  } else if (status.lives == 2) {
    alertaMorteAtivo = false;
    alertaMorteLigado = false;
    ligarBomba();
    corAmarela();
  } else {
    alertaMorteAtivo = false;
    alertaMorteLigado = false;
    ligarBomba();
    corVerde();
  }

  if (ultimaVidaExibida >= 0 && status.lives < ultimaVidaExibida) {
    piscarCor(corVermelha, 3, 100);
  }

  ultimaVidaExibida = status.lives;
}

void tickAlertaMorte() {
  if (!alertaMorteAtivo) return;

  unsigned long agora = millis();
  if (ultimoAlertaMorteMs != 0 && agora - ultimoAlertaMorteMs < ALERTA_MORTE_INTERVALO_MS) {
    return;
  }

  ultimoAlertaMorteMs = agora;
  alertaMorteLigado = !alertaMorteLigado;

  if (alertaMorteLigado) {
    corVermelhaIntensa();
  } else {
    forcarFitaApagada();
  }
}

void manterFitaApagadaQuandoDesconectado() {
  if (futureFest.isConnected() || alertaMorteAtivo) return;

  unsigned long agora = millis();
  if (ultimoApagadoIdleMs != 0 && agora - ultimoApagadoIdleMs < APAGADO_IDLE_INTERVALO_MS) {
    return;
  }

  ultimoApagadoIdleMs = agora;
  forcarFitaApagada();
}

bool garantirConfiguracaoTotem() {
  if (!futureFest.hasManager()) {
    Serial.println("Gestor fixo invalido no firmware. Ajuste MANAGER_ID para 01, 02, 03 ou 04.");
    return false;
  }

  if (!futureFest.hasSession()) {
    Serial.println("Defina ou informe a sessao. Exemplo: connect A7K2");
    return false;
  }

  return true;
}

void solicitarSessaoNoTerminal() {
  Serial.print("Totem fixo do gestor ");
  Serial.print(futureFest.managerId());
  Serial.print(" ");
  Serial.println(futureFest.managerLabel());
  Serial.println("Informe o codigo da sessao para conectar. Exemplo: connect A7K2");
}

void encerrarSessaoLocalInexistente() {
  Serial.println("Sessao nao encontrada no servidor. O lobby pode ter sido encerrado ou resetado.");
  alertaMorteAtivo = false;
  alertaMorteLigado = false;
  desligarBomba();
  piscarCor(corVermelha, 3, 120);
  forcarFitaApagada();
  futureFest.clearSession();
  ultimaVidaExibida = -99;
  solicitarSessaoNoTerminal();
}

void conectarTotem() {
  if (!garantirConfiguracaoTotem()) return;

  Serial.println("Conectando totem ao servidor...");
  TotemStatus status = futureFest.connect();
  imprimirStatusTotem(status);

  if (status.ok) {
    alertaMorteAtivo = false;
    alertaMorteLigado = false;
    ligarBomba();
    ultimoHeartbeatMs = millis();
    ultimoStatusMs = millis();
    ultimaVidaExibida = status.lives;
    if (status.gameOver || status.lives == 0) {
      aplicarVidaNoHardware(status);
    } else {
      piscarCor(corAzul, 3, 120);
      corVerde();
    }
  } else {
    aplicarVidaNoHardware(status);
    if (status.httpCode == 404) {
      encerrarSessaoLocalInexistente();
    }
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
  alertaMorteAtivo = false;
  alertaMorteLigado = false;
  desligarBomba();
  piscarCor(corVermelha, 3, 120);
  forcarFitaApagada();
  solicitarSessaoNoTerminal();
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
  if (status.httpCode == 404) {
    encerrarSessaoLocalInexistente();
    return;
  }
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
      if (status.httpCode == 404) {
        encerrarSessaoLocalInexistente();
        return;
      }
    }
    ultimoHeartbeatMs = agora;
  }
}

void imprimirAjuda() {
  Serial.println();
  Serial.println("Comandos FutureFest:");
  Serial.println("O gestor deste totem e fixo no firmware pela constante MANAGER_ID.");
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
    Serial.println("O gestor deste totem e fixo no firmware. Altere MANAGER_ID e grave novamente para trocar.");
    Serial.print("Gestor atual: ");
    Serial.print(futureFest.managerId());
    Serial.print(" ");
    Serial.println(futureFest.managerLabel());
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

  pinMode(PINO_VERMELHO, OUTPUT);
  pinMode(PINO_VERDE, OUTPUT);
  pinMode(PINO_AZUL, OUTPUT);
  digitalWrite(PINO_VERMELHO, valorDesligadoDoCanal(PINO_VERMELHO));
  digitalWrite(PINO_VERDE, valorDesligadoDoCanal(PINO_VERDE));
  digitalWrite(PINO_AZUL, valorDesligadoDoCanal(PINO_AZUL));

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

  forcarFitaApagada();
  futureFest.setDeviceId(DEVICE_ID);
  if (!futureFest.setManagerId(MANAGER_ID)) {
    Serial.println("MANAGER_ID invalido. Use 01, 02, 03 ou 04.");
    while (true) {
      forcarFitaApagada();
      delay(1000);
    }
  }
  forcarFitaApagada();
  delay(600);
  forcarFitaApagada();
  conectarWiFi(WIFI_SSID, WIFI_SENHA, WIFI_TIMEOUT_MS);
  forcarFitaApagada();

  Serial.println("Sistema iniciado.");
  imprimirAjuda();
  imprimirInfo();
  solicitarSessaoNoTerminal();
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
  tickAlertaMorte();
  manterFitaApagadaQuandoDesconectado();
}

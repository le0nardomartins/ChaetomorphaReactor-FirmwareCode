# PulmaoAlga

Projeto Arduino/ESP32 para controle de:
- fita RGB (via PWM)
- bomba (via rele)
- conexao Wi-Fi

## Funcionalidades
- Controle de cor da fita RGB por comando serial.
- Liga/desliga da bomba por comando serial.
- Inicializacao de rede Wi-Fi no `setup()` com timeout e exibicao de IP.

## Hardware esperado
- ESP32
- Fita RGB (3 canais: R, G, B)
- Modulo rele para bomba

## Mapeamento de pinos
- `PINO_VERDE`: 4
- `PINO_VERMELHO`: 21
- `PINO_AZUL`: 19
- `PINO_BOMBA`: 25

## Configuracao Wi-Fi
No arquivo `PulmaoAlga.ino`, ajuste:

```cpp
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_SENHA = "SUA_SENHA";
```

Opcionalmente, ajuste o timeout:

```cpp
const unsigned long WIFI_TIMEOUT_MS = 15000;
```

## Comandos seriais
Abra o monitor serial em `115200 baud` e envie:
- `verde`
- `amarelo`
- `vermelho`
- `apagar`
- `on` (liga bomba)
- `off` (desliga bomba)

## Fluxo de inicializacao
1. Configura pinos e PWM.
2. Tenta conectar ao Wi-Fi.
3. Define cor inicial verde.
4. Aguarda comandos no serial.

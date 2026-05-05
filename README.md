# PulmaoAlga Totem Firmware

Firmware Arduino/ESP32 para o totem fisico de gestor do FutureFest.

O totem representa uma area fixa da partida, conecta em uma sessao criada no lobby, liga a bomba ao entrar no lobby e acompanha a vida do gestor pela API do servidor.

## Arquivos

```text
PulmaoAlga.ino
FutureFestTotemClient.h
FutureFestTotemClient.cpp
```

`PulmaoAlga.ino` contem pinos, Wi-Fi, comportamento do LED, bomba e comandos seriais.

`FutureFestTotemClient.h` e `FutureFestTotemClient.cpp` contem a comunicacao HTTP com o servidor FutureFest.

## Configuracao antes de gravar

No arquivo `PulmaoAlga.ino`, ajuste o Wi-Fi, o identificador fisico do hardware e o gestor fixo do totem:

```cpp
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_SENHA = "SUA_SENHA";
const String DEVICE_ID = "pulmao-alga-01";
const String MANAGER_ID = "03";
```

`DEVICE_ID` identifica o hardware fisico. Use nomes como `totem-social-01`, `totem-clima-01`, `totem-flora-01` ou `totem-fauna-01`.

`MANAGER_ID` define qual gestor esse hardware representa. Esse valor e fixo no firmware e nao deve ser trocado pelo monitor serial.

| MANAGER_ID | Gestor fisico | Area do jogo |
| --- | --- | --- |
| `01` | Pessoas | Social |
| `02` | Climaticos | Climatica |
| `03` | Plantas | Flora Silvestre |
| `04` | Animais | Fauna Silvestre |

O servidor padrao aponta para producao:

```cpp
const String FUTUREFEST_API_BASE_URL = "https://ffcvg-production-f333.up.railway.app/api";
```

## Boot e Wi-Fi

Ao ligar o ESP32:

1. A fita RGB fica apagada.
2. A bomba fica desligada.
3. O firmware tenta conectar no Wi-Fi indefinidamente.
4. Enquanto o Wi-Fi nao conecta, o sistema nao aceita sessao e nao inicia a operacao do totem.
5. Depois do Wi-Fi conectado, o monitor serial imprime os comandos e solicita o codigo da sessao.

O monitor serial deve ser aberto em `115200 baud`.

## Fluxo de uso

Depois que o Wi-Fi conectar, informe o codigo da sessao de 4 caracteres:

```text
connect A7K2
```

Tambem e possivel salvar a sessao antes e conectar depois:

```text
sess A7K2
connect
```

Para consultar manualmente:

```text
status
```

Para sair da sessao:

```text
disconnect
```

Para trocar de sessao:

```text
trocar B9Q4
```

O comando `trocar` desconecta da sessao anterior, salva o novo codigo e conecta novamente.

## Comandos seriais

```text
sess A7K2           -> salva o codigo da sessao
connect A7K2        -> define a sessao e conecta
connect             -> conecta usando a sessao salva
status              -> consulta vida e status do gestor
disconnect          -> desconecta o totem da sessao atual
trocar B9Q4         -> desconecta, troca sessao e conecta
info                -> mostra Wi-Fi, gestor, sessao e API
help                -> mostra ajuda no serial
ajuda               -> alias de help
```

O comando `id` nao altera mais o gestor. Se alguem digitar `id 01`, o firmware apenas informa o gestor fixo atual e pede para alterar `MANAGER_ID` no codigo e gravar novamente.

## Debug manual

O LED e a bomba sao controlados automaticamente pela API. Os comandos abaixo existem apenas para debug de bancada:

```text
debug verde         -> liga LED verde
debug amarelo       -> liga LED amarelo
debug vermelho      -> liga LED vermelho
debug azul          -> liga LED azul
debug apagar        -> apaga a fita RGB
debug on            -> liga a bomba
debug off           -> desliga a bomba
```

Use esses comandos apenas para testar hardware. Durante a partida, o estado correto vem do servidor.

## Resposta visual e fisica

| Estado do totem | LED | Bomba |
| --- | --- | --- |
| Boot antes do Wi-Fi | Apagado | Desligada |
| Wi-Fi tentando conectar | Apagado | Desligada |
| Wi-Fi conectado, sem sessao | Apagado | Desligada |
| Conectou ao lobby | Pisca azul e fica verde | Ligada |
| Conectado com 3 vidas | Verde | Ligada |
| Conectado com 2 vidas | Amarelo | Ligada |
| Conectado com 1 vida | Vermelho | Ligada |
| Perdeu vida | Pisca vermelho rapidamente e volta para a cor da vida atual | Ligada |
| 0 vidas ou game over | Vermelho piscando rapido continuamente | Ligada |
| Desconectou da sessao | Pisca vermelho e apaga | Desligada |

No lobby, mesmo antes da partida iniciar, o servidor retorna `lives: 3` e `max_lives: 3`.

## Atualizacao automatica

Depois de conectado:

- o totem consulta o status a cada 5 segundos;
- o totem envia heartbeat de conexao a cada 15 segundos;
- o LED e a bomba sao atualizados a partir da resposta da API;
- se o servidor devolver `game_over: true` ou `lives: 0`, o alerta vermelho rapido fica ativo.

## APIs usadas pelo firmware

Base URL de producao:

```text
https://ffcvg-production-f333.up.railway.app/api
```

Endpoints:

```text
POST /api/totems/{managerId}/{sessionId}/connect
GET  /api/totems/{managerId}/{sessionId}/status
POST /api/totems/{managerId}/{sessionId}/disconnect
```

Exemplo para o totem de Plantas na sessao `A7K2`:

```text
POST https://ffcvg-production-f333.up.railway.app/api/totems/03/A7K2/connect
GET  https://ffcvg-production-f333.up.railway.app/api/totems/03/A7K2/status
POST https://ffcvg-production-f333.up.railway.app/api/totems/03/A7K2/disconnect
```

Payload enviado no `connect`:

```json
{
  "device_id": "pulmao-alga-01"
}
```

Resposta esperada:

```json
{
  "session_id": "A7K2",
  "manager_id": "03",
  "manager_label": "Plantas",
  "area_label": "Flora Silvestre",
  "connected": true,
  "device_id": "pulmao-alga-01",
  "session_status": "waiting",
  "current_round": 1,
  "game_over": false,
  "lives": 3,
  "max_lives": 3
}
```

Campos usados pelo firmware:

```text
connected      -> define se o totem esta conectado
game_over      -> ativa alerta de morte/game over
lives          -> define cor do LED e estado de vida
max_lives      -> impresso no serial
current_round  -> impresso no serial
session_status -> impresso no serial
```

## Hardware esperado

- ESP32;
- fita RGB de 3 canais;
- modulo rele para bomba;
- bomba do totem.

## Mapeamento de pinos

```text
PINO_VERDE: 4
PINO_VERMELHO: 21
PINO_AZUL: 19
PINO_BOMBA: 25
```

## Configuracao eletrica da fita

O firmware atual considera os canais RGB como ativos em LOW:

```cpp
const bool FITA_COMUM_ANODO = true;
const bool CANAL_VERMELHO_ATIVO_EM_LOW = true;
const bool CANAL_VERDE_ATIVO_EM_LOW = true;
const bool CANAL_AZUL_ATIVO_EM_LOW = true;
```

Se alguma cor ficar acesa fraca quando deveria estar apagada, verifique inversao do canal, fiação, resistor, transistor ou fuga eletrica no circuito.

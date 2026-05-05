# Chaetomorpha Reactor

Firmware Arduino/ESP32 para o totem fisico do FutureFest.

O codigo controla:

- fita RGB via PWM;
- bomba via rele;
- conexao Wi-Fi;
- conexao do totem ao server FutureFest;
- consulta periodica do nivel de vida do gestor da partida.

## Arquivos

```text
PulmaoAlga.ino
FutureFestTotemClient.h
FutureFestTotemClient.cpp
```

`PulmaoAlga.ino` fica com pinos, hardware, Wi-Fi e comandos seriais.

`FutureFestTotemClient.h` e `FutureFestTotemClient.cpp` ficam com a comunicacao HTTP com o server.

## Configuracao antes de gravar

No arquivo `PulmaoAlga.ino`, ajuste Wi-Fi e identificador do hardware:

```cpp
const char* WIFI_SSID = "SEU_WIFI";
const char* WIFI_SENHA = "SUA_SENHA";
const String DEVICE_ID = "pulmao-alga-01";
```

O server padrao ja aponta para producao:

```cpp
const String FUTUREFEST_API_BASE_URL = "https://ffcvg-production-f333.up.railway.app/api";
```

## IDs dos gestores

O operador define pelo terminal serial qual gestor aquele totem representa.

| ID | Gestor do totem | Comandos aceitos |
| --- | --- | --- |
| `01` | Pessoas / Social | `id 01`, `id pessoas`, `id social` |
| `02` | Climaticos | `id 02`, `id clima`, `id climaticos` |
| `03` | Plantas / Flora | `id 03`, `id plantas`, `id flora` |
| `04` | Animais / Fauna | `id 04`, `id animais`, `id fauna` |

## Fluxo no monitor serial

Abra o monitor serial em `115200 baud`.

1. Defina o gestor:

```text
id flora
```

2. Conecte na partida usando o codigo de 4 caracteres do lobby:

```text
connect A7K2
```

3. Consulte manualmente o status quando quiser:

```text
status
```

4. Para sair da sessao atual:

```text
disconnect
```

5. Para trocar para outra sessao:

```text
trocar B9Q4
```

O comando `trocar` desconecta da sessao anterior, salva o novo codigo e conecta de novo.

## Comandos FutureFest

```text
id 01 | id pessoas  -> gestor Pessoas/Social
id 02 | id clima    -> gestor Climaticos
id 03 | id flora    -> gestor Plantas/Flora
id 04 | id fauna    -> gestor Animais/Fauna
sess A7K2           -> salva o codigo da sessao
connect A7K2        -> define a sessao e conecta
connect             -> conecta usando a sessao salva
status              -> consulta vida do gestor
disconnect          -> desconecta o totem da sessao atual
trocar B9Q4         -> desconecta, troca sessao e conecta
info                -> mostra configuracao atual
help                -> mostra ajuda
```

## Debug manual de hardware

O LED e a bomba sao controlados automaticamente pela vida recebida do server. Os comandos abaixo existem apenas para debug no monitor serial.

```text
debug verde
debug amarelo
debug vermelho
debug azul
debug apagar
debug on
debug off
```

`debug on` liga a bomba.

`debug off` desliga a bomba.

## Atualizacao automatica

Depois de conectado:

- o totem consulta o status a cada 5 segundos;
- o totem envia heartbeat de conexao a cada 15 segundos;
- se a vida cair, a fita pisca em vermelho;
- 3 vidas: verde;
- 2 vidas: amarelo;
- 1 vida: vermelho e bomba ligada;
- 0 vidas ou game over: vermelho e bomba ligada.

## Endpoints usados

```text
POST /api/totems/{managerId}/{sessionId}/connect
GET  /api/totems/{managerId}/{sessionId}/status
POST /api/totems/{managerId}/{sessionId}/disconnect
```

Exemplo:

```text
POST https://ffcvg-production-f333.up.railway.app/api/totems/03/A7K2/connect
GET  https://ffcvg-production-f333.up.railway.app/api/totems/03/A7K2/status
POST https://ffcvg-production-f333.up.railway.app/api/totems/03/A7K2/disconnect
```

## Hardware esperado

- ESP32;
- fita RGB de 3 canais;
- modulo rele para bomba.

## Mapeamento de pinos

```text
PINO_VERDE: 4
PINO_VERMELHO: 21
PINO_AZUL: 19
PINO_BOMBA: 25
```

<div align="center">

# ⚡ Monitor de Energia UPS

### ESP32 • INA219 • ZMPT101B • ST7789 • MQTT • Home Assistant

[![Compilar firmware](https://github.com/mpsc/monitor-energia-ups/actions/workflows/compile.yml/badge.svg)](https://github.com/mpsc/monitor-energia-ups/actions/workflows/compile.yml)
[![Versão](https://img.shields.io/badge/vers%C3%A3o-1.0.0-success.svg)](CHANGELOG.md)
[![Licença: MIT](https://img.shields.io/badge/licen%C3%A7a-MIT-blue.svg)](LICENSE)
[![Placa](https://img.shields.io/badge/placa-ESP32_DevKit-informational.svg)](docs/HARDWARE.md)
[![Apoie](https://img.shields.io/badge/Apoie-doacao-orange.svg)](#apoie-este-projeto)

Sistema de monitoramento de energia para **UPS/nobreak** que mede
**tensão AC (rede)** e **DC (bateria)** em tempo real, exibe em display local,
publica via **MQTT para Home Assistant** e oferece **interface web** na rede local.

> Preview navegável da interface (extraída do firmware): [`preview/index.html`](preview/index.html) · [`preview/config.html`](preview/config.html).

</div>

---

## Índice

- [Recursos](#recursos)
- [Hardware e pinagem](#hardware-e-pinagem)
- [Instalação](#instalação)
- [Primeira configuração](#primeira-configuração)
- [Interface web](#interface-web)
- [MQTT e Home Assistant](#mqtt-e-home-assistant)
- [API HTTP](#api-http)
- [Estrutura do projeto](#estrutura-do-projeto)
- [Compilação (CI)](#compilação-ci)
- [Solução de problemas](#solução-de-problemas)
- [Documentação](#documentação)
- [Contribuindo](#contribuindo)
- [Apoie este projeto](#apoie-este-projeto)
- [Licença](#licença)

## Recursos

- 🔋 **Tensão DC** via INA219 (I2C: SDA 21 / SCL 22) — bateria do nobreak.
- ⚡ **Tensão AC** via ZMPT101B (ADC GPIO 34) com fator de calibração ajustável.
- 🖥️ **Display ST7789** (135x240, SPI) com leituras + status ENERGIA OK/OFF.
- 🌐 **Página web responsiva** (`/`) com atualização a cada 1 s, sem login.
- ⚙️ **Página `/config`** (admin/admin) para Wi-Fi, MQTT, calibração AC e umbrais.
- 🏠 **MQTT + auto-descoberta no Home Assistant** — 3 entidades automáticas.
- 📡 **Portal cativo** WiFiManager (`Monitor-UPS-Setup`) na primeira ligação.
- 🔘 **Botão BOOT (GPIO 0, 10 s)** apaga tudo — recuperação sem USB.
- 🧠 **Histerese**: ON ≥ 80 V, OFF ≤ 70 V (evita pisca-pisca).

## Hardware e pinagem

| Componente | Função | Interface |
|---|---|---|
| ESP32 DevKit | Cérebro do sistema | — |
| INA219 | Mede tensão DC (bateria) | I2C (SDA 21 / SCL 22) |
| ZMPT101B | Mede tensão AC (rede) | ADC (GPIO 34) |
| Display ST7789 | Mostra valores localmente | SPI (MOSI 23, SCLK 18, CS 15, DC 2, RST 4, BL 32) |
| Fonte 5V/USB | Alimentação | — |
| Botão BOOT | Reset de fábrica (10 s) | GPIO 0 |

> Ligação completa, alimentação e **segurança com 110/220 V** em
> [docs/HARDWARE.md](docs/HARDWARE.md).

## Instalação

Precisa de **Arduino IDE 2.x** (ou `arduino-cli`), core **esp32:esp32 2.0.17** e:

```
Adafruit INA219, Adafruit GFX, Adafruit ST7735/ST7789, PubSubClient, WiFiManager
```

```bash
arduino-cli core install esp32:esp32@2.0.17
arduino-cli lib install "Adafruit INA219" "Adafruit GFX Library" "Adafruit ST7735 and ST7789 Library" PubSubClient WiFiManager

arduino-cli compile --fqbn esp32:esp32:esp32 firmware/Monitor_Energia_UPS
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32 firmware/Monitor_Energia_UPS
```

Passo a passo completo em **[docs/INSTALACAO.md](docs/INSTALACAO.md)**.

## Primeira configuração

1. Grave o firmware no ESP32.
2. Sem Wi-Fi salvo → cria a rede aberta **`Monitor-UPS-Setup`**.
3. Conecte o celular/PC nela e abra **http://192.168.4.1**.
4. Preencha Wi-Fi + MQTT e salve → ESP32 reinicia e conecta.
5. O IP aparece no Monitor Serial (115200). Acesse **http://IP_ESP32/**.
6. O Home Assistant reconhece as 3 entidades sozinho.

Troca posterior: **http://IP_ESP32/** → **⚙️ Config** (admin/admin) → salvar → reinicia.
Detalhes em [docs/MANUAL.md](docs/MANUAL.md).

## Interface web

| Endereço | Função | Auth |
|---|---|---|
| `http://IP_ESP32/` | Página principal (tempo real) | Não |
| `http://IP_ESP32/config` | Configuração | admin/admin |
| `http://IP_ESP32/dados` | JSON com leituras | Não |
| `http://IP_ESP32/reset` | Reiniciar ESP32 | admin/admin |
| `http://IP_ESP32/reset?full=1` | Apagar tudo | admin/admin |

- 🟢 **ENERGIA OK** / 🔴 **ENERGIA OFF** (com pulso).
- 🔵 Valor ciano = DC · 🟡 Valor amarelo = AC.
- 🟢/🔴 bolinha = conectado / sem conexão (3 falhas seguidas).
- Preview local navegável em [`preview/`](preview/) — mesma interface do firmware.
- Capturas em `docs/images/` (dados fictícios) serão adicionadas na v1.0.1.

Manual de operação em **[docs/MANUAL.md](docs/MANUAL.md)**.

## MQTT e Home Assistant

| Tópico | Payload | Retained |
|---|---|---|
| `energia/estado` | `ENERGIA OK` / `ENERGIA OFF` | Sim |
| `energia/voltaje_ac` | ex. `127.3` | Sim |
| `energia/voltaje_dc` | ex. `12.60` | Sim |
| `energia/disponivel` | `online` / `offline` (LWT) | Sim |

| Entidade | Tipo |
|---|---|
| `binary_sensor.monitor_ups_energia` | Binário (power, mdi:power-plug) |
| `sensor.monitor_ups_tensao_ac` | Tensão AC em V (mdi:flash) |
| `sensor.monitor_ups_tensao_dc` | Tensão DC em V (mdi:battery) |

Publica a cada 5 s (só se mudou: ΔAC ≥ 1,0 V, ΔDC ≥ 0,05 V) + estado com histerese.
Detalhes, testes e Lovelace em **[docs/MQTT_HOME_ASSISTANT.md](docs/MQTT_HOME_ASSISTANT.md)**.

## API HTTP

| Endpoint | Descrição |
|---|---|
| `GET /` | Página principal |
| `GET /dados` | `{ac, dc, energia, uptime, wifi, mqtt, ssid}` |
| `GET /config` | Página de configuração (auth) |
| `POST /config/save` | Salva e reinicia (auth) |
| `GET /reset` | Reinicia (auth) |
| `GET /reset?full=1` | Apaga NVS e reinicia (auth) |

Referência com exemplos em **[docs/API.md](docs/API.md)**.

## Estrutura do projeto

```
firmware/Monitor_Energia_UPS/Monitor_Energia_UPS.ino  firmware (pasta = nome do .ino)
preview/           index.html + config.html (espelho da interface, com mock de /dados)
docs/              MANUAL, INSTALACAO, HARDWARE, MQTT_HOME_ASSISTANT, API, LICENCAS
.github/           CI (compilação) e templates de issue/PR
CHANGELOG.md       histórico de versões
```

## Compilação (CI)

O workflow [`.github/workflows/compile.yml`](.github/workflows/compile.yml)
compila `firmware/Monitor_Energia_UPS` a cada *push*/PR com `arduino-cli`
+ core `esp32:esp32@2.0.17`.

## Solução de problemas

| Problema | Solução |
|---|---|
| Esqueci senha da `/config` | Segurar BOOT por 10 s |
| Troquei de rede Wi-Fi | Portal `Monitor-UPS-Setup` |
| ESP32 não responde | Segurar BOOT por 10 s |
| MQTT não conecta | Verificar IP/porta na `/config` |
| Valores AC errados | Ajustar fator AC na `/config` (comparar com multímetro) |
| Display preto | Verificar cabos SPI e 3V3 |
| INA219 não detectado | Verificar SDA/SCL (21/22) |

## Documentação

| Documento | Conteúdo |
|---|---|
| [docs/MANUAL.md](docs/MANUAL.md) | Manual de operação e configuração |
| [docs/INSTALACAO.md](docs/INSTALACAO.md) | Compilar e gravar (IDE/CLI) |
| [docs/HARDWARE.md](docs/HARDWARE.md) | Pinagem, ligação e segurança |
| [docs/MQTT_HOME_ASSISTANT.md](docs/MQTT_HOME_ASSISTANT.md) | Tópicos e integração com HA |
| [docs/API.md](docs/API.md) | Referência da API HTTP |
| [docs/LICENCAS.md](docs/LICENCAS.md) | Licenças e créditos de terceiros |
| [CHANGELOG.md](CHANGELOG.md) | Histórico de versões |
| [SECURITY.md](SECURITY.md) | Segurança |

## Contribuindo

Sugestões e correções são bem-vindas! Veja
[CONTRIBUTING.md](CONTRIBUTING.md). Este projeto segue o
[Código de Conduta](CODE_OF_CONDUCT.md).

## Apoie este projeto

Este projeto é gratuito e de código aberto. Se foi útil, considere doar —
qualquer valor ajuda a manter novos sistemas de automação!

[![Doar com PayPal](https://img.shields.io/badge/PayPal-Doar-00457C?logo=paypal&logoColor=white)](https://www.paypal.com/donate/?business=CCQMVDFQXNLCW&no_recurring=0&item_name=Se+puder+me+ajude+a+construir+outros+sistemas+e+controles+para+ajudar+nas+automa%C3%A7%C3%B5es%21&currency_code=BRL)

**PIX** — chave aleatória:

```
b897f5e5-b141-49bc-a787-8ffae93b6873
```

## Licença

Distribuído sob **[MIT](LICENSE)**.

Projeto **independente**, sem vínculo ou endosso da Espressif, Arduino ou
Home Assistant — marcas dos titulares, citadas de forma descritiva. Créditos em
**[docs/LICENCAS.md](docs/LICENCAS.md)**.

---

<div align="center">

Feito por **Marcos (mpsc)** · ⭐ se foi útil

</div>

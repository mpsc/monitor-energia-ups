# Manual de Operação e Configuração

Monitor de Energia UPS — ESP32 • INA219 • ZMPT101B • ST7789 • MQTT • Home Assistant.
Firmware: [`firmware/Monitor_Energia_UPS/Monitor_Energia_UPS.ino`](../firmware/Monitor_Energia_UPS/Monitor_Energia_UPS.ino).

## 1. Visão geral

Mede tensão AC (rede) e DC (bateria) em tempo real, exibe no display local,
publica via MQTT e oferece interface web na rede local.

| Componente | Função | Interface |
|---|---|---|
| ESP32 DevKit | Cérebro do sistema | — |
| INA219 | Tensão DC (bateria) | I2C (SDA 21 / SCL 22) |
| ZMPT101B | Tensão AC (rede) | ADC (GPIO 34) |
| Display ST7789 | Valores locais | SPI |
| Fonte 5V/USB | Alimentação | — |

## 2. Formas de acesso

### A) Página web

| Endereço | Função | Auth |
|---|---|---|
| `http://IP_ESP32/` | Principal (tempo real) | Não |
| `http://IP_ESP32/config` | Configuração | admin/admin |
| `http://IP_ESP32/dados` | JSON com leituras | Não |
| `http://IP_ESP32/reset` | Reiniciar | admin/admin |
| `http://IP_ESP32/reset?full=1` | Apagar tudo | admin/admin |

### B) Portal cativo (WiFiManager)

Sem rede salva → cria `Monitor-UPS-Setup` (aberta):

1. Conecte celular/PC nela.
2. Abra `http://192.168.4.1`.
3. Preencha Wi-Fi + MQTT.
4. Salva → reinicia e conecta.

### C) Serial USB

Baud **115200**: logs de conexão, IP, leituras e erros.

### D) Botão físico (GPIO 0 = BOOT)

Segurar **10 s** → apaga NVS e reinicia. Útil se esquecer senha ou trocar de rede.

### E) Home Assistant (MQTT)

Auto-descoberta cria 3 entidades + `online`/`offline`. Sem editar `configuration.yaml`.

## 3. Operação normal

| Intervalo | Ações |
|---|---|
| 1 s | Lê DC e AC, atualiza display e web |
| 5 s | Publica MQTT (se mudou) + estado energia |
| Contínuo | Mantém WiFi/MQTT, atende web, verifica botão |

Detecção com histerese:

| Situação | Ação |
|---|---|
| AC ≥ 80 V (ON) | ENERGIA OK |
| AC ≤ 70 V (OFF) | ENERGIA OFF |
| Entre 70 e 80 V | Mantém anterior |

## 4. Página /config

| Campo | Descrição |
|---|---|
| Wi-Fi SSID | Nome da rede |
| Wi-Fi Senha | Branco = não troca |
| MQTT Server | IP do broker |
| MQTT Porta | Padrão 1883 |
| MQTT Usuário | Vazio se não usar |
| MQTT Senha | Branco = não troca |
| Fator AC | Ajuste até bater com multímetro |
| Umbral ON | Tensão de "tem energia" |
| Umbral OFF | Tensão de "sem energia" |

> Salvar reinicia automaticamente.

## 5. Fluxos

**Inicial:** gravar → `Monitor-UPS-Setup` → `192.168.4.1` → Wi-Fi+MQTT →
reinicia → IP no Serial → `http://IP` → HA reconhece.

**Troca:** `http://IP` → ⚙️ Config → admin/admin → alterar → Salvar e reiniciar.

## 6. Recuperação

| Situação | Solução |
|---|---|
| Esqueci senha | BOOT 10 s |
| Troquei Wi-Fi | Portal `Monitor-UPS-Setup` |
| Sem resposta | BOOT 10 s |
| MQTT não conecta | IP/porta na `/config` |
| AC errado | Fator AC na `/config` |
| Display preto | SPI e 3V3 |
| INA219 ausente | SDA/SCL 21/22 |

## 7. Padrões

MQTT `192.168.1.8:1883`, `/config` admin/admin, fator AC `0.5`,
ON `80.0 V`, OFF `70.0 V`, leitura 1 s, MQTT 5 s.

## 8. Indicadores

Display: `DC 12.60 V` (ciano), `AC 127.3 V` (amarelo), `ENERGIA: OK` (verde/vermelho).
Web: 🟢 OK / 🔴 OFF, 🔵 DC, 🟡 AC, bolinha 🟢/🔴 = conexão.

## 9. Checklist

- [ ] Firmware gravado · Wi-Fi + MQTT OK · HA com 3 entidades
- [ ] Display + web OK · Fator AC calibrado · Umbrais 127/220 V ajustados
- [ ] Senha `/config` trocada (HTTP sem HTTPS — só rede local confiável)

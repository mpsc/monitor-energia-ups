# Instalação e gravação

## Pré-requisitos

- Arduino IDE 2.x **ou** `arduino-cli`.
- Core `esp32:esp32` **2.0.17**.
- Bibliotecas: **Adafruit INA219**, **Adafruit GFX**, **Adafruit ST7735/ST7789**, **PubSubClient**, **WiFiManager**.

## Estrutura

```
firmware/Monitor_Energia_UPS/Monitor_Energia_UPS.ino
```

> O Arduino exige pasta = nome do `.ino`. Não renomeie só um dos dois.

## IDE

1. Boards Manager → **esp32** (Espressif) 2.0.17.
2. Library Manager → instale as 5 libs acima.
3. Abra `firmware/Monitor_Energia_UPS/Monitor_Energia_UPS.ino`.
4. Placa **ESP32 Dev Module**, porta COM correta.
5. **Verificar** → **Carregar**.

## CLI

```bash
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
arduino-cli lib install "Adafruit INA219" "Adafruit GFX Library" "Adafruit ST7735 and ST7789 Library" PubSubClient WiFiManager

arduino-cli compile --fqbn esp32:esp32:esp32 firmware/Monitor_Energia_UPS
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32 firmware/Monitor_Energia_UPS
arduino-cli monitor -p COM3 --config 115200
```

## Problemas comuns

| Sintoma | Solução |
|---|---|
| `Timed out` ao gravar | Segure BOOT, inicie, solte; cabo de dados; porta certa |
| `main file missing` | Pasta ≠ `.ino` — renomeie |
| `ERROR INA219` no display/Serial | SDA/SCL 21/22, VCC 3V3, endereço I2C |
| Display preto | SPI (23/18/15/2/4), BL 32, VCC 3V3 |
| Sem portal `Monitor-UPS-Setup` | Já há Wi-Fi salvo; BOOT 10 s para limpar |
| AC sempre 0 | ZMPT OUT no 34, VCC 5V, trimpot + fator AC |

## Após gravar

Primeira vez sobe em portal cativo — siga o [README](../README.md#primeira-configuração).

# Hardware, ligação e segurança

## Alvo

| Item | Valor |
|---|---|
| Placa | ESP32 DevKit (clássico, dual-core 240 MHz) |
| Flash | 4 MB |
| Lógica | 3,3 V |
| USB-serial | CP2102 ou CH340 |

## Pinagem do firmware

| Função | Pino |
|---|---|
| TFT MOSI | GPIO 23 |
| TFT SCLK | GPIO 18 |
| TFT CS | GPIO 15 |
| TFT DC | GPIO 2 |
| TFT RST | GPIO 4 |
| TFT BL (backlight) | GPIO 32 (HIGH) |
| INA219 SDA | GPIO 21 |
| INA219 SCL | GPIO 22 |
| ZMPT101B (analógico) | GPIO 34 (attenuation 11 dB) |
| Botão reset fábrica | GPIO 0 (BOOT, INPUT_PULLUP) |

Display: `tft.init(135, 240)`, rotação 1.

## Ligações

```
ST7789          ESP32
MOSI  ───────── 23
SCLK  ───────── 18
CS    ───────── 15
DC    ───────── 2
RST   ───────── 4
BL    ───────── 32 (ou 3V3)
VCC   ───────── 3V3  (!! não use 5V no display 3V3)
GND   ───────── GND

INA219          ESP32
SDA   ───────── 21 (pull-up 4k7 se o módulo não tiver)
SCL   ───────── 22
VCC   ───────── 3V3
GND   ───────── GND
VIN+  ───────── positivo da bateria / barramento DC
VIN-  ───────── GND comum

ZMPT101B        ESP32
OUT   ───────── 34
VCC   ───────── 5V (módulo) — ajuste o trimpot com multímetro
GND   ───────── GND
L/N   ───────── rede AC (com fusível + isolamento!)
```

## Calibração AC

1. Meça a rede com multímetro confiável.
2. Em `/config`, ajuste **Fator AC** até o display bater.
3. Padrão `0.5`. O firmware faz RMS de 1500+1500 amostras × fator.
4. Leituras < 70 V são zeradas (corte de ruído).

## ⚠️ Segurança elétrica

- ZMPT101B lida com **110/220 V AC — risco de choque e incêndio**.
- Trabalhe desenergizado, use fusível no primário, caixa isolada e NBR 5410.
- GND comum obrigatório entre ESP32, INA219 e ZMPT101B (lado baixa tensão).
- Não alimente o display 3V3 com 5V.
- Sem certificação — uso por sua conta e risco.

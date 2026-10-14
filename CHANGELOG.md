# Changelog

Todas as mudanças relevantes deste projeto são documentadas neste arquivo.

O formato segue o [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/)
e o projeto adere ao [Versionamento Semântico](https://semver.org/lang/pt-BR/).

## [Não lançado]

## [1.0.0] - 2026-10-14

### Adicionado

- Monitor de tensão DC (INA219 via I2C) e AC (ZMPT101B via ADC GPIO 34).
- Display ST7789 135x240 com voltajes e status ENERGIA OK/OFF.
- Interface web responsiva em `/` com atualização a cada 1 s via `/dados` (JSON).
- Página `/config` (auth `admin/admin`) para Wi-Fi, MQTT, fator AC e umbrais.
- Endpoints `/reset` (reinicia) e `/reset?full=1` (apaga NVS).
- Botão BOOT (GPIO 0, 10 s) apaga tudo e reinicia.
- MQTT com LWT (`energia/disponivel`), retained e auto-descoberta no Home Assistant:
  `binary_sensor.monitor_ups_energia`, `sensor.monitor_ups_tensao_ac`,
  `sensor.monitor_ups_tensao_dc`.
- Detecção de energia com histerese (ON ≥ 80 V, OFF ≤ 70 V).
- Portal cativo WiFiManager (`Monitor-UPS-Setup`, `192.168.4.1`) na primeira ligação.
- Documentação padrão GitHub: README, `docs/` (MANUAL, INSTALACAO, HARDWARE,
  MQTT_HOME_ASSISTANT, API, LICENCAS), CI de compilação, LICENSE MIT.

[1.0.0]: https://github.com/mpsc/monitor-energia-ups/releases/tag/v1.0.0

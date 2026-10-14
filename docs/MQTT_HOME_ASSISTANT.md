# MQTT e Home Assistant

Auto-descoberta — sem editar `configuration.yaml`.

## Pré-requisitos

- Broker (ex. Mosquitto) + integração **MQTT** no HA apontando para ele.
- No firmware (`/config`): IP, porta (1883), usuário/senha do broker.

## Tópicos

| Tópico | Payload | Retained | Descrição |
|---|---|:---:|---|
| `energia/estado` | `ENERGIA OK` / `ENERGIA OFF` | ✅ | Estado com histerese |
| `energia/voltaje_ac` | ex. `127.3` | ✅ | AC (1 casa) |
| `energia/voltaje_dc` | ex. `12.60` | ✅ | DC (2 casas) |
| `energia/disponivel` | `online` / `offline` (LWT) | ✅ | Disponibilidade |

Publica a cada 5 s, só se mudou (ΔAC ≥ 1,0 V, ΔDC ≥ 0,05 V).
Discovery republicado a cada conexão, com `availability_topic` e `expire_after: 30`.

## Entidades

| Entidade | Tipo | Discovery |
|---|---|---|
| `binary_sensor.monitor_ups_energia` | power, `mdi:power-plug` | `homeassistant/binary_sensor/monitor_energia_ups/energia/config` |
| `sensor.monitor_ups_tensao_ac` | voltage V, `mdi:flash` | `homeassistant/sensor/monitor_energia_ups/voltaje_ac/config` |
| `sensor.monitor_ups_tensao_dc` | voltage V, `mdi:battery` | `homeassistant/sensor/monitor_energia_ups/voltaje_dc/config` |

Dispositivo: `Monitor Energia UPS` (`ESP32 DIY`, `sw_version 1.0.0`,
`configuration_url http://<IP>`).

## Testes

```bash
mosquitto_sub -h 192.168.1.8 -t "energia/#" -v
mosquitto_pub -h 192.168.1.8 -t "energia/estado" -m "ENERGIA OK" -r
curl http://IP_ESP32/dados
```

## Lovelace

```yaml
type: entities
title: UPS
entities:
  - binary_sensor.monitor_ups_energia
  - sensor.monitor_ups_tensao_ac
  - sensor.monitor_ups_tensao_dc
```

## Órfãs

Renomeou algo e duplicou? Remova em HA → MQTT → tópicos descobertos.

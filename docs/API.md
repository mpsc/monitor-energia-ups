# API HTTP (JSON)

Base: `http://<IP_ESP32>/`. `/` e `/dados` sem auth; resto com `admin/admin`
(HTTP basic, sem HTTPS — só rede local).

## Páginas

| Método | Rota | Descrição |
|---|---|---|
| `GET` | `/` | Principal (tempo real) |
| `GET` | `/config` | Configuração (auth) |
| `POST` | `/config/save` | Salva (form) e reinicia em 3 s (auth) |
| `GET` | `/reset` | Reinicia (auth) |
| `GET` | `/reset?full=1` | Apaga NVS + reinicia (auth) |

## `GET /dados`

```json
{
  "ac": 127.30,
  "dc": 12.60,
  "energia": true,
  "uptime": 123,
  "wifi": -62,
  "mqtt": true,
  "ssid": "MinhaRede"
}
```

| Campo | Tipo |
|---|---|
| `ac`/`dc` | float (V) |
| `energia` | bool (histerese ON ≥ 80 / OFF ≤ 70) |
| `uptime` | segundos |
| `wifi` | RSSI dBm |
| `mqtt` | bool conectado |
| `ssid` | string |

```bash
curl http://192.168.1.50/dados
```

PowerShell:

```powershell
Invoke-RestMethod "http://192.168.1.50/dados"
```

## `POST /config/save`

Campos: `wifi_ssid`, `wifi_pass` (branco mantém), `mqtt_server`,
`mqtt_port`, `mqtt_user`, `mqtt_pass` (branco mantém),
`fator_ac` (>0), `umbral_on`, `umbral_off`.

Retorna `OK` (texto) e reinicia.

## Códigos

| Código | Quando |
|---|---|
| 200 | OK |
| 401 | Sem auth em rota protegida |
| 404 | Rota inexistente |

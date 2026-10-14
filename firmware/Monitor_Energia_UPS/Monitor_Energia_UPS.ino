#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Preferences.h>

// ==========================================
// PINOS
// ==========================================

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BL   32

#define INA219_SDA 21
#define INA219_SCL 22

#define ZMPT_PIN 34

#define BOTAO_RESET 0   // Botão BOOT do ESP32

// ==========================================
// OBJETOS
// ==========================================

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_INA219 ina219;

WiFiClient   espClient;
PubSubClient mqttClient(espClient);
WebServer    server(80);
Preferences  preferences;
WiFiManager  wifiManager;

// ==========================================
// TOPICOS MQTT FIXOS
// ==========================================

const char* MQTT_TOPIC_ENERGIA   = "energia/estado";
const char* MQTT_TOPIC_AC        = "energia/voltaje_ac";
const char* MQTT_TOPIC_DC        = "energia/voltaje_dc";
const char* MQTT_TOPIC_AVAIL     = "energia/disponivel";

const char* MQTT_DISCOVERY_ENERGIA =
  "homeassistant/binary_sensor/monitor_energia_ups/energia/config";
const char* MQTT_DISCOVERY_AC =
  "homeassistant/sensor/monitor_energia_ups/voltaje_ac/config";
const char* MQTT_DISCOVERY_DC =
  "homeassistant/sensor/monitor_energia_ups/voltaje_dc/config";

// ==========================================
// CONFIGURACOES (persistidas na NVS)
// ==========================================

struct Config {
  String mqttServer;
  int    mqttPort;
  String mqttUser;
  String mqttPassword;
  String wifiSSID;
  String wifiPassword;
  float  fatorCalibracaoAC;
  float  umbralEnergiaON;
  float  umbralEnergiaOFF;
};

Config cfg;

const char* DEFAULT_MQTT_SERVER = "192.168.1.8";
const int   DEFAULT_MQTT_PORT   = 1883;
const char* DEFAULT_MQTT_USER   = "";
const char* DEFAULT_MQTT_PASS   = "";
const float DEFAULT_FATOR_AC    = 0.5;
const float DEFAULT_UMBRAL_ON   = 80.0;
const float DEFAULT_UMBRAL_OFF  = 70.0;

// Senha para acessar /config (deixe vazio para não exigir login)
const char* CONFIG_USER     = "admin";
const char* CONFIG_PASSWORD = "admin";

// ==========================================
// VARIAVEIS GLOBAIS
// ==========================================

float voltajeDC = 0.0;
float voltajeAC = 0.0;

unsigned long ultimoIntentoMQTT = 0;
const unsigned long intervaloMQTT = 5000;

bool estadoEnergiaAnterior     = false;
bool estadoEnergiaInicializado = false;

unsigned long ultimaLectura     = 0;
unsigned long ultimaPantalla    = 0;
unsigned long ultimaPublicacion = 0;

const unsigned long intervaloLectura   = 1000;
const unsigned long intervaloPantalla  = 1000;
const unsigned long intervaloPublicar  = 5000;

float ultimoACPublicado = -1000.0;
float ultimoDCPublicado = -1000.0;

const float deltaAC = 1.0;
const float deltaDC = 0.05;

// ==========================================
// PROTOTIPOS
// ==========================================

void carregarConfiguracoes();
void salvarConfiguracoes();
void resetarConfiguracoes();

void iniciarWiFiManager();
void iniciarServidorWeb();
void iniciarMQTT();

void handleRoot();
void handleDados();
void handleConfigPage();
void handleConfigSave();
void handleReset();
void handleNotFound();

void publicarMQTTDiscovery();
void conectarMQTT();
void revisarMQTT();
void publicarEstadoEnergia();
void publicarVoltajesMQTT();

void leerVoltajeDC();
void leerVoltajeAC();

void dibujarPantallaCompleta();
void actualizarValoresPantalla();
void mostrarTelaConfiguracao();

bool verificarAutenticacao();
void verificarBotaoReset();

// ==========================================
// PAGINA PRINCIPAL
// ==========================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Monitor de Energia UPS</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  html, body { height: 100%; width: 100%; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    background: #0f1419;
    color: #e6e6e6;
    display: grid;
    place-items: center;
    min-height: 100vh;
    min-height: 100dvh;
    padding: 20px;
  }
  .container {
    width: 100%;
    max-width: 520px;
    background: #1a2028;
    border-radius: 16px;
    box-shadow: 0 20px 40px rgba(0,0,0,0.5);
    overflow: hidden;
    align-self: center;
    justify-self: center;
  }
  .header {
    background: linear-gradient(135deg, #1e3a5f, #2d5a8c);
    padding: 22px;
    text-align: center;
    position: relative;
  }
  .header h1 { font-size: 1.35rem; font-weight: 600; letter-spacing: 0.5px; }
  .header p { font-size: 0.78rem; opacity: 0.75; margin-top: 4px; }
  .btn-config {
    position: absolute;
    top: 12px;
    right: 12px;
    background: rgba(255,255,255,0.15);
    color: #fff;
    border: none;
    padding: 6px 12px;
    border-radius: 6px;
    cursor: pointer;
    font-size: 0.75rem;
    text-decoration: none;
    transition: background 0.2s;
  }
  .btn-config:hover { background: rgba(255,255,255,0.3); }
  .status {
    padding: 22px;
    text-align: center;
    font-size: 1.5rem;
    font-weight: bold;
    letter-spacing: 1px;
    transition: all 0.3s ease;
  }
  .status.ok { background: #0d3d1f; color: #4ade80; border-bottom: 1px solid #166534; }
  .status.off {
    background: #3d0d0d;
    color: #f87171;
    border-bottom: 1px solid #991b1b;
    animation: pulse 1.5s infinite;
  }
  @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.6; } }
  .valores {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 1px;
    background: #2a3340;
  }
  .card { background: #1a2028; padding: 24px 16px; text-align: center; }
  .card .label {
    font-size: 0.75rem;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    color: #8b96a5;
    margin-bottom: 8px;
  }
  .card .value {
    font-size: 2.4rem;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
  }
  .card .unit { font-size: 0.9rem; color: #8b96a5; margin-left: 4px; }
  .card.dc .value { color: #22d3ee; }
  .card.ac .value { color: #fbbf24; }
  .footer {
    padding: 14px;
    text-align: center;
    font-size: 0.72rem;
    color: #55606e;
    background: #0f1419;
    display: flex;
    justify-content: space-between;
    align-items: center;
  }
  .dot {
    display: inline-block;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: #ef4444;
    margin-right: 6px;
    transition: background 0.3s;
  }
  .dot.online { background: #4ade80; }
  .dot.offline { background: #ef4444; }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <a href="/config" class="btn-config">⚙️ Config</a>
    <h1>⚡ Monitor de Energia UPS</h1>
    <p>ESP32 • INA219 • ZMPT101B</p>
  </div>
  <div id="status" class="status off">CARREGANDO...</div>
  <div class="valores">
    <div class="card dc">
      <div class="label">Voltaje DC</div>
      <div><span class="value" id="vdc">--.--</span><span class="unit">V</span></div>
    </div>
    <div class="card ac">
      <div class="label">Voltaje AC</div>
      <div><span class="value" id="vac">---.-</span><span class="unit">V</span></div>
    </div>
  </div>
  <div class="footer">
    <span><span id="dot" class="dot"></span><span id="conn">Conectando...</span></span>
    <span id="uptime">--</span>
  </div>
</div>
<script>
let falhas = 0;
function formatarUptime(s) {
  const d = Math.floor(s/86400), h = Math.floor((s%86400)/3600);
  const m = Math.floor((s%3600)/60), sec = s%60;
  if (d>0) return `${d}d ${h}h ${m}m`;
  if (h>0) return `${h}h ${m}m ${sec}s`;
  if (m>0) return `${m}m ${sec}s`;
  return `${sec}s`;
}
async function atualizar() {
  try {
    const resp = await fetch('/dados', { cache: 'no-store' });
    if (!resp.ok) throw new Error('HTTP ' + resp.status);
    const d = await resp.json();
    document.getElementById('vdc').textContent = Number(d.dc).toFixed(2);
    document.getElementById('vac').textContent = Number(d.ac).toFixed(1);
    const st = document.getElementById('status');
    if (d.energia) {
      st.textContent = '✓ ENERGIA OK';
      st.className = 'status ok';
    } else {
      st.textContent = '⚠ ENERGIA OFF';
      st.className = 'status off';
    }
    document.getElementById('dot').className = 'dot online';
    document.getElementById('conn').textContent = 'Online • ' + d.wifi + ' dBm';
    document.getElementById('uptime').textContent = 'Uptime: ' + formatarUptime(d.uptime);
    falhas = 0;
  } catch (e) {
    falhas++;
    if (falhas >= 3) {
      document.getElementById('dot').className = 'dot offline';
      document.getElementById('conn').textContent = 'Sem conexão';
    }
  }
}
atualizar();
setInterval(atualizar, 1000);
</script>
</body>
</html>
)rawliteral";

// ==========================================
// PAGINA DE CONFIGURACAO
// ==========================================

const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Configuração - Monitor UPS</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  html, body { height: 100%; }
  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    background: #0f1419;
    color: #e6e6e6;
    display: grid;
    place-items: center;
    min-height: 100vh;
    min-height: 100dvh;
    padding: 20px;
  }
  .container {
    width: 100%;
    max-width: 560px;
    background: #1a2028;
    border-radius: 16px;
    box-shadow: 0 20px 40px rgba(0,0,0,0.5);
    overflow: hidden;
    align-self: center;
  }
  .header {
    background: linear-gradient(135deg, #5f3a1e, #8c5a2d);
    padding: 22px;
    text-align: center;
    position: relative;
  }
  .header h1 { font-size: 1.35rem; font-weight: 600; }
  .header p { font-size: 0.78rem; opacity: 0.75; margin-top: 4px; }
  .btn-voltar {
    position: absolute;
    top: 12px;
    left: 12px;
    background: rgba(255,255,255,0.15);
    color: #fff;
    border: none;
    padding: 6px 12px;
    border-radius: 6px;
    cursor: pointer;
    font-size: 0.75rem;
    text-decoration: none;
    transition: background 0.2s;
  }
  .btn-voltar:hover { background: rgba(255,255,255,0.3); }
  .form { padding: 24px; }
  .secao {
    margin-bottom: 24px;
    padding-bottom: 20px;
    border-bottom: 1px solid #2a3340;
  }
  .secao:last-of-type { border-bottom: none; }
  .secao h2 {
    font-size: 0.9rem;
    color: #8b96a5;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    margin-bottom: 14px;
  }
  label {
    display: block;
    font-size: 0.8rem;
    color: #a8b2bf;
    margin-bottom: 6px;
    margin-top: 12px;
  }
  input {
    width: 100%;
    background: #0f1419;
    border: 1px solid #2a3340;
    border-radius: 8px;
    color: #e6e6e6;
    padding: 12px 14px;
    font-size: 0.95rem;
    font-family: inherit;
    transition: border 0.2s;
  }
  input:focus {
    outline: none;
    border-color: #3b82f6;
  }
  .linha {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
  }
  .botoes {
    display: flex;
    gap: 10px;
    margin-top: 20px;
    flex-wrap: wrap;
  }
  button {
    flex: 1;
    padding: 13px 20px;
    border-radius: 8px;
    border: none;
    font-size: 0.9rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
    font-family: inherit;
    min-width: 140px;
  }
  .btn-salvar {
    background: linear-gradient(135deg, #166534, #22c55e);
    color: white;
  }
  .btn-salvar:hover { background: linear-gradient(135deg, #14532d, #16a34a); }
  .btn-reiniciar {
    background: #2a3340;
    color: #e6e6e6;
  }
  .btn-reiniciar:hover { background: #3a4450; }
  .btn-reset {
    background: #7f1d1d;
    color: white;
  }
  .btn-reset:hover { background: #991b1b; }
  .alerta {
    background: #0d3d1f;
    border-left: 4px solid #22c55e;
    padding: 14px 16px;
    border-radius: 8px;
    margin-bottom: 20px;
    font-size: 0.85rem;
    color: #86efac;
  }
  .aviso {
    background: #422006;
    border-left: 4px solid #f59e0b;
    padding: 14px 16px;
    border-radius: 8px;
    margin-top: 16px;
    font-size: 0.8rem;
    color: #fcd34d;
  }
  .info {
    background: #0c1e2e;
    border-left: 4px solid #3b82f6;
    padding: 14px 16px;
    border-radius: 8px;
    margin-top: 16px;
    font-size: 0.8rem;
    color: #93c5fd;
  }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <a href="/" class="btn-voltar">← Voltar</a>
    <h1>⚙️ Configuração</h1>
    <p>Monitor de Energia UPS</p>
  </div>
  <div class="form">

    <div id="alerta" class="alerta" style="display:none;">
      ✓ Configurações salvas! Reiniciando ESP32 em 3 segundos...
    </div>

    <form id="form-config" method="POST" action="/config/save">

      <div class="secao">
        <h2>📶 Rede Wi-Fi</h2>
        <label>Nome da rede (SSID)</label>
        <input type="text" name="wifi_ssid" value="%WIFI_SSID%" placeholder="MinhaRede">
        <label>Senha do Wi-Fi</label>
        <input type="password" name="wifi_pass" placeholder="Deixe em branco para manter a atual">
        <div class="aviso">
          ⚠️ Se mudar o Wi-Fi, o ESP32 vai reiniciar e tentar conectar na nova rede.
        </div>
      </div>

      <div class="secao">
        <h2>📡 Broker MQTT</h2>
        <div class="linha">
          <div>
            <label>IP do servidor</label>
            <input type="text" name="mqtt_server" value="%MQTT_SERVER%" placeholder="192.168.1.8">
          </div>
          <div>
            <label>Porta</label>
            <input type="number" name="mqtt_port" value="%MQTT_PORT%" placeholder="1883">
          </div>
        </div>
        <label>Usuário MQTT</label>
        <input type="text" name="mqtt_user" value="%MQTT_USER%" placeholder="(vazio se não usar)">
        <label>Senha MQTT</label>
        <input type="password" name="mqtt_pass" placeholder="Deixe em branco para manter a atual">
      </div>

      <div class="secao">
        <h2>🔧 Calibração</h2>
        <label>Fator de calibração AC</label>
        <input type="number" step="0.01" name="fator_ac" value="%FATOR_AC%" placeholder="0.5">
        <div class="linha">
          <div>
            <label>Umbral AC (liga)</label>
            <input type="number" step="0.1" name="umbral_on" value="%UMBRAL_ON%" placeholder="80">
          </div>
          <div>
            <label>Umbral AC (desliga)</label>
            <input type="number" step="0.1" name="umbral_off" value="%UMBRAL_OFF%" placeholder="70">
          </div>
        </div>
        <div class="info">
          ℹ️ O <strong>fator AC</strong> é multiplicado pela leitura do ZMPT101B.
          Ajuste até o valor no display coincidir com o multímetro.
        </div>
      </div>

      <div class="botoes">
        <button type="submit" class="btn-salvar">💾 Salvar e reiniciar</button>
        <button type="button" class="btn-reiniciar" onclick="reiniciar()">🔄 Só reiniciar</button>
      </div>

    </form>

    <div class="botoes">
      <button type="button" class="btn-reset" onclick="resetar()">⚠️ Apagar tudo e reconfigurar do zero</button>
    </div>

    <div class="aviso">
      ℹ️ <strong>Apagar tudo</strong> remove Wi-Fi e MQTT salvos. O ESP32 vai criar a rede
      <strong>Monitor-UPS-Setup</strong> para você reconfigurar pelo celular.
    </div>

  </div>
</div>

<script>
document.getElementById('form-config').addEventListener('submit', function(e) {
  e.preventDefault();
  const form = e.target;
  const dados = new FormData(form);
  fetch('/config/save', {
    method: 'POST',
    body: new URLSearchParams(dados)
  }).then(r => r.text()).then(resp => {
    document.getElementById('alerta').style.display = 'block';
    document.getElementById('alerta').scrollIntoView({behavior:'smooth'});
    setTimeout(() => { window.location.href = '/'; }, 6000);
  }).catch(err => {
    alert('Erro ao salvar: ' + err);
  });
});

function reiniciar() {
  if (confirm('Reiniciar o ESP32 agora?')) {
    fetch('/reset').then(() => {
      alert('Reiniciando... Aguarde 15 segundos.');
    });
  }
}

function resetar() {
  if (confirm('⚠️ Isso vai APAGAR TODAS as configurações. Continuar?')) {
    fetch('/reset?full=1').then(() => {
      alert('Configurações apagadas.\n\n1. Aguarde 15s\n2. Conecte na rede Monitor-UPS-Setup\n3. Acesse 192.168.4.1');
    });
  }
}
</script>
</body>
</html>
)rawliteral";

// ==========================================
// CARREGAR / SALVAR / RESETAR
// ==========================================

void carregarConfiguracoes() {

  preferences.begin("monitor-ups", true);

  cfg.mqttServer        = preferences.getString("mqtt_server", DEFAULT_MQTT_SERVER);
  cfg.mqttPort          = preferences.getInt("mqtt_port", DEFAULT_MQTT_PORT);
  cfg.mqttUser          = preferences.getString("mqtt_user", DEFAULT_MQTT_USER);
  cfg.mqttPassword      = preferences.getString("mqtt_pass", DEFAULT_MQTT_PASS);
  cfg.wifiSSID          = preferences.getString("wifi_ssid", "");
  cfg.wifiPassword      = preferences.getString("wifi_pass", "");
  cfg.fatorCalibracaoAC = preferences.getFloat("fator_ac", DEFAULT_FATOR_AC);
  cfg.umbralEnergiaON   = preferences.getFloat("umbral_on", DEFAULT_UMBRAL_ON);
  cfg.umbralEnergiaOFF  = preferences.getFloat("umbral_off", DEFAULT_UMBRAL_OFF);

  preferences.end();

  Serial.println("=== Configuracoes carregadas ===");
  Serial.print("MQTT: "); Serial.print(cfg.mqttServer);
  Serial.print(":"); Serial.println(cfg.mqttPort);
  Serial.print("WiFi SSID: "); Serial.println(cfg.wifiSSID);
  Serial.print("Fator AC: "); Serial.println(cfg.fatorCalibracaoAC, 3);
  Serial.println("================================");
}

void salvarConfiguracoes() {

  preferences.begin("monitor-ups", false);
  preferences.putString("mqtt_server", cfg.mqttServer);
  preferences.putInt("mqtt_port", cfg.mqttPort);
  preferences.putString("mqtt_user", cfg.mqttUser);
  preferences.putString("mqtt_pass", cfg.mqttPassword);
  preferences.putString("wifi_ssid", cfg.wifiSSID);
  preferences.putString("wifi_pass", cfg.wifiPassword);
  preferences.putFloat("fator_ac", cfg.fatorCalibracaoAC);
  preferences.putFloat("umbral_on", cfg.umbralEnergiaON);
  preferences.putFloat("umbral_off", cfg.umbralEnergiaOFF);
  preferences.end();

  Serial.println("Configuracoes salvas na NVS");
}

void resetarConfiguracoes() {
  preferences.begin("monitor-ups", false);
  preferences.clear();
  preferences.end();
  Serial.println("Configuracoes apagadas");
}

// ==========================================
// WIFI MANAGER
// ==========================================

void iniciarWiFiManager() {

  Serial.println();
  Serial.println("=== WiFiManager ===");

  if (cfg.wifiSSID.length() > 0) {
    WiFi.begin(cfg.wifiSSID.c_str(), cfg.wifiPassword.c_str());

    Serial.print("Conectando a WiFi salvo");
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - inicio < 15000) {
      delay(500);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi conectado!");
      Serial.print("IP: "); Serial.println(WiFi.localIP());
      return;
    }
    Serial.println("Falha. Abrindo portal...");
  } else {
    Serial.println("Sem credenciais. Abrindo portal...");
  }

  mostrarTelaConfiguracao();

  wifiManager.setConfigPortalTimeout(300);
  wifiManager.setAPCallback([](WiFiManager* wm) {
    Serial.println();
    Serial.println("================================");
    Serial.println("MODO CONFIGURACAO ATIVO");
    Serial.println("1. Conecte-se a rede: Monitor-UPS-Setup");
    Serial.println("2. Abra: http://192.168.4.1");
    Serial.println("================================");
  });

  WiFiManagerParameter p_srv("mqtt", "Servidor MQTT", cfg.mqttServer.c_str(), 40);
  WiFiManagerParameter p_prt("port", "Porta MQTT", String(cfg.mqttPort).c_str(), 6);
  WiFiManagerParameter p_usr("muser", "Usuario MQTT", cfg.mqttUser.c_str(), 40);
  WiFiManagerParameter p_pwd("mpass", "Senha MQTT", cfg.mqttPassword.c_str(), 40);

  wifiManager.addParameter(&p_srv);
  wifiManager.addParameter(&p_prt);
  wifiManager.addParameter(&p_usr);
  wifiManager.addParameter(&p_pwd);

  if (wifiManager.startConfigPortal("Monitor-UPS-Setup")) {

    cfg.wifiSSID     = WiFi.SSID();
    cfg.wifiPassword = WiFi.psk();
    cfg.mqttServer   = p_srv.getValue();
    cfg.mqttPort     = String(p_prt.getValue()).toInt();
    cfg.mqttUser     = p_usr.getValue();
    cfg.mqttPassword = p_pwd.getValue();

    if (cfg.mqttPort <= 0) cfg.mqttPort = 1883;

    salvarConfiguracoes();
    Serial.println("Configuracao via portal concluida!");

  } else {
    Serial.println("Timeout do portal. Reiniciando...");
    ESP.restart();
  }
}

// ==========================================
// SERVIDOR WEB
// ==========================================

void iniciarServidorWeb() {

  server.on("/", HTTP_GET, handleRoot);
  server.on("/dados", HTTP_GET, handleDados);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/config/save", HTTP_POST, handleConfigSave);
  server.on("/reset", HTTP_GET, handleReset);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Servidor Web iniciado na porta 80");
}

bool verificarAutenticacao() {

  if (strlen(CONFIG_PASSWORD) == 0) return true;

  if (!server.authenticate(CONFIG_USER, CONFIG_PASSWORD)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleDados() {

  bool energia;
  if (estadoEnergiaInicializado) energia = estadoEnergiaAnterior;
  else energia = (voltajeAC >= cfg.umbralEnergiaON);

  String json = "{";
  json += "\"ac\":" + String(voltajeAC, 2) + ",";
  json += "\"dc\":" + String(voltajeDC, 2) + ",";
  json += "\"energia\":" + String(energia ? "true" : "false") + ",";
  json += "\"uptime\":" + String(millis() / 1000) + ",";
  json += "\"wifi\":" + String(WiFi.RSSI()) + ",";
  json += "\"mqtt\":" + String(mqttClient.connected() ? "true" : "false") + ",";
  json += "\"ssid\":\"" + WiFi.SSID() + "\"";
  json += "}";

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "application/json", json);
}

void handleConfigPage() {

  if (!verificarAutenticacao()) return;

  String html = FPSTR(CONFIG_HTML);

  html.replace("%WIFI_SSID%", cfg.wifiSSID);
  html.replace("%MQTT_SERVER%", cfg.mqttServer);
  html.replace("%MQTT_PORT%", String(cfg.mqttPort));
  html.replace("%MQTT_USER%", cfg.mqttUser);
  html.replace("%FATOR_AC%", String(cfg.fatorCalibracaoAC, 3));
  html.replace("%UMBRAL_ON%", String(cfg.umbralEnergiaON, 1));
  html.replace("%UMBRAL_OFF%", String(cfg.umbralEnergiaOFF, 1));

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.send(200, "text/html", html);
}

void handleConfigSave() {

  if (!verificarAutenticacao()) return;

  if (server.hasArg("wifi_ssid")) {
    String novoSSID = server.arg("wifi_ssid");
    String novaSenha = server.arg("wifi_pass");

    if (novoSSID.length() > 0 && novoSSID != cfg.wifiSSID) {
      cfg.wifiSSID = novoSSID;
      if (novaSenha.length() > 0) cfg.wifiPassword = novaSenha;
    }
  }

  if (server.hasArg("mqtt_server")) cfg.mqttServer = server.arg("mqtt_server");
  if (server.hasArg("mqtt_port")) {
    int p = server.arg("mqtt_port").toInt();
    if (p > 0) cfg.mqttPort = p;
  }
  if (server.hasArg("mqtt_user")) cfg.mqttUser = server.arg("mqtt_user");
  if (server.hasArg("mqtt_pass") && server.arg("mqtt_pass").length() > 0)
    cfg.mqttPassword = server.arg("mqtt_pass");

  if (server.hasArg("fator_ac")) {
    float f = server.arg("fator_ac").toFloat();
    if (f > 0) cfg.fatorCalibracaoAC = f;
  }
  if (server.hasArg("umbral_on"))
    cfg.umbralEnergiaON = server.arg("umbral_on").toFloat();
  if (server.hasArg("umbral_off"))
    cfg.umbralEnergiaOFF = server.arg("umbral_off").toFloat();

  salvarConfiguracoes();

  server.send(200, "text/plain", "OK");
  Serial.println("Config salva. Reiniciando em 3s...");
  delay(3000);
  ESP.restart();
}

void handleReset() {

  if (!verificarAutenticacao()) return;

  bool full = server.hasArg("full");

  if (full) {
    resetarConfiguracoes();
    server.send(200, "text/plain", "Reset completo");
  } else {
    server.send(200, "text/plain", "Reiniciando...");
  }

  delay(1500);
  ESP.restart();
}

void handleNotFound() {
  server.send(404, "text/plain", "Rota nao encontrada");
}

// ==========================================
// MQTT DISCOVERY (melhorado)
// ==========================================

void publicarMQTTDiscovery() {

  if (!mqttClient.connected()) return;

  String deviceID = "monitor_energia_ups_";
  deviceID += String((uint32_t)ESP.getEfuseMac(), HEX);

  String availTopic = String(MQTT_TOPIC_AVAIL);

  String deviceBlock = "\"device\":{";
  deviceBlock += "\"identifiers\":[\"" + deviceID + "\"],";
  deviceBlock += "\"name\":\"Monitor Energia UPS\",";
  deviceBlock += "\"manufacturer\":\"ESP32 DIY\",";
  deviceBlock += "\"model\":\"Monitor de Energia UPS\",";
  deviceBlock += "\"sw_version\":\"1.0.0\",";
  deviceBlock += "\"configuration_url\":\"http://" + WiFi.localIP().toString() + "\"";
  deviceBlock += "}";

  // -------- BINARY SENSOR: Energia --------
  String d1 = "{";
  d1 += "\"name\":\"Energia da Rede\",";
  d1 += "\"unique_id\":\"" + deviceID + "_energia\",";
  d1 += "\"object_id\":\"monitor_ups_energia\",";
  d1 += "\"state_topic\":\"" + String(MQTT_TOPIC_ENERGIA) + "\",";
  d1 += "\"availability_topic\":\"" + availTopic + "\",";
  d1 += "\"payload_on\":\"ENERGIA OK\",";
  d1 += "\"payload_off\":\"ENERGIA OFF\",";
  d1 += "\"payload_available\":\"online\",";
  d1 += "\"payload_not_available\":\"offline\",";
  d1 += "\"device_class\":\"power\",";
  d1 += "\"icon\":\"mdi:power-plug\",";
  d1 += "\"expire_after\":30,";
  d1 += deviceBlock + ",";
  d1 += "\"platform\":\"binary_sensor\"";
  d1 += "}";
  mqttClient.publish(MQTT_DISCOVERY_ENERGIA, d1.c_str(), true);

  // -------- SENSOR: Tensao AC --------
  String d2 = "{";
  d2 += "\"name\":\"Tensao AC\",";
  d2 += "\"unique_id\":\"" + deviceID + "_voltaje_ac\",";
  d2 += "\"object_id\":\"monitor_ups_tensao_ac\",";
  d2 += "\"state_topic\":\"" + String(MQTT_TOPIC_AC) + "\",";
  d2 += "\"availability_topic\":\"" + availTopic + "\",";
  d2 += "\"unit_of_measurement\":\"V\",";
  d2 += "\"device_class\":\"voltage\",";
  d2 += "\"state_class\":\"measurement\",";
  d2 += "\"icon\":\"mdi:flash\",";
  d2 += "\"expire_after\":30,";
  d2 += "\"suggested_display_precision\":1,";
  d2 += deviceBlock + ",";
  d2 += "\"platform\":\"sensor\"";
  d2 += "}";
  mqttClient.publish(MQTT_DISCOVERY_AC, d2.c_str(), true);

  // -------- SENSOR: Tensao DC --------
  String d3 = "{";
  d3 += "\"name\":\"Tensao da Bateria\",";
  d3 += "\"unique_id\":\"" + deviceID + "_voltaje_dc\",";
  d3 += "\"object_id\":\"monitor_ups_tensao_dc\",";
  d3 += "\"state_topic\":\"" + String(MQTT_TOPIC_DC) + "\",";
  d3 += "\"availability_topic\":\"" + availTopic + "\",";
  d3 += "\"unit_of_measurement\":\"V\",";
  d3 += "\"device_class\":\"voltage\",";
  d3 += "\"state_class\":\"measurement\",";
  d3 += "\"icon\":\"mdi:battery\",";
  d3 += "\"expire_after\":30,";
  d3 += "\"suggested_display_precision\":2,";
  d3 += deviceBlock + ",";
  d3 += "\"platform\":\"sensor\"";
  d3 += "}";
  mqttClient.publish(MQTT_DISCOVERY_DC, d3.c_str(), true);

  Serial.println("MQTT Discovery publicado (3 entidades + availability)");
}

// ==========================================
// MQTT CONEXAO (com LWT)
// ==========================================

void conectarMQTT() {

  if (WiFi.status() != WL_CONNECTED) return;
  if (mqttClient.connected()) return;

  unsigned long agora = millis();
  if (agora - ultimoIntentoMQTT < intervaloMQTT) return;
  ultimoIntentoMQTT = agora;

  mqttClient.setServer(cfg.mqttServer.c_str(), cfg.mqttPort);
  mqttClient.setBufferSize(1536);

  String clientID = "ESP32_VOLTAGE_";
  clientID += String((uint32_t)ESP.getEfuseMac(), HEX);

  bool ok;

  if (cfg.mqttUser.length() > 0) {
    ok = mqttClient.connect(
      clientID.c_str(),
      cfg.mqttUser.c_str(),
      cfg.mqttPassword.c_str(),
      MQTT_TOPIC_AVAIL,   // LWT tópico
      1,                  // QoS
      true,               // retained
      "offline"           // payload LWT
    );
  } else {
    ok = mqttClient.connect(
      clientID.c_str(),
      nullptr, nullptr,
      MQTT_TOPIC_AVAIL,
      1, true, "offline"
    );
  }

  if (ok) {

    // Publica "online" assim que conectar
    mqttClient.publish(MQTT_TOPIC_AVAIL, "online", true);

    Serial.println("MQTT CONECTADO");
    publicarMQTTDiscovery();

    if (estadoEnergiaInicializado) {
      mqttClient.publish(MQTT_TOPIC_ENERGIA,
        estadoEnergiaAnterior ? "ENERGIA OK" : "ENERGIA OFF", true);

      char v[16];
      snprintf(v, sizeof(v), "%.1f", voltajeAC);
      mqttClient.publish(MQTT_TOPIC_AC, v, true);
      snprintf(v, sizeof(v), "%.2f", voltajeDC);
      mqttClient.publish(MQTT_TOPIC_DC, v, true);

      ultimoACPublicado = voltajeAC;
      ultimoDCPublicado = voltajeDC;
    }

  } else {
    Serial.print("Erro MQTT. Codigo: ");
    Serial.println(mqttClient.state());
  }
}

void revisarMQTT() {

  if (WiFi.status() != WL_CONNECTED) {
    if (mqttClient.connected()) mqttClient.disconnect();
    return;
  }

  if (!mqttClient.connected()) conectarMQTT();
  if (mqttClient.connected()) mqttClient.loop();
}

void publicarEstadoEnergia() {

  bool energiaAtual;

  if (estadoEnergiaInicializado) {
    if (estadoEnergiaAnterior)
      energiaAtual = (voltajeAC >= cfg.umbralEnergiaOFF);
    else
      energiaAtual = (voltajeAC >= cfg.umbralEnergiaON);
  } else {
    energiaAtual = (voltajeAC >= cfg.umbralEnergiaON);
  }

  if (!estadoEnergiaInicializado) {
    estadoEnergiaAnterior = energiaAtual;
    estadoEnergiaInicializado = true;

    if (mqttClient.connected()) {
      mqttClient.publish(MQTT_TOPIC_ENERGIA,
        energiaAtual ? "ENERGIA OK" : "ENERGIA OFF", true);
    }
    return;
  }

  if (energiaAtual != estadoEnergiaAnterior) {
    estadoEnergiaAnterior = energiaAtual;

    if (mqttClient.connected()) {
      mqttClient.publish(MQTT_TOPIC_ENERGIA,
        energiaAtual ? "ENERGIA OK" : "ENERGIA OFF", true);
      Serial.println(energiaAtual ? "MQTT -> ENERGIA OK" : "MQTT -> ENERGIA OFF");
    }
  }
}

void publicarVoltajesMQTT() {

  if (!mqttClient.connected()) return;

  if (fabs(voltajeAC - ultimoACPublicado) >= deltaAC) {
    char v[16];
    snprintf(v, sizeof(v), "%.1f", voltajeAC);
    mqttClient.publish(MQTT_TOPIC_AC, v, true);
    ultimoACPublicado = voltajeAC;
  }

  if (fabs(voltajeDC - ultimoDCPublicado) >= deltaDC) {
    char v[16];
    snprintf(v, sizeof(v), "%.2f", voltajeDC);
    mqttClient.publish(MQTT_TOPIC_DC, v, true);
    ultimoDCPublicado = voltajeDC;
  }
}

// ==========================================
// SENSORES
// ==========================================

void leerVoltajeDC() {
  voltajeDC = ina219.getBusVoltage_V();
}

void leerVoltajeAC() {

  const int muestras = 1500;
  float soma = 0.0, sq = 0.0;

  for (int i = 0; i < muestras; i++) soma += analogRead(ZMPT_PIN);

  float media = soma / muestras;

  for (int i = 0; i < muestras; i++) {
    float d = analogRead(ZMPT_PIN) - media;
    sq += d * d;
  }

  float rms = sqrt(sq / muestras);
  voltajeAC = rms * cfg.fatorCalibracaoAC;

  if (voltajeAC < 70.0) voltajeAC = 0.0;
}

// ==========================================
// DISPLAY
// ==========================================

void dibujarPantallaCompleta() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, 8);
  tft.print("VOLTAJES");

  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 45);
  tft.print("DC");

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(10, 78);
  tft.print("AC");
}

void actualizarValoresPantalla() {

  char b1[16], b2[16];
  snprintf(b1, sizeof(b1), "%5.2f", voltajeDC);
  snprintf(b2, sizeof(b2), "%5.1f", voltajeAC);

  tft.fillRect(50, 40, 180, 28, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(50, 40);
  tft.print(b1);
  tft.setTextSize(2);
  tft.print(" V");

  tft.fillRect(50, 73, 180, 28, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(3);
  tft.setCursor(50, 73);
  tft.print(b2);
  tft.setTextSize(2);
  tft.print(" V");

  tft.fillRect(0, 110, 240, 30, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setCursor(57, 115);

  bool ok = estadoEnergiaInicializado ?
    estadoEnergiaAnterior : (voltajeAC >= cfg.umbralEnergiaON);

  if (ok) {
    tft.setTextColor(ST77XX_GREEN);
    tft.print("ENERGIA: OK");
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.print("ENERGIA: OFF");
  }
}

void mostrarTelaConfiguracao() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_ORANGE);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
  tft.print("MODO CONFIG");
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 55);
  tft.print("1. Conecte no WiFi:");
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 70);
  tft.print("   Monitor-UPS-Setup");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 95);
  tft.print("2. Abra no navegador:");
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, 110);
  tft.print("   192.168.4.1");
}

// ==========================================
// BOTAO RESET (GPIO 0)
// ==========================================

void verificarBotaoReset() {

  static unsigned long inicioPress = 0;
  static bool pressionado = false;

  if (digitalRead(BOTAO_RESET) == LOW) {
    if (!pressionado) {
      pressionado = true;
      inicioPress = millis();
    } else if (millis() - inicioPress > 10000) {
      Serial.println("Reset por botao! Apagando config...");
      resetarConfiguracoes();
      delay(500);
      ESP.restart();
    }
  } else {
    pressionado = false;
  }
}

// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);
  delay(200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  pinMode(BOTAO_RESET, INPUT_PULLUP);

  pinMode(ZMPT_PIN, INPUT);
  analogSetPinAttenuation(ZMPT_PIN, ADC_11db);

  carregarConfiguracoes();

  Wire.begin(INA219_SDA, INA219_SCL);

  if (!ina219.begin()) {
    Serial.println("ERROR: INA219 no encontrado");
    tft.init(135, 240);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.setCursor(10, 50);
    tft.print("ERROR INA219");
    while (1) delay(1000);
  }

  Serial.println("INA219 OK");

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(135, 240);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  dibujarPantallaCompleta();

  iniciarWiFiManager();
  iniciarServidorWeb();

  Serial.println();
  Serial.println("================================");
  Serial.println("SISTEMA PRONTO");
  Serial.print("Acesse: http://");
  Serial.println(WiFi.localIP());
  Serial.println("================================");
}

// ==========================================
// LOOP
// ==========================================

void loop() {

  unsigned long agora = millis();

  verificarBotaoReset();
  revisarMQTT();
  server.handleClient();

  if (agora - ultimaLectura >= intervaloLectura) {
    ultimaLectura = agora;
    leerVoltajeDC();
    leerVoltajeAC();
  }

  if (agora - ultimaPantalla >= intervaloPantalla) {
    ultimaPantalla = agora;
    actualizarValoresPantalla();
  }

  if (agora - ultimaPublicacion >= intervaloPublicar) {
    ultimaPublicacion = agora;
    publicarEstadoEnergia();
    publicarVoltajesMQTT();
  }

  revisarMQTT();
}
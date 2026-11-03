
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// =====================================
// CONFIGURACAO DO WI-FI
// =====================================

const char* ssid = "iPhone de Isabella";
const char* password = "12345678";

WebServer server(80);

// =====================================
// PINOS DO ESP32
// =====================================

const int trigPin = 26;
const int echoPin = 25;
const int rele = 15;

#define SOUND_SPEED 0.034

// Rele ativo em LOW
const int RELE_LIGADO = LOW;
const int RELE_DESLIGADO = HIGH;

// =====================================
// VARIAVEIS
// =====================================

float distanciaCm = 0;
bool leituraValida = false;
bool lampadaLigada = false;

unsigned long ultimaLeitura = 0;
unsigned long ultimoLink = 0;
unsigned long ultimaReconexao = 0;

// =====================================
// PAGINA WEB - BOOTSTRAP
// =====================================

const char pagina[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">

<head>
  <meta charset="UTF-8">
  <meta name="viewport"
        content="width=device-width, initial-scale=1.0">

  <title>Monitoramento ESP32</title>

  <link
    href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css"
    rel="stylesheet">

  <style>
    body {
      background: #0f172a;
      color: white;
      font-family: Arial, sans-serif;
    }

    .painel {
      max-width: 800px;
      margin: 0 auto;
      padding: 60px 15px;
    }

    h1 {
      text-align: center;
      margin-bottom: 35px;
      font-weight: bold;
    }

    .cards {
      display: grid;
      grid-template-columns:
        repeat(auto-fit, minmax(230px, 1fr));
      gap: 20px;
    }

    .card {
      background: #1e293b;
      color: white;
      border: 1px solid #334155;
      border-radius: 15px;
      text-align: center;
    }

    .card-body {
      padding: 30px 15px;
    }

    .card-title {
      color: #cbd5e1;
      font-size: 18px;
    }

    .valor {
      color: #38bdf8;
      font-size: 32px;
      font-weight: bold;
      margin-top: 20px;
    }
  </style>
</head>

<body>

  <main class="container painel">

    <h1>Monitoramento ESP32</h1>

    <div class="cards">

      <!-- DISTANCIA ATUAL -->
      <div class="card shadow-sm">
        <div class="card-body">

          <h5 class="card-title">
            Distancia atual
          </h5>

          <p class="valor" id="distancia">
            --
          </p>

        </div>
      </div>

      <!-- ESTADO DA LAMPADA -->
      <div class="card shadow-sm">
        <div class="card-body">

          <h5 class="card-title">
            Estado da lampada
          </h5>

          <p class="valor" id="lampada">
            --
          </p>

        </div>
      </div>

    </div>

  </main>

  <script>

    async function atualizarDados() {

      try {

        const resposta = await fetch("/dados", {
          cache: "no-store"
        });

        if (!resposta.ok) {
          throw new Error("Erro de comunicacao");
        }

        const dados = await resposta.json();

        // Atualiza distancia
        document.getElementById("distancia").textContent =
          dados.valido
            ? Number(dados.distancia)
                .toFixed(1).replace(".", ",") + " cm"
            : "Sem leitura";

        // Atualiza lampada
        const lampada =
          document.getElementById("lampada");

        lampada.textContent =
          dados.ligada ? "Ligada" : "Desligada";

        lampada.style.color =
          dados.ligada ? "#22c55e" : "#ef4444";

      } catch (erro) {

        document.getElementById("distancia")
          .textContent = "--";

        document.getElementById("lampada")
          .textContent = "--";

      }
    }

    atualizarDados();

    // Atualiza a cada segundo
    setInterval(atualizarDados, 1000);

  </script>

</body>
</html>
)rawliteral";

// =====================================
// LEITURA DO SENSOR HC-SR04
// =====================================

void lerSensor() {

  if (millis() - ultimaLeitura < 500) {
    return;
  }

  ultimaLeitura = millis();

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duracao = pulseIn(echoPin, HIGH, 30000);

  if (duracao == 0) {

    leituraValida = false;
    lampadaLigada = false;

    digitalWrite(rele, RELE_DESLIGADO);
    return;
  }

  distanciaCm = duracao * SOUND_SPEED / 2;

  leituraValida = true;

  // Liga lampada a 10 cm ou menos
  if (distanciaCm <= 10) {

    digitalWrite(rele, RELE_LIGADO);
    lampadaLigada = true;

  } else {

    digitalWrite(rele, RELE_DESLIGADO);
    lampadaLigada = false;
  }
}

// =====================================
// ENVIA OS VALORES PARA O SITE
// =====================================

void enviarDados() {

  String json = "{";

  json += "\"distancia\":";
  json += String(distanciaCm, 1);

  json += ",\"valido\":";
  json += (leituraValida ? "true" : "false");

  json += ",\"ligada\":";
  json += (lampadaLigada ? "true" : "false");

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );
}

// =====================================
// MOSTRAR LINK NO SERIAL MONITOR
// =====================================

void mostrarLink() {

  Serial.println();
  Serial.println("==================================");

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WI-FI CONECTADO!");
    Serial.print("Rede: ");
    Serial.println(ssid);

    Serial.print("LINK DO SITE: http://");
    Serial.println(WiFi.localIP());

  } else {

    Serial.println("Hotspot do iPhone nao conectado.");
  }

  Serial.println();
  Serial.println("ACESSO ALTERNATIVO:");
  Serial.println("Rede: ESP32-Monitor");
  Serial.println("Senha: 12345678");

  Serial.print("LINK ALTERNATIVO: http://");
  Serial.println(WiFi.softAPIP());

  Serial.println("==================================");
  Serial.println();
}

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("INICIANDO ESP32...");

  // Configura sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  digitalWrite(rele, RELE_DESLIGADO);
  pinMode(rele, OUTPUT);

  // Ativa Wi-Fi e rede alternativa
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  // Rede alternativa sempre disponivel
  bool apCriado =
    WiFi.softAP("ESP32-Monitor", "12345678");

  if (!apCriado) {
    Serial.println("Falha ao criar rede alternativa!");
  }

  // Conecta ao iPhone
  Serial.println("Conectando ao iPhone de Isabella...");

  WiFi.begin(ssid, password);

  unsigned long inicio = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - inicio < 20000
  ) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Conexao Wi-Fi realizada!");
  } else {
    Serial.println("Nao foi possivel conectar ao iPhone.");
    Serial.println("Use a rede alternativa.");
  }

  // Pagina principal
  server.on("/", HTTP_GET, []() {

    server.send_P(
      200,
      "text/html; charset=utf-8",
      pagina
    );

  });

  // Dados do sensor
  server.on("/dados", HTTP_GET, enviarDados);

  // Inicia servidor
  server.begin();

  Serial.println("Servidor WEB iniciado!");

  // Mostra os links
  mostrarLink();

  ultimoLink = millis();
  ultimaReconexao = millis();
}

// =====================================
// LOOP PRINCIPAL
// =====================================

void loop() {

  lerSensor();

  server.handleClient();

  // Mostra os links a cada 5 segundos
  if (millis() - ultimoLink >= 5000) {

    ultimoLink = millis();

    mostrarLink();
  }

  // Reconecta automaticamente se falhar
  if (WiFi.status() != WL_CONNECTED &&
      millis() - ultimaReconexao >= 15000) {

    ultimaReconexao = millis();

    Serial.println("Tentando reconectar ao iPhone...");

    WiFi.reconnect();
  }

  delay(10);
}

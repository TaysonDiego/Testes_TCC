#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>


MAX30105 Sensor;
Adafruit_MPU6050 SensorDeQueda;

// =========================
// LEITURA SENSOR
// =========================
long RED;
long IR;

// =========================
// BPM
// =========================
float BatimentosPorMinuto = 0;
int MediaBatimento = 0;
long UltimoBatimento = 0;
long Delta = 0;

const byte Frequencia_Tamanho = 4;
byte Frequencia[Frequencia_Tamanho];
byte FrequenciaSpot = 0;

// =========================
// SPO2
// =========================
const int BufferSize = 50;

uint32_t BufferIR[BufferSize];
uint32_t BufferRed[BufferSize];

int32_t Spo2;
int8_t Spo2Valido;

int32_t BPM;
int8_t BPMvalido;

int BufferIndex = 0;

// =========================
// SENSOR DE QUEDA
// =========================
sensors_event_t aceleracao, giroscopio, temperaturadosensor;

float AccMagnitude;
float WccMagnitude;

float movimento = 0;

bool PossivelQueda = false;
unsigned long tempoQueda = 0;

// =========================
// CONTROLE SERIAL
// =========================
unsigned long ultimoPrint = 0;

// =========================
// WIFI/API
// =========================
char url[] = "https://api-monitoramento-pvcc.onrender.com/dados";

char NomeRede[] = "Thaysla casa";
char Senha[] = "Taysoncasa.#";

int contadorWifi = 0;

unsigned long ultimoEnvio = 0;


void setup() {

  Serial.begin(115200);
  delay(1000);

  Wire.begin(4, 5);

  Serial.println("Iniciando sensores...");

  // =========================
  // MAX30102
  // =========================
  if (!Sensor.begin(Wire, I2C_SPEED_STANDARD)) {

    Serial.println("MAX30102 nao encontrado");

    while (1);
  }

  Serial.println("MAX30102 conectado");

  byte brilhoLED = 60;
  byte media = 4;
  byte modoLED = 2;
  int taxaAmostra = 200;
  int larguraPulso = 411;
  int faixaADC = 4096;

  Sensor.setup(
    brilhoLED,
    media,
    modoLED,
    taxaAmostra,
    larguraPulso,
    faixaADC
  );

  // =========================
  // BUFFER INICIAL SPO2
  // =========================
  Serial.println("Coletando amostras iniciais...");

  for (int i = 0; i < BufferSize; i++) {

    while (!Sensor.available()) {

      Sensor.check();
    }

    BufferRed[i] = Sensor.getRed();
    BufferIR[i] = Sensor.getIR();

    Sensor.nextSample();
  }

  // =========================
  // MPU6050
  // =========================
  if (!SensorDeQueda.begin(0x68, &Wire)) {

    Serial.println("MPU6050 nao encontrado");

    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 conectado");

  SensorDeQueda.setAccelerometerRange(MPU6050_RANGE_8_G);

  SensorDeQueda.setGyroRange(MPU6050_RANGE_500_DEG);

  SensorDeQueda.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("Sistema iniciado");

  // =========================
// WIFI
// =========================
WiFi.begin(NomeRede, Senha);

Serial.println("Conectando WiFi...");

while (WiFi.status() != WL_CONNECTED &&
       contadorWifi < 10) {

  delay(500);

  Serial.print(".");

  contadorWifi++;
}

if (WiFi.status() == WL_CONNECTED) {

  Serial.println("\nWiFi conectado!");

} else {

  Serial.println("\nFalha WiFi");
}

configTime(
  -3 * 3600,
  0,
  "pool.ntp.org",
  "time.nist.gov"
);
}
//--------------
// FUNCAO DATA
//-------------
String dataAtual() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {

    return "sem_data";
  }

  char buffer[25];

  strftime(
    buffer,
    sizeof(buffer),
    "%d-%m-%Y %H:%M:%S",
    &timeinfo
  );

  return String(buffer);
}

void loop() {

  LerMAX30102();

  LerMPU6050();

  DetectarQueda();

  MostrarDados();

  EnviarAPI();
}

// =====================================================
// LEITURA MAX30102
// =====================================================
void LerMAX30102() {

  Sensor.check();

  if (Sensor.available()) {

    IR = Sensor.getIR();
    RED = Sensor.getRed();

    Sensor.nextSample();

    // =========================
    // DEDO DETECTADO
    // =========================
    if (IR > 50000) {

      DetectarBPM();

      AtualizarSPO2();
    }

    // =========================
    // CONTATO FRACO
    // =========================
    else if (IR >= 10000 && IR < 50000) {
      
      BatimentosPorMinuto = 0;
      MediaBatimento = 0;
    }

    // =========================
    // SEM CONTATO
    // =========================
    else {

      BatimentosPorMinuto = 0;
      MediaBatimento = 0;
    }
  }
}

// =====================================================
// BPM
// =====================================================
void DetectarBPM() {

  if (checkForBeat(IR)) {

    Delta = millis() - UltimoBatimento;

    UltimoBatimento = millis();

    BatimentosPorMinuto =
      60.0 / (Delta / 1000.0);

    if (BatimentosPorMinuto > 20 &&
        BatimentosPorMinuto < 255) {

      Frequencia[FrequenciaSpot++] =
        (byte)BatimentosPorMinuto;

      FrequenciaSpot %= Frequencia_Tamanho;

      MediaBatimento = 0;

      for (byte x = 0;
           x < Frequencia_Tamanho;
           x++) {

        MediaBatimento += Frequencia[x];
      }

      MediaBatimento /= Frequencia_Tamanho;
    }
  }
}

// =====================================================
// SPO2
// =====================================================
void AtualizarSPO2() {

  BufferRed[BufferIndex] = RED;
  BufferIR[BufferIndex] = IR;

  BufferIndex++;

  if (BufferIndex >= BufferSize) {

    maxim_heart_rate_and_oxygen_saturation(
      BufferIR,
      BufferSize,
      BufferRed,
      &Spo2,
      &Spo2Valido,
      &BPM,
      &BPMvalido
    );

    BufferIndex = 0;
  }
}

// =====================================================
// MPU6050
// =====================================================
void LerMPU6050() {

  SensorDeQueda.getEvent(
    &aceleracao,
    &giroscopio,
    &temperaturadosensor
  );

  AccMagnitude = sqrt(

    aceleracao.acceleration.x *
    aceleracao.acceleration.x +

    aceleracao.acceleration.y *
    aceleracao.acceleration.y +

    aceleracao.acceleration.z *
    aceleracao.acceleration.z
  );

  WccMagnitude = sqrt(

    giroscopio.gyro.x *
    giroscopio.gyro.x +

    giroscopio.gyro.y *
    giroscopio.gyro.y +

    giroscopio.gyro.z *
    giroscopio.gyro.z
  );

  movimento = abs(AccMagnitude - 9.81);
}

// =====================================================
// DETECCAO DE QUEDA
// =====================================================
void DetectarQueda() {

  if (movimento >= 10 &&
      WccMagnitude > 2.0) {

    PossivelQueda = true;

    tempoQueda = millis();
  }

  if (PossivelQueda) {

    if (millis() - tempoQueda > 3000) {

      if (movimento < 1.5 &&
          WccMagnitude < 0.2) {

        Serial.println("\nQUEDA DETECTADA\n");
        delay(10000);
      }

      PossivelQueda = false;
    }
  }
}

// =====================================================
// MOSTRAR DADOS
// =====================================================
void MostrarDados() {

  if (millis() - ultimoPrint > 500) {

    ultimoPrint = millis();

    Serial.println("===================================");
    // =========================
    // DEDO DETECTADO
    // =========================
    if (IR > 50000) {
      Serial.println("Pele encostada\n");
    }
    // =========================
    // CONTATO FRACO
    // =========================
    else if (IR >= 10000 && IR < 50000) {
      Serial.println("Contato fraco com a pele\n");
    }
    // =========================
    // SEM CONTATO
    // =========================
    else {
      Serial.println("Sem fontato com a pele\n");
    }

    // =========================
    // CARDIACO
    // =========================
    Serial.println("CARDIACO");

    Serial.print("BPM Atual: ");
    Serial.println(BatimentosPorMinuto);

    Serial.print("Media BPM: ");
    Serial.println(MediaBatimento);

    Serial.print("SPO2: ");

    if (Spo2Valido) {

      Serial.print(Spo2);
      Serial.println("%");

    } else {

      Serial.println("Invalido");
    }

    // =========================
    // ACELERACAO
    // =========================
    Serial.println("\nACELERACAO");

    Serial.print("X: ");
    Serial.println(aceleracao.acceleration.x);

    Serial.print("Y: ");
    Serial.println(aceleracao.acceleration.y);

    Serial.print("Z: ");
    Serial.println(aceleracao.acceleration.z);

    // =========================
    // GIROSCOPIO
    // =========================
    Serial.println("\nGIROSCOPIO");

    Serial.print("X: ");
    Serial.println(giroscopio.gyro.x);

    Serial.print("Y: ");
    Serial.println(giroscopio.gyro.y);

    Serial.print("Z: ");
    Serial.println(giroscopio.gyro.z);

    // =========================
    // ESTADO
    // =========================
    Serial.println("\nESTADO");

    Serial.print("Movimento: ");
    Serial.println(movimento);

    Serial.print("Vel Angular: ");
    Serial.println(WccMagnitude);

    if (movimento < 1.5 &&
        WccMagnitude < 0.2) {

      Serial.println("Status: PARADO");

    } else if (movimento < 4) {

      Serial.println("Status: MOVIMENTO LEVE");

    } else if (movimento < 10) {

      Serial.println("Status: ANDANDO");
    }

    if (PossivelQueda) {

      Serial.println("ALERTA: POSSIVEL QUEDA");
    }

    Serial.println("===================================\n");
  }
}
//================================
// FUNCAO API
//=================================
void EnviarAPI() {

  // ENVIA A CADA 5 SEGUNDOS
  if (millis() - ultimoEnvio < 5000) {

    return;
  }

  ultimoEnvio = millis();

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    http.begin(url);

    http.addHeader(
      "Content-Type",
      "application/json"
    );

    http.addHeader(
      "Authorization",
      "X%BNY&9@1s8!A"
    );

    // =========================
    // DEFINE MOVIMENTO
    // =========================
    int nivelMovimento = 0;

    if (movimento < 1.5) {

      nivelMovimento = 0;

    } else if (movimento < 4) {

      nivelMovimento = 1;

    } else {

      nivelMovimento = 2;
    }

    // =========================
    // JSON
    // =========================
    String json = "{";

    json += "\"Batimentos\":"
            + String(MediaBatimento) + ",";

    json += "\"HRV\":0,";

    json += "\"Temperatura\":0,";

    json += "\"Oxigenacao\":"
            + String(Spo2) + ",";

    json += "\"Acelerometro\":"
            + String(AccMagnitude) + ",";

    json += "\"Giroscopio\":"
            + String(WccMagnitude) + ",";

    json += "\"Movimento\":"
            + String(movimento > 1.5) + ",";

    json += "\"NivelMovimento\":"
            + String(nivelMovimento) + ",";

    json += "\"Queda\":"
            + String(PossivelQueda) + ",";

    json += "\"ChanceDeQueda\":"
            + String(PossivelQueda ? 80 : 0) + ",";

    json += "\"Bateria\":70,";

    json += "\"Carregando\":false,";

    json += "\"Tensao\":3.7,";

    json += "\"Data\":\""
            + dataAtual() + "\",";

    json += "\"WiFiRSSI\":"
            + String(WiFi.RSSI());

    json += "}";

    // =========================
    // ENVIA
    // =========================
    int codigo = http.POST(json);

    Serial.println("\n========== API ==========");

    Serial.print("HTTP: ");
    Serial.println(codigo);

    Serial.println("JSON enviado:");

    Serial.println(json);

    Serial.println("=========================\n");

    http.end();

  } else {

    Serial.println("WiFi desconectado");
  }
}


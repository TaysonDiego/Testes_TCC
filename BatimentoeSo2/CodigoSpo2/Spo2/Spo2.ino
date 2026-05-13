#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

MAX30105 Sensor;

uint32_t BufferIR[100];
uint32_t BufferRed[100];

int32_t Spo2;
int8_t Spo2Valido;
int32_t BPM;
int8_t BPMvalido;

void setup() {
  Serial.begin(115200);

  Wire.begin(4, 5);

  if (!Sensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Sensor nao encontrado");
    while (1);
  }

  Sensor.setup();

  for (int i = 0; i < 100; i++) {

    while (!Sensor.available()) {
        Sensor.check();
    }

    BufferRed[i] = Sensor.getRed();
    BufferIR[i] = Sensor.getIR();

    Sensor.nextSample();
  }
}

void loop() {

  // MOVE 75 AMOSTRAS ANTIGAS
  for (int i = 25; i < 100; i++) {

      BufferRed[i - 25] = BufferRed[i];
      BufferIR[i - 25] = BufferIR[i];
  }

  // ADICIONA 25 NOVAS
  for (int i = 75; i < 100; i++) {

      while (!Sensor.available()) {
          Sensor.check();
      }

      BufferRed[i] = Sensor.getRed();
      BufferIR[i] = Sensor.getIR();

      Sensor.nextSample();
  }
  
  // CALCULAR BPM + SPO2
  maxim_heart_rate_and_oxygen_saturation(
    BufferIR,
    100,
    BufferRed,
    &Spo2,
    &Spo2Valido,
    &BPM,
    &BPMvalido
  );


  if (Spo2Valido) {
    Serial.print("SPO2: ");
    Serial.print(Spo2);
    Serial.print("%");
  }
  else {
      Serial.print("SPO2 invalido");
  }

  Serial.print(" | ");

  Serial.println("");

  delay(100);
}
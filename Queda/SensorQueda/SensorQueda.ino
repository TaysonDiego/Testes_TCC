#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 SensorDeQueda;


sensors_event_t aceleracao, giroscopio, temperaturadosensor;
float AccMagnitude; // Aceleracao Resultante
float WccMagnitude; // Velocidade Angular Resultante
bool PossivelQueda = false;
float movimento = 0;
float tempoQueda;

void setup() {

  Serial.begin(115200);
  delay(1000);
  Wire.begin(4,5);
  delay(1000);


  if(SensorDeQueda.begin(0x68, &Wire) == false){
    Serial.println("Sensor de queda não encontrado");
    while(1){
      delay(10);
    }
  }else{
    Serial.println("Sensor de queda conectado");
  }
  SensorDeQueda.setAccelerometerRange(MPU6050_RANGE_8_G);

  SensorDeQueda.setGyroRange(MPU6050_RANGE_500_DEG);

  SensorDeQueda.setFilterBandwidth(MPU6050_BAND_21_HZ);


 
}

void loop() {
  SensorDeQueda.getEvent(&aceleracao, &giroscopio, &temperaturadosensor);

  AccMagnitude = sqrt( 
  aceleracao.acceleration.x * aceleracao.acceleration.x +
  aceleracao.acceleration.y * aceleracao.acceleration.y +
  aceleracao.acceleration.z * aceleracao.acceleration.z
  );

  WccMagnitude = sqrt(
   giroscopio.gyro.x * giroscopio.gyro.x +
   giroscopio.gyro.y * giroscopio.gyro.y +
   giroscopio.gyro.z * giroscopio.gyro.z 
  );

  movimento = abs(AccMagnitude - 9.81);


  Serial.print("Aceleracao ");
  Serial.print("X:  ");
  Serial.print(aceleracao.acceleration.x);
  Serial.print(" | ");

  Serial.print("Y: ");
  Serial.print(aceleracao.acceleration.y);
  Serial.print(" | ");

  Serial.print("Z: ");
  Serial.print(aceleracao.acceleration.z);

  Serial.println("");

  Serial.print("Giroscopio ");
  Serial.print("X: ");
  Serial.print(giroscopio.gyro.x);
  Serial.print(" | ");

  Serial.print("Y:  ");
  Serial.print(giroscopio.gyro.y);
  Serial.print(" | ");

  Serial.print("Z: ");
  Serial.print(giroscopio.gyro.z);

  if (movimento < 1.5 &&
      WccMagnitude < 0.2) {

      Serial.println("\nParado\n");

  }
  else if (movimento < 4) {

      Serial.println("\nMovimento leve\n");

  }
  else if (movimento < 10) {

      Serial.println("\nAndando / Movimento forte\n");
  }

  if (movimento >= 10 &&
    WccMagnitude > 2.0) {

    PossivelQueda = true;
    tempoQueda = millis();
  }
  if (PossivelQueda) {

    if (millis() - tempoQueda > 3000) {

        if (movimento < 1.5 &&
            WccMagnitude < 0.2) {

            Serial.println("QUEDA DETECTADA");
        }

        PossivelQueda = false;  
        delay(10000);
    }
}

  Serial.println("");
  delay(50);
}
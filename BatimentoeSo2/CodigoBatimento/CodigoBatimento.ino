#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include "spo2_algorithm.h"


MAX30105 Sensor;

long RED;
long IR;

float Batimento = 0;
float BatimentosPorMinuto = 0;
int MediaBatimento = 0;
long UltimoBatimento = 0;

long Delta = 0;

const byte Frequencia_Tamanho = 4;
byte Frequencia[Frequencia_Tamanho];
byte FrequenciaSpot = 0;


void setup() {
  
  Serial.begin(115200);
  delay(1000);
  Wire.begin(4,5);

  Serial.println("Iniciando Sensor MAX");
  delay(2000);
  if(Sensor.begin() == false){
    Serial.println("Sensor MAX nao encontrado");
    while(1);
  }else{
    Serial.println("Sensor encontrado");
  }
  Sensor.setup();
}

void loop() {

  IR = Sensor.getIR();
  RED = Sensor.getRed();

  //Batimentos
  //Pele em contato com o 
  
  if( IR > 50000){
     Serial.println("Dedo encostado");
    if(checkForBeat(IR)){  
      Delta = millis() - UltimoBatimento;
      UltimoBatimento = millis();

      BatimentosPorMinuto = 60.0 / (Delta / 1000.0);

      if (BatimentosPorMinuto < 255 and BatimentosPorMinuto > 20){
        Frequencia[FrequenciaSpot++] = (byte)BatimentosPorMinuto;
        FrequenciaSpot %= Frequencia_Tamanho;

        MediaBatimento = 0;
        for(byte x = 0; x < Frequencia_Tamanho; x++){
          MediaBatimento += Frequencia[x]; 
        }
        MediaBatimento /= Frequencia_Tamanho; 
      }
    }
    
  }
  Serial.print("Batimentos: ");
  Serial.print(BatimentosPorMinuto);
  Serial.print(" | ");
  Serial.print("Media do Batimento: ");
  Serial.print(MediaBatimento);
  Serial.print(" | ");
  Serial.print("IR: ");
  Serial.print(IR);
  Serial.println("");
  
  if (IR >= 10000 and IR < 50000) {
    Serial.println("Contato fraco");
    BatimentosPorMinuto = 0;
    MediaBatimento = 0;
  }else if (IR >= 0 and IR < 10000 ){
    Serial.println("Sem contato!");
    BatimentosPorMinuto = 0;
    MediaBatimento = 0;
  }



}
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>


char url[] = "https://api-monitoramento-pvcc.onrender.com/dados";
char NomeRede[] = "Thaysla casa";
char Senha[] = "Taysoncasa.#";
int contadorWifi = 0;

float batimentos = 95;
int HRV = 43;
float temperatura = 36.5;
int oxigenacao = 95;
int aceleromentro;
int giroscopio;
bool movimento = true;
int nivelMovimento = 1; /*0 1 2 3*/
bool queda = false;
int chanceDeQueda = 0;
int bateria = 70;
bool carregando = false;
int tensao = 3.7;

String data(){
 struct tm timeinfo;
   if (!getLocalTime(&timeinfo)) {
    return "sem_data";
  }

  char buffer[25];
  strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);

  return String(buffer);
}



void setup() {
  Serial.begin(115200);
  WiFi.begin(NomeRede, Senha);
  Serial.println("Conectando Wi-fi");
  while(WiFi.status() != WL_CONNECTED and contadorWifi < 10){
    delay(500);
    Serial.print(".");
    WiFi.disconnect();
    WiFi.begin(NomeRede, Senha);
    contadorWifi++;
  }
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("");
    Serial.println("Wi-Fi Conectado!");
    contadorWifi = 0;
  }else{
    Serial.println("Wi-Fi não contectado!");
    contadorWifi = 0;
  }
  configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
}

void loop() {
if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    http.begin(url);

    http.addHeader("Content-Type", "application/json");

    String json = "{";
      json += "\"Batimentos\":"     + String(batimentos)     + ",";
      json += "\"HRV\":"            + String(HRV)            + ",";
      json += "\"Temperatura\":"    + String(temperatura)    + ",";
      json += "\"Oxigenacao\":"     + String(oxigenacao)     + ",";
      json += "\"Acelerometro\":"   + String(aceleromentro)  + ",";
      json += "\"Giroscopio\":"     + String(giroscopio)     + ",";
      json += "\"Movimento\":"      + String(movimento)      + ",";
      json += "\"NivelMovimento\":" + String(nivelMovimento) + ","; 
      json += "\"Queda\":"          + String(queda)          + ",";
      json += "\"ChanceDeQueda\":"  + String(chanceDeQueda)  + ",";
      json += "\"Bateria\":"        + String(bateria)        + ",";
      json += "\"Carregando\":"     + String(carregando)     + ",";
      json += "\"Tensao\":"         + String(tensao)         + ",";
      json += "\"Data\":\""         + data()                 + "\",";
      json += "\"WiFiRSSI\":"       + String(WiFi.RSSI());
      json += "}";

    http.addHeader("Authorization", "X%BNY&9@1s8!A");
    int codigo = http.POST(json);

    Serial.print("Codigo HTTP: ");
    Serial.println(codigo);

    String resposta = http.getString();

    Serial.println(resposta);

    http.end();

    batimentos += random(-3, 4);   
    temperatura += random(-2, 3) * 0.1;
    oxigenacao += random(-1, 2);

    if (batimentos < 60) batimentos = 60;
    if (batimentos > 120) batimentos = 120;

    if (temperatura < 35.5) temperatura = 35.5;
    if (temperatura > 38.5) temperatura = 38.5;

    if (oxigenacao < 90) oxigenacao = 90;
    if (oxigenacao > 100) oxigenacao = 100;


  }else{
    Serial.println("Wi-FI desconectado");
    contadorWifi = 0;
  }
  delay(1000);
}

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
Adafruit_BMP085 bmp;

DHTesp dht;
int dhtPin = 16;
ComfortState cs;

#define sensorUmidade A0 // Sensor de umidade de solo do módulo
#define pinRele D3
int leitura;
bool irrigacaoAtiva = false;
bool habilitaSistema = false;

uint32_t timer = 0;

const char*         ssid = "xxxxxx"; //Aqui o nome da sua rede local wi fi
const char*     password = "xxxxxx"; // Aqui a senha da sua rede local wi fi
const char*   mqttServer = "m15.cloudmqtt.com"; // Aqui o endereço do seu servidor fornecido pelo site 
const int       mqttPort = 15950; // Aqui mude para sua porta fornecida pelo site
const char*     mqttUser = "bspouqyb"; //  Aqui o nome de usuario fornecido pelo site
const char* mqttPassword = "xSqOx3aIN1T_"; //  Aqui sua senha fornecida pelo site

WiFiClient espClient;
PubSubClient client(espClient);

void mqtt_callback(char* topic, byte* payload, unsigned int length);

void setup(){
  Serial.begin(115200);
//  configuração do sensor DHT-11
  dht.setup(dhtPin, DHTesp::DHT11);
  
  pinMode(pinRele, OUTPUT);
  digitalWrite(pinRele, HIGH);

//  configuração do sensor BMP180  
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }
  
// Configuração wi-fi
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

//  configuração MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqtt_callback);
 
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP8266Client", mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  client.subscribe("projeto/habilita");
  client.subscribe("projeto/bomba_dagua");
}


void mqtt_callback(char* topic, byte* payload, unsigned int length) {
//  Serial.print("Message arrived in topic: ");
//  Serial.println(topic);
 
//  Serial.print("Message:");
//  for (int i = 0; i < length; i++) {
//    Serial.print((char)payload[i]);
//    String msg;
//    
//    //obtem a string do payload recebido
//    for(int i = 0; i < length; i++) {
//       char c = (char)payload[i];
//       msg += c;
//    }

//    if (strcmp(topic,"projeto/habilita") == 0) {
//      Serial.print("valor para projeto/habilita: ");
//      Serial.println(msg);
//      habilitaSistema = true;
//      client.publish("projeto/habilita", "1");
//      
//    } else if(strcmp(topic, "projeto/bomba_dagua") == 0) {
//      Serial.print("valor para projeto/bomba_dagua: ");
//      Serial.println(msg);
//    }
//  }

//  Serial.println();
//  Serial.println("-----------------------");
}

void loop() {

  if((millis() - timer) >= 3000) {
    
    showInfo();

    avaliarUmidadeDoSolo();      
    
//    if (habilitaSistema) {
//      avaliarUmidadeDoSolo();      
//    } else {
//      client.publish("projeto/habilita", "0");
//    }
     
    timer = millis(); // Atualiza a referência
  }

  client.loop();
}


void avaliarUmidadeDoSolo() {
  leitura = analogRead(sensorUmidade);
  char* statusBomba;
//  showSoilHumidityTest();
    
  if (leitura >= 900) {
    if (!irrigacaoAtiva) {
      digitalWrite(pinRele, !digitalRead(pinRele));
      irrigacaoAtiva = true;
      statusBomba = "0";
      Serial.println("O SOLO ULTRAPASSOU SEU NÍVEL DE UMIDADE MÍNIMO");
      Serial.println("ATIVANDO SISTEMA DE IRRIGAÇÃO");
    }
  } else {
    if (leitura <= 515) {
      if (irrigacaoAtiva) {
        digitalWrite(pinRele, !digitalRead(pinRele));
        irrigacaoAtiva = false;
        statusBomba = "1";
        Serial.println("O SOLO ATINGIU SEU NÍVEL DE UMIDADE ADEQUADO");
        Serial.println("DESATIVANDO SISTEMA DE IRRIGAÇÃO");
      }
    }
  }
  client.publish("projeto/bomba_dagua", statusBomba);
}

void showTempAndHum() {
  float humidity = (float) dht.getHumidity();
  float temperature = (float) dht.getTemperature();
  static char outTemp[5];
  static char outHum[5];

  Serial.print("Status do Sensor: ");
  Serial.println(dht.getStatusString());
  Serial.print("Umidade relativa do ar (%): ");
  Serial.println(humidity, 1);
  Serial.print("Temperatura (*C): ");
  Serial.println(temperature, 1);
  Serial.print("Sensação térmica (*C): ");
  Serial.println(dht.computeHeatIndex(temperature, humidity, false), 1);

  dtostrf(temperature, 5, 2, outTemp);
  dtostrf(humidity, 5, 2, outHum);
  
  client.publish("projeto/temperatura", outTemp);
  client.publish("projeto/umidade_ar", outHum);
  
  float cr = dht.getComfortRatio(cs, temperature, humidity, false);
  String comfortStatus;
  switch(cs) {
    case Comfort_OK:
      comfortStatus = "OK";
      break;
    case Comfort_TooHot:
      comfortStatus = "Muito calor";
      break;
    case Comfort_TooCold:
      comfortStatus = "Muito frio";
      break;
    case Comfort_TooDry:
      comfortStatus = "Muito seco";
      break;
    case Comfort_TooHumid:
      comfortStatus = "Muito úmido";
      break;
    case Comfort_HotAndHumid:
      comfortStatus = "Quente e úmido";
      break;
    case Comfort_HotAndDry:
      comfortStatus = "Quente e seco";
      break;
    case Comfort_ColdAndHumid:
      comfortStatus = "Frio e úmido";
      break;
    case Comfort_ColdAndDry:
      comfortStatus = "Frio e seco";
      break;
    default:
      comfortStatus = "Desconhecido";
      break;
  };  
  Serial.print("Conforto térmico: ");
  Serial.println(comfortStatus);
  
  Serial.print("Percepção climática: ");
  byte hp = dht.computePerception(temperature, humidity, false);
  String humanPerception;
  switch(hp) {
    case Perception_Dry:
      humanPerception = "Clima seco";
      break;
    case Perception_VeryComfy:
      humanPerception = "Bastante agradável";
      break;
    case Perception_Comfy:
      humanPerception = "Agradável";
      break;
    case Perception_Ok:
      humanPerception = "Ok";
      break;
    case Perception_UnComfy:
      humanPerception = "Desconfortável";
      break;
    case Perception_QuiteUnComfy:
      humanPerception = "Bastante desconfortável";
      break;
    case Perception_VeryUnComfy:
      humanPerception = "Muito desconfortável";
      break;
    case Perception_SevereUncomfy:
      humanPerception = "Extremamente desconfortável";
      break;
    default:
      humanPerception = "Desconhecido";
      break;
  };
  Serial.println(humanPerception);
}

void showSoilHumidity() {
  int leitura = analogRead(sensorUmidade); // Leitura dos dados analógicos vindos do sensor de umidade de solo
  char* statusUmidadeSolo;
 
  if (leitura <= 1023 && leitura >= 682) { // Se a leitura feita for um valor entre 1023 e 682 podemos definir que o solo está com uma baixa condutividade, logo a planta deve ser regada
    statusUmidadeSolo = "NÍVEL DE UMIDADE BAIXO";
  } else {
    if (leitura <= 681 && leitura >= 516) { // Se a leitura feita for um valor entre 681 e 341 podemos definir que o solo está com um nível médio de umidade, logo dependendo da planta pode ou não ser vantajoso regar
    statusUmidadeSolo = "NÍVEL DE UMIDADE INSUFICIENTE";
    }
    else {
      if (leitura <= 515 && leitura >= 0) { // Se a leitura feita for um valor entre 0 e 340 podemos definir que o solo está com um nível aceitável de umidade, logo talvez não seja interessante regar neste momento
        statusUmidadeSolo = "NÍVEL DE UMIDADE ADEQUADO";
      }
    } 
  }
  Serial.print(statusUmidadeSolo);
  Serial.print(": ");
  Serial.println(leitura);
  client.publish("projeto/umidade_solo", statusUmidadeSolo);  
}

void showSoilHumidityTest() {
  Serial.println(analogRead(sensorUmidade));
}

void showBMP180Info() {
  Serial.print("TEMPERATURA = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");
  
  Serial.print("PRESSÃO = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");
  
  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  Serial.print("ALTITUDE = ");
  Serial.print(bmp.readAltitude());
  Serial.println(" metros");

  Serial.print("PRESSÃO AO NÍVEL DO MAR (CALCULADO) = ");
  Serial.print(bmp.readSealevelPressure());
  Serial.println(" Pa");

  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
  Serial.print("ALTITUDE REAL = ");
  Serial.print(bmp.readAltitude(101500));
  Serial.println(" metros");
  
  Serial.println();

  String temp = String(bmp.readPressure());
  char attributes[10];
  temp.toCharArray(attributes, 10);
  client.publish("projeto/pressao_atm", attributes);
}

void clearAndHome() {
  Serial.write(27); // ESC
  Serial.write("[2J"); // clear screen
  Serial.write(27); // ESC
  Serial.write("[H"); // cursor to home
}

void showInfo() {
  clearAndHome();

  Serial.println("----------------------------------");
  Serial.println("           DHT11 - INFO           ");
  Serial.println("----------------------------------");
  showTempAndHum();
  Serial.println("----------------------------------");
  Serial.println();
  Serial.println();
  Serial.println();

  Serial.println("----------------------------------");
  Serial.println("    SENSOR DE UMIDADE DO SOLO     ");
  Serial.println("----------------------------------");
  showSoilHumidity();
  Serial.println("----------------------------------");
  Serial.println();
  Serial.println();
  Serial.println();

  Serial.println("----------------------------------");
  Serial.println("          BMP180 - INFO           ");
  Serial.println("----------------------------------");
  showBMP180Info();
  Serial.println("----------------------------------");
  Serial.println();
  Serial.println();
  Serial.println();
}


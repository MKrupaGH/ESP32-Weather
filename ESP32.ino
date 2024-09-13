//Libraries
#include <SoftwareSerial.h>
#include "MQ131.h"
#include "Air_Quality_Sensor.h"
#include "PMS.h"
#include <Adafruit_BMP280.h>
#include <BH1750.h>

PMS pms(Serial);
PMS::DATA data;
//Constants
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define tvocPin 7

Adafruit_BMP280 bmp280;
BH1750 lightMeter;

SoftwareSerial pmsSerial(8, 9);

float humidity;
float temperature;
float pressure;
float cloud;
int o3;
int tvoc;
unsigned long dataTimer3 = 0;
int pm1_0;
int pm2_5;
int pm10_0;

AirQualitySensor sensor(A1);

// Connection

const char *ssid = "ha";
const char *password = "12345678";

void setup()
{
  pinMode(6, OUTPUT);
  pinMode(tvocPin, OUTPUT);

  pmsSerial.begin(9600);
  Serial.begin(57600);
  espSerial.begin(9600); 
  pms.passiveMode(); 
  // DHT22 INIT
  dht.begin();

  // Warming up sensors
  digitalWrite(6, HIGH);        // Ozone sensor
  digitalWrite(tvocPin, HIGH);  // TVOC sensor
  delay(20 * 1000); // delay 20 seconds
  digitalWrite(6, LOW);
  digitalWrite(tvocPin, LOW);

  //   MQ131 INIT
  digitalWrite(tvocPin, HIGH);  // TVOC sensor
  MQ131.begin(6, A0, LOW_CONCENTRATION, 1000000); //
  MQ131.setTimeToRead(20); 
  MQ131.setR0(9000);
   

  // MP503
  Serial.println("Waiting sensor to init...");
  delay(20000);

  if (sensor.init()) {
    Serial.println("MP503 Sensor ready.");
  } else {
    Serial.println("MP503 Sensor ERROR!");
  }
  delay(2000);

  // Connection
   WiFi.begin(ssid,password);

   Serial.print("Connecting...");
   while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
  }
  Serial.println("Connected to WiFi with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}
}


void loop()
{
 Serial.println("LOOP");
   // Read DHT22 Data
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  pressure = round(bmp280.readPressure() / 100);
  cloud = map(round(lightMeter.readLightLevel()/65535*10000), 0, 100, 100, 0);

  MQ131.setEnv(temperature, humidity);

  // Read PM  
  pms.wakeUp();
  
  // Read MQ131 Data
  MQ131.sample();
  o3 = MQ131.getO3(PPB);
  o3 = map(o3, -32768, 32768, 10, 1000);
  
  // Read MP503
  digitalWrite(tvocPin, HIGH);
  
  int quality = sensor.slope();
  
  delay(10000); 
  
    // Read TVOC for 5 seconds 
  tvoc = analogRead(A1); 
  tvoc = map(tvoc, 0, 1024, 0, 100);
//  digitalWrite(tvocPin, LOW);
  digitalWrite(tvocPin, LOW);

  // Reading PMS data
  pms.requestRead();
  pmsSerial.listen();
  dataTimer3 = millis();
  while (millis() - dataTimer3 <= 1000) {
    pms.readUntil(data);
    pm1_0 = data.PM_AE_UG_1_0;
    pm2_5 = data.PM_AE_UG_2_5;
    pm10_0 = data.PM_AE_UG_10_0;
  }

  pms.sleep();

  String result = str = String("temp=")+String(temperature)+String("&")+String("hum=")+String(humidity)+String("&")+String("pres=")+String(pressure)+String("&")+String("clo=")+String(cloud)+String("&pm1=")+String(pm1_0)+String("&pm25=")+String(pm2_5)+String("&pm10=")+String(pm10_0)+String("&o3=")+String(o3)+String("&co2=")+String(tvoc);
  Serial.println(result);

  if(WiFi.status()==WL_CONNECTED){
    Serial.println("Still connected");
          
          WiFiClient client;
          HTTPClient http;
    
    String serverName = "http://express-weatherstation-production."+
    +"up.render.app/catalog/sensor?" + result;

    http.begin(client, serverName);

          http.addHeader("Content-Type", "application/x-www-form-urlencoded");

          int httpResponseCode = http.GET();
    Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          isDone=false;
          http.end();
  } else {
    result="";
    Serial.println("WiFi Disconnected");
  }


  Serial.println("LOOP END");
  delay(300000);
}

   

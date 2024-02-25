#include <Arduino.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <WiFi.h>
#include <HTTPClient.h>
#define DHTPIN 23
#define DHTTYPE DHT22
#define GREEN_LED 18
#define RED_LED 19
#define BLUE_LED 21 //WIFI
#define RAIN_SENSOR 33
#define BUZZER_PIN 14
#define RELAY_1_PIN 22
#define RELAY_2_PIN 25
#define LIMIT_1_PIN 39
#define LIMIT_2_PIN 34
#define GROUND_PIN_1 26
#define GROUND_PIN_2 27

int lightVal;
int env_status = 0;
const int ANALOG_READ_PIN = 36; // or A0
const int RESOLUTION = 12;      // Could be 9-12
const char *ssid = "OPPO A5 2020";
const char *password = "pawito236";
bool online = true;
const String api = "http://192.168.170.58:8000";

DHT dht(DHTPIN, DHTTYPE);
int is_collect = 0;
void showData(float h, float t, bool rainDetected, int env_status, bool is_auto, bool is_hanging);

void wi_fi(void *p);
bool isRainDetected();
bool flag = false;
bool flag_motor = false;

bool is_hanging = false;
bool is_auto = false;
bool is_beep = false;


const unsigned long beepInterval = 60000;
unsigned long lastBeepTime = -beepInterval; 

void switchRelay(void *p) ;
void update_status(float temperature, float humidity, float light, float rain, int is_hanging, int is_auto, int env_status);
void get_status();
void respond_force_collect();
void stop();
void forward();
void backward();
int environment_status(float temperature, float humidity, float light, float rainVal);

void beep(void *p);

void beep_alert();


void setup()
{
  // ledcWriteTone(3, 500);
  flag = !flag;
  Serial.begin(115200);
  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RELAY_1_PIN, OUTPUT);
  pinMode(RELAY_2_PIN, OUTPUT);
  pinMode(LIMIT_1_PIN, INPUT);
  pinMode(LIMIT_2_PIN, INPUT);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  ledcSetup(3, 8000, 12);
  ledcAttachPin(BUZZER_PIN, 3);
  byte i = 9;

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(BLUE_LED, !digitalRead(BLUE_LED));
    delay(500);
    if (++i % 10 == 0)
    {
      Serial.println("Connecting");
      Serial.print(WiFi.status());
    }
    else
      Serial.print(".");
    if (i > 100)
    {
      online = false;
      Serial.println("Connection failed");
      return;
    }
  }
  xTaskCreate(wi_fi, "wi_fi", 4096, NULL, 1, NULL);
  // xTaskCreate(switchRelay, "switchRelay", 4096, NULL, 0, NULL);
  xTaskCreate(beep, "beep", 4096, NULL, 0, NULL);
}

void loop()
{

  analogReadResolution(RESOLUTION);
  int rainVal = analogRead(RAIN_SENSOR);
  lightVal = analogRead(ANALOG_READ_PIN);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println("Failed to read from DHT sensor!");
    // return;
  }

  update_status(t, h, lightVal, rainVal, is_hanging, is_auto, env_status);
  get_status();
  env_status = environment_status(t, h, lightVal, rainVal);

  if(is_auto){
    if(env_status == 1 && is_hanging){
      Serial.println("Rain");
      backward();
      is_hanging = false;
      is_beep = false;
    } else if(env_status == 2 && is_hanging){
      Serial.println("Alert");
      // is_beep = true;
      beep_alert();
    } else if(env_status == 3 || env_status == 4 && !is_hanging){
      Serial.println("Sunny");
      forward();
      is_hanging = true;
      is_beep = false;
    } else {
      is_beep = false;
    }
  } 
  // else {
  //   if(!is_hanging && (env_status == 3 || env_status == 4)){
  //     // แจ้งเตือนให้เอาผ้าออกมาตาก เมื่อแดดออก (หากใช้ระบบ Manual)
  //     Serial.print("Not hang - beep");
  //     is_beep = true;
  //   } else {
  //     Serial.print("hang - not beep");
  //     is_beep = false;
  //   }
  // }

  showData(h, t, isRainDetected(), env_status, is_auto, is_hanging);
  delay(2000);
}

void showData(float h, float t, bool rainDetected, int env_status, bool is_auto, bool is_hanging)
{
  Serial.println("\n------------------------");
  Serial.print("Hanging Status: "); 
  if(is_hanging){
    Serial.println("Hanging"); 
  } else {
    Serial.println("Keeping");
  }

  Serial.print("System: "); 
  if(is_auto){
    Serial.println("Auto"); 
  } else {
    Serial.println("Manual");
  }

  Serial.print("Environment state: ");
  if(env_status == 1){
    Serial.println("Rain");
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  } else if(env_status == 2){
    Serial.println("Alert (Might Rain)");
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else if(env_status == 3){
    Serial.println("Sunny (Hot)");
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else {
    Serial.println("Sunny");
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  }

  Serial.print("Rain: ");
  if (rainDetected){
    Serial.print("Yes | ");
  }
  else{
    Serial.print("No | ");
  }
  Serial.print("Light: ");
  Serial.print(lightVal);
  Serial.print(" Humidity: ");
  Serial.print(h);
  Serial.print(" Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");
  Serial.println("------------------------\n");
}

void wi_fi(void *p)
{
  while (true)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      digitalWrite(BLUE_LED, HIGH);
      HTTPClient http;
      http.begin(api + "/status");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpResponseCode = http.GET();
      if (httpResponseCode > 0)
      {
        online = true;
      }
      else
      {
        online = false;
        Serial.print("Error code: ");
      }
      Serial.println(httpResponseCode);
      http.end();
    }
    else
    {
      digitalWrite(BLUE_LED, LOW);
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }
    delay(1000);
  }
}

int environment_status(float temperature, float humidity, float light, float rainVal){
  if( (rainVal >= 1500) || (humidity >= 80 && light <= 900)){
    return 1; //rain
  } else if(humidity >= 80 || light <= 900) {
    return 2; //alert before rain
  } else if(temperature >= 30){
    return 3; // sunny HOT
  } else {
    return 4; //sunny
  }
}

bool isRainDetected()
{
  int rainVal = analogRead(RAIN_SENSOR);
  // Serial.println(rainVal);
  return rainVal > 1500;
}

void switchRelay(void *p) {
  while (true) {
    if (flag_motor) {
      digitalWrite(RELAY_1_PIN, LOW);
      digitalWrite(RELAY_2_PIN, HIGH);
      // ledcWriteTone(3, 500);
      delay(15000);
    }
    else {
      digitalWrite(RELAY_1_PIN, HIGH);
      digitalWrite(RELAY_2_PIN, LOW);
      // ledcWriteTone(3, 0);
      delay(10000);
    }
    // Serial.print(digitalRead(RELAY_1_PIN));
    // Serial.print(" , ");
    // Serial.println(digitalRead(RELAY_2_PIN)); 
    flag_motor = !flag_motor;
    
  }
}


void update_status(float temperature, float humidity, float light, float rain, int is_hanging, int is_auto, int env_status){
  HTTPClient http;
  String url = api + "/update-sensor" + "?" + "temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&light=" + String(light) + "&rain=" + String(rain) + "&is_hanging=" + String(is_hanging) + "&is_auto=" + String(is_auto) + "&env_status=" + String(env_status) ;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      // Serial.println(payload);
    } else {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void get_status() {
  HTTPClient http;
  String url = api + "/status";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      // Extract variables from JSON
      float temperature = doc["temperature"];
      float humidity = doc["humidity"];
      float light = doc["light"];
      float rain = doc["rain"];
      int force_collect = doc["force_collect"];
      is_auto = doc["is_auto"];
      
      if(force_collect == 1){
        if(is_hanging){
          Serial.println("Collect in");
          backward();
          is_hanging = !is_hanging;
        } else {
          Serial.println("put out");
          forward();
          is_hanging = !is_hanging;
        }
        respond_force_collect();
      }
    } else {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}


void respond_force_collect() {
  HTTPClient http;
  String url = api + "/update-force-collect";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("update_force_collect");
    } else {
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void stop(){
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, LOW);
  is_beep = false;
}

void forward(){
  is_beep = true;
  digitalWrite(RELAY_1_PIN, HIGH);
  digitalWrite(RELAY_2_PIN, LOW);
  delay(8000);
  stop();
}

void backward(){
  is_beep = true;
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, HIGH);
  delay(8000);
  stop();
}

void beep(void *p){
  while(true){
    // Serial.println("beep in bg");
    if(is_beep){
      // Serial.print("is_beep: "); Serial.println(is_beep);
      // Serial.print("Beppppppp");
      ledcWriteTone(3, 500); 
      delay(100); 
      ledcWriteTone(3, 0);
      // Serial.println("- Stop Beppppppp");
      delay(200);
    } else {
      ledcWriteTone(3, 0);
    }
    // delay(100);
  }
}

void beep_alert(){
  Serial.println(millis() - lastBeepTime);
  if (millis() - lastBeepTime >= beepInterval) {
    is_beep = true;
    delay(1000);
    is_beep = false;
    lastBeepTime = millis(); // Update last beep time
  }

}

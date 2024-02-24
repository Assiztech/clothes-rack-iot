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
const int ANALOG_READ_PIN = 36; // or A0
const int RESOLUTION = 12;      // Could be 9-12
const char *ssid = "OPPO A5 2020";
const char *password = "pawito236";
bool online = true;
const String api = "http://192.168.170.58:8000";

DHT dht(DHTPIN, DHTTYPE);
int is_collect = 0;
void showData(float h, float t, bool rainDetected);

void wi_fi(void *p);
bool isRainDetected();
bool flag = false;
bool flag_motor = false;

bool is_hanging = false;
bool is_auto = false;

void switchRelay(void *p) ;
void update_status(float temperature, float humidity, float light, float rain, int is_hanging, int is_auto);
void get_status();
void respond_force_collect();
void stop();
void forward(int delay_time_ms);
void backward(int delay_time_ms);
int environment_status(float temperature, float humidity, float light, float rainVal);

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
  
  showData(h, t, isRainDetected());

  update_status(t, h, lightVal, rainVal, is_hanging, is_auto);
  get_status();
  int env_status = environment_status(t, h, lightVal, rainVal);

  if(is_auto){
    if(env_status == 1 && is_hanging){
      Serial.println("Rain");
      backward(3000);
      is_hanging = false;
    } else if(env_status == 2 && is_hanging){
      Serial.println("Alert");
      digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW);
    } else if(env_status == 3 && !is_hanging){
      Serial.println("Sunny");
      forward(3000);
      is_hanging = true;
    }
  }

  delay(2000);
}

void showData(float h, float t, bool rainDetected)
{
  Serial.print("Rain: ");
  if (rainDetected)
  {
    Serial.println("Rain Detected");
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
  }
  else
  {
    Serial.println("No Rain");
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  }
  Serial.print("Light: ");
  Serial.print(lightVal);
  Serial.print(" Humidity: ");
  Serial.print(h);
  Serial.print(" Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");
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
  if( (rainVal >= 1000) || (humidity >= 70 && light <= 1500)){
    return 1; //rain
  } else if(humidity >= 80 || light <= 1500) {
    return 2; //alert before rain
  } else {
    return 3; //sunny
  }
}

bool isRainDetected()
{
  int rainVal = analogRead(RAIN_SENSOR);
  // Serial.println(rainVal);
  return rainVal > 1000;
}

void switchRelay(void *p) {
  while (true) {
    if (flag_motor) {
      digitalWrite(RELAY_1_PIN, LOW);
      digitalWrite(RELAY_2_PIN, HIGH);
      // ledcWriteTone(3, 500);
    }
    else {
      digitalWrite(RELAY_1_PIN, HIGH);
      digitalWrite(RELAY_2_PIN, LOW);
      // ledcWriteTone(3, 0);
    }
    // Serial.print(digitalRead(RELAY_1_PIN));
    // Serial.print(" , ");
    // Serial.println(digitalRead(RELAY_2_PIN)); 
    flag_motor = !flag_motor;
    delay(1000);
  }
}


void update_status(float temperature, float humidity, float light, float rain, int is_hanging, int is_auto){
  HTTPClient http;
  String url = api + "/update-sensor" + "?" + "temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&light=" + String(light) + "&rain=" + String(rain) + "&is_hanging=" + String(is_hanging) + "&is_auto=" + String(is_auto);
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
      Serial.println("is_auto");
      
      if(force_collect == 1){
        if(is_hanging){
          Serial.println("Collect in");
          backward(3000);
          is_hanging = !is_hanging;
        } else {
          Serial.println("put out");
          forward(1000);
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
}

void forward(int delay_time_ms){
  digitalWrite(RELAY_1_PIN, HIGH);
  digitalWrite(RELAY_2_PIN, LOW);
  delay(delay_time_ms);
  stop();
}

void backward(int delay_time_ms){
  digitalWrite(RELAY_1_PIN, LOW);
  digitalWrite(RELAY_2_PIN, HIGH);
  delay(delay_time_ms);
  stop();
}

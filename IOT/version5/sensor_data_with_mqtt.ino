
#include <Arduino.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <PubSubClient.h>

const int MQ2_PIN = 32;
const int MQ7_PIN = 35;
#define DUST_LED_PIN 18
#define DUST_ANALOG_PIN 34
#define DHTPIN 19

const int RL_VALUE_MQ2 = 5;
float RO_CLEAN_AIR_FACTOR_MQ2 = 9.83;
float LPGCurve[3]   = {2.3, 0.21, -0.47};
float COCurve[3]    = {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};
float Ro_MQ2 = 10;

int CALIBRATION_SAMPLE_TIMES_MQ2    = 50;
int CALIBRATION_SAMPLE_INTERVAL_MQ2 = 500;
int READ_SAMPLE_INTERVAL_MQ2        = 50;
int READ_SAMPLE_TIMES_MQ2           = 5;

const float Vcc_MQ7 = 5.0;
const float RL_MQ7 = 10000.0;
float R0_MQ7 = 1.0;

int delayTime = 280;
int delayTime2 = 40;
float offTime = 9680;

int dustVal = 0;
char s[32];
float voltage = 0;
float dustdensity = 0;
DHTesp dht;

const char* ssid = "";
const char* password = "";
unsigned long myChannelNumber = 2999925;
const char* myWriteAPIKey = "";

WiFiClient client;
PubSubClient mqtt_client(client);

const char* mqtt_server = "";
const int mqtt_port = 1883;
const char* mqtt_topic = "16";
const int device_id = 16;

unsigned long previousMillis = 0;
const long interval = 15000;  // 15 giây cho ThingSpeak
unsigned long previousMillisMQTT = 0;
const long intervalMQTT = 1000;  // 1 giây cho MQTT

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(DUST_LED_PIN, OUTPUT);
  analogSetPinAttenuation(DUST_ANALOG_PIN, ADC_11db);
  dht.setup(DHTPIN, DHTesp::DHT11);

  Serial.print("Calibrating MQ2... ");
  Ro_MQ2 = MQCalibration(MQ2_PIN);
  Serial.println("done");
  Serial.print("Ro_MQ2 = "); Serial.print(Ro_MQ2); Serial.println(" kΩ");

  const int calib_samples = 50;
  float sum = 0;
  for(int i = 0; i < calib_samples; i++) {
    sum += analogRead(MQ7_PIN);
    delay(100);
  }
  float avg = sum / calib_samples;
  float V_sensor = avg * (3.3 / 4095.0);
  R0_MQ7 = (Vcc_MQ7 / V_sensor - 1.0) * RL_MQ7;
  Serial.print("Calibration R0_MQ7 = ");
  Serial.println(R0_MQ7);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Device ID: ");
  Serial.println(device_id);

  ThingSpeak.begin(client);
  mqtt_client.setServer(mqtt_server, mqtt_port);
  reconnect();

  delay(3000);
}

void reconnect() {
  while (!mqtt_client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(device_id);
    if (mqtt_client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Gửi dữ liệu qua ThingSpeak mỗi 15 giây
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    TempAndHumidity data = dht.getTempAndHumidity();
    float temperature = data.temperature;
    float humidity = data.humidity;

    int raw_adc_mq2 = analogRead(MQ2_PIN);
    float rs_ro_ratio_mq2 = MQRead(MQ2_PIN) / Ro_MQ2;
    long ppmLPG   = MQGetGasPercentage(rs_ro_ratio_mq2, 0);
    long ppmCO_MQ2    = MQGetGasPercentage(rs_ro_ratio_mq2, 1);
    long ppmSmoke = MQGetGasPercentage(rs_ro_ratio_mq2, 2);

    if (ppmLPG > 10000) ppmLPG = 10000;
    if (ppmCO_MQ2 > 2000) ppmCO_MQ2 = 2000;
    if (ppmSmoke > 10000) ppmSmoke = 10000;

    const int samples = 10;
    float sum_mq7 = 0;
    for(int i = 0; i < samples; i++) {
      sum_mq7 += analogRead(MQ7_PIN);
      delay(10);
    }
    float sensorVal_mq7 = sum_mq7 / samples;
    float V_sensor_mq7 = sensorVal_mq7 * (3.3 / 4095.0);
    float Rs_mq7 = (Vcc_MQ7 / V_sensor_mq7 - 1.0) * RL_MQ7;
    float ratio_mq7 = Rs_mq7 / R0_MQ7;
    float ppmCO_MQ7 = 0.93 * pow(ratio_mq7, -1.709);

    if (ppmCO_MQ7 > 2000) ppmCO_MQ7 = 2000;

    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(delayTime);
    dustVal = analogRead(DUST_ANALOG_PIN);
    delayMicroseconds(delayTime2);
    digitalWrite(DUST_LED_PIN, HIGH);
    delayMicroseconds(offTime);
    voltage = (dustVal / 4095.0) * 3.3;
    dustdensity = 172 * voltage - 100;
    if (dustdensity < 0) dustdensity = 0;
    if (dustdensity > 500) dustdensity = 500;

    if (!isnan(temperature) && !isnan(humidity)) {
      ThingSpeak.setField(1, temperature);
      ThingSpeak.setField(2, humidity);
      ThingSpeak.setField(3, sensorVal_mq7);
      ThingSpeak.setField(4, ppmCO_MQ7);
      ThingSpeak.setField(5, raw_adc_mq2);
      ThingSpeak.setField(6, ppmLPG);
      ThingSpeak.setField(7, ppmSmoke);
      ThingSpeak.setField(8, dustdensity);
      ThingSpeak.setStatus("Device ID: " + String(device_id));
      int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if (status == 200) {
        Serial.println("Data sent to ThingSpeak");
      } else {
        Serial.print("Failed to send data. Error code: ");
        Serial.println(status);
        if (status == 404) {
          Serial.println("404: Channel ID không tồn tại.");
        } else if (status == 400) {
          Serial.println("400: Dữ liệu không hợp lệ hoặc thiếu field.");
        } else if (status == 401) {
          Serial.println("401: Write API Key sai.");
        } else if (status == -320) {
          Serial.println("-320: Kênh không tồn tại hoặc không có quyền.");
        } else {
          Serial.println("Lỗi không xác định.");
        }
      }
    } else {
      Serial.println("Failed to read from DHT11 sensor!");
    }

    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
    Serial.print("MQ7 raw: "); Serial.println(sensorVal_mq7);
    Serial.print("MQ7 CO ppm: "); Serial.println(ppmCO_MQ7);
    Serial.print("MQ2 raw: "); Serial.println(raw_adc_mq2);
    Serial.print("MQ2 LPG ppm: "); Serial.println(ppmLPG);
    Serial.print("MQ2 Smoke ppm: "); Serial.println(ppmSmoke);
    Serial.print("Dust density: "); Serial.print(dustdensity); Serial.println(" ug/m3");
  }

  // Gửi dữ liệu qua MQTT mỗi 1 giây
  if (currentMillis - previousMillisMQTT >= intervalMQTT) {
    previousMillisMQTT = currentMillis;

    TempAndHumidity data = dht.getTempAndHumidity();
    float temperature = data.temperature;
    float humidity = data.humidity;

    int raw_adc_mq2 = analogRead(MQ2_PIN);
    float rs_ro_ratio_mq2 = MQRead(MQ2_PIN) / Ro_MQ2;
    long ppmLPG   = MQGetGasPercentage(rs_ro_ratio_mq2, 0);
    long ppmCO_MQ2    = MQGetGasPercentage(rs_ro_ratio_mq2, 1);
    long ppmSmoke = MQGetGasPercentage(rs_ro_ratio_mq2, 2);

    if (ppmLPG > 10000) ppmLPG = 10000;
    if (ppmCO_MQ2 > 2000) ppmCO_MQ2 = 2000;
    if (ppmSmoke > 10000) ppmSmoke = 10000;

    const int samples = 10;
    float sum_mq7 = 0;
    for(int i = 0; i < samples; i++) {
      sum_mq7 += analogRead(MQ7_PIN);
      delay(10);
    }
    float sensorVal_mq7 = sum_mq7 / samples;
    float V_sensor_mq7 = sensorVal_mq7 * (3.3 / 4095.0);
    float Rs_mq7 = (Vcc_MQ7 / V_sensor_mq7 - 1.0) * RL_MQ7;
    float ratio_mq7 = Rs_mq7 / R0_MQ7;
    float ppmCO_MQ7 = 0.93 * pow(ratio_mq7, -1.709);

    if (ppmCO_MQ7 > 2000) ppmCO_MQ7 = 2000;

    digitalWrite(DUST_LED_PIN, LOW);
    delayMicroseconds(delayTime);
    dustVal = analogRead(DUST_ANALOG_PIN);
    delayMicroseconds(delayTime2);
    digitalWrite(DUST_LED_PIN, HIGH);
    delayMicroseconds(offTime);
    voltage = (dustVal / 4095.0) * 3.3;
    dustdensity = 172 * voltage - 100;
    if (dustdensity < 0) dustdensity = 0;
    if (dustdensity > 500) dustdensity = 500;

    if (!isnan(temperature) && !isnan(humidity)) {
      String payload = "{";
      payload += "\"device_id\":" + String(device_id) + ",";
      payload += "\"temperature\":" + String(temperature, 1) + ",";
      payload += "\"humidity\":" + String(humidity, 1) + ",";
      payload += "\"sensorVal_mq7\":" + String(sensorVal_mq7, 1) + ",";
      payload += "\"ppmCO_MQ7\":" + String(ppmCO_MQ7, 1) + ",";
      payload += "\"raw_adc_mq2\":" + String(raw_adc_mq2) + ",";
      payload += "\"ppmLPG\":" + String(ppmLPG) + ",";
      payload += "\"ppmSmoke\":" + String(ppmSmoke) + ",";
      payload += "\"dustdensity\":" + String(dustdensity, 1);
      payload += "}";

      if (!mqtt_client.connected()) {
        reconnect();
      }
      if (mqtt_client.publish(mqtt_topic, payload.c_str())) {
        Serial.println("Data sent to MQTT broker");
      } else {
        Serial.println("Failed to send data to MQTT broker");
      }
    } else {
      Serial.println("Failed to read from DHT11 sensor for MQTT!");
    }
  }

  delay(10);
}

float MQResistanceCalculation(int raw_adc) {
  return ( (float)RL_VALUE_MQ2 * (4095 - raw_adc) / raw_adc );
}

float MQCalibration(int mq_pin) {
  float val = 0;
  for (int i = 0; i < CALIBRATION_SAMPLE_TIMES_MQ2; i++) {
    val += MQResistanceCalculation( analogRead(mq_pin) );
    delay(CALIBRATION_SAMPLE_INTERVAL_MQ2);
  }
  val /= CALIBRATION_SAMPLE_TIMES_MQ2;
  val /= RO_CLEAN_AIR_FACTOR_MQ2;
  return val;
}

float MQRead(int mq_pin) {
  float rs = 0;
  for (int i = 0; i < READ_SAMPLE_TIMES_MQ2; i++) {
    rs += MQResistanceCalculation( analogRead(mq_pin) );
    delay(READ_SAMPLE_INTERVAL_MQ2);
  }
  rs /= READ_SAMPLE_TIMES_MQ2;
  return rs;
}

long MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
  if (gas_id == 0) {
    return MQGetPercentage(rs_ro_ratio, LPGCurve);
  } else if (gas_id == 1) {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  } else if (gas_id == 2) {
    return MQGetPercentage(rs_ro_ratio, SmokeCurve);
  }
  return 0;
}

long MQGetPercentage(float rs_ro_ratio, float *pcurve) {
  return pow(10, ((log(rs_ro_ratio) - pcurve[1]) / pcurve[2] + pcurve[0]));
}

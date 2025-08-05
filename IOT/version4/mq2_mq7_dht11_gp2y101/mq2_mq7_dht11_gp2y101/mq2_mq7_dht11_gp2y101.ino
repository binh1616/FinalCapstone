// File: main.ino

#include <Arduino.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <ThingSpeak.h>
#include <ArduinoOTA.h>

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

const char* ssid = "F214L";
const char* password = "0374250247";
unsigned long myChannelNumber = 3026849;
const char* myWriteAPIKey = "5QBXCU8I6UNA0DQB";

IPAddress staticIP(192, 168, 1, 104);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);

WiFiClient client;

unsigned long previousMillis = 0;
const long interval = 15000;

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

  if (!WiFi.config(staticIP, gateway, subnet, dns)) {
    Serial.println("Failed to configure static IP");
  } else {
    Serial.println("Static IP configured");
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ThingSpeak.begin(client);

  ArduinoOTA.setHostname("ESP32-OTA");
  ArduinoOTA.begin();
  Serial.println("OTA Ready");

  delay(3000);
}

void loop() {
  ArduinoOTA.handle();

  unsigned long currentMillis = millis();
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
      ThingSpeak.setField(6, ppmCO_MQ2);
      ThingSpeak.setField(7, ppmSmoke);
      ThingSpeak.setField(8, dustdensity);
      int status = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
      if (status == 200) {
        Serial.println("Data sent to ThingSpeak");
      } else {
        Serial.println("Failed to send data");
      }
    } else {
      Serial.println("Failed to read from DHT11 sensor!");
    }

    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
    Serial.print("MQ7 raw: "); Serial.println(sensorVal_mq7);
    Serial.print("MQ7 CO ppm: "); Serial.println(ppmCO_MQ7);
    Serial.print("MQ2 raw: "); Serial.println(raw_adc_mq2);
    Serial.print("MQ2 CO ppm: "); Serial.println(ppmCO_MQ2);
    Serial.print("MQ2 Smoke ppm: "); Serial.println(ppmSmoke);
    Serial.print("Dust density: "); Serial.print(dustdensity); Serial.println(" ug/m3");
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

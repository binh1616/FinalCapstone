#include <Arduino.h>
#include <DHTesp.h>  // Thêm thư viện DHTesp cho ESP32
#include <WiFi.h>    // Thêm thư viện WiFi
#include <ArduinoOTA.h> // Thêm thư viện OTA

#define DUST_LED_PIN 18
#define DUST_ANALOG_PIN 34
#define DHTPIN 19  // Định nghĩa chân cho DHT11

int delayTime = 280;
int delayTime2 = 40;
float offTime = 9680; // 10000 - 280 - 40

int dustVal = 0;
char s[32];
float voltage = 0;
float dustdensity = 0;

DHTesp dht;  // Khai báo đối tượng DHTesp

// Thông tin WiFi
const char* ssid = "MYLAP_8545";       // Thay bằng SSID của bạn
const char* password = "123456789";    // Thay bằng mật khẩu WiFi của bạn

// Thiết lập IP tĩnh
IPAddress staticIP(192, 168, 137, 125); // Địa chỉ IP tĩnh mong muốn
IPAddress gateway(192, 168, 137, 1);    // Địa chỉ gateway
IPAddress subnet(255, 255, 255, 0);     // Mặt nạ mạng
IPAddress dns(8, 8, 8, 8);              // DNS server (tùy chọn)

void setup() {
  Serial.begin(9600);
  pinMode(DUST_LED_PIN, OUTPUT);
  analogSetPinAttenuation(DUST_ANALOG_PIN, ADC_11db); // Set ADC to measure up to ~3.3V
  dht.setup(DHTPIN, DHTesp::DHT11);  // Khởi tạo DHT11

  // Thiết lập IP tĩnh
  if (!WiFi.config(staticIP, gateway, subnet, dns)) {
    Serial.println("Failed to configure static IP");
  }

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Khởi động OTA
  ArduinoOTA.setHostname("ESP32-OTA");  // Đặt tên cho thiết bị OTA
  // Tùy chọn: Thêm mật khẩu OTA để tăng bảo mật
  ArduinoOTA.setPassword("your_ota_password");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}

void loop() {
  // Xử lý OTA
  ArduinoOTA.handle();

  digitalWrite(DUST_LED_PIN, LOW); // Power on the LED
  delayMicroseconds(delayTime);
  dustVal = analogRead(DUST_ANALOG_PIN); // Read the dust value
  delayMicroseconds(delayTime2);
  digitalWrite(DUST_LED_PIN, HIGH); // Turn the LED off
  delayMicroseconds(offTime);
  
  voltage = (dustVal / 4095.0) * 3.3; // Convert 12-bit ADC reading to voltage (0-3.3V)
  dustdensity = 172 * voltage - 100; // Calculate dust density directly in µg/m³
  
  if (dustdensity < 0)
    dustdensity = 0;
  if (dustdensity > 500)
    dustdensity = 500;

  // Đọc dữ liệu từ DHT11
  TempAndHumidity data = dht.getTempAndHumidity();
  float temperature = data.temperature;
  float humidity = data.humidity;

  // Kiểm tra nếu đọc thành công
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor!");
  } else {
    Serial.print("Analog: ");
    Serial.print(dustVal);
    Serial.print(", Voltage: ");
    Serial.print(voltage, 4);
    Serial.print(" V, Dust Density: ");
    Serial.print(dustdensity, 2);
    Serial.print(" ug/m3, Temperature: ");
    Serial.print(temperature, 2);
    Serial.print(" C, Humidity: ");
    Serial.print(humidity, 2);
    Serial.println(" %");
  }
  delay(1000);
}
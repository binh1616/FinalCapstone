# IoT-Based Urban Air Quality Monitoring and Forecasting System

## Project Overview

This project implements an IoT-enabled system for real-time urban air quality monitoring and short-term forecasting using a distributed 5×5 grid of low-cost environmental sensors. The system combines hardware sensing (ESP32-based nodes), cloud storage (ThingSpeak), advanced spatio-temporal forecasting with a hybrid CNN–LSTM model, and an interactive web dashboard for visualization and alerts.

The primary goal is to provide accessible, high-resolution spatial and temporal air quality data to support public awareness, research, and decision-making in polluted urban areas (with a focus on cities like Hanoi, Vietnam).

### Key Features
- **Distributed Sensing Network**: 25 sensor nodes arranged in a 5×5 grid for spatial coverage.
- **Sensors**:
  - DHT11: Temperature and humidity
  - MQ-2: Combustible gases (e.g., LPG, smoke)
  - MQ-7: Carbon monoxide (CO)
  - GP2Y1010AU0F: PM2.5 particulate matter
- **Data Transmission**: ESP32 microcontrollers collect, calibrate, and upload data via Wi-Fi (HTTP/MQTT) to ThingSpeak.
- **Data Preprocessing**: Timestamp alignment, missing value interpolation, and feature scaling.
- **AI Forecasting**: Hybrid CNN–LSTM model for multi-variable, multi-sensor spatio-temporal prediction (3 past timesteps → next timestep).
- **Interactive Dashboard**: Heatmaps, time-series plots, historical filtering, prediction overlays, and alert indicators.
- **Performance**: Normalized RMSE ≈ 0.1043 (promising initial results).

## Motivation

Air pollution is a major public health crisis. In Vietnam:
- Ranked 22nd globally and 2nd in ASEAN (IQAir 2023)
- Hanoi frequently among the world's most polluted cities (PM2.5 ~6× WHO guideline)
- ~60,000–70,000 premature deaths annually
- Economic loss: $11–13 billion/year (~4–5% GDP)

This project aims to contribute affordable, scalable monitoring tools to support better urban air quality management.

## Repository Structure
.
├── AI/                  # AI forecasting model (CNN–LSTM)
├── Dashboard/           # React web dashboard
├── IOT/                 # ESP32 firmware
└── README.md            # This file


## Components

### 1. IOT Layer (ESP32 Sensors)
- Hardware per node: ESP32, DHT11, MQ-2, MQ-7, GP2Y1010AU0F, resistors, breadboard, power supply
- Firmware: `IOT/sensor_data_with_mqtt.ino`
- Upload via Arduino IDE, configure Wi-Fi + ThingSpeak/MQTT credentials
- Deploy 25 nodes in 5×5 grid

### 2. Cloud Storage
- Create ThingSpeak channel with fields: Temperature, Humidity, CO, Gas, PM2.5 (+ optional Node ID/Grid position)
- Use Channel ID and Write API Key in ESP32 code

### 3. AI Forecasting Model
**Kiến trúc mô hình (Hybrid CNN–LSTM):**

- Nhiệm vụ: Dự báo spatio-temporal đa biến trên lưới 5×5
- Input: (samples, 3 timesteps trước, 5, 5, 5 features) → dự đoán timestep tiếp theo
- Các layer chính:
  - TimeDistributed Conv2D(32 filters, kernel 3×3, ReLU, padding same)
  - TimeDistributed MaxPooling2D(2×2)
  - TimeDistributed Flatten
  - LSTM(64 units, tanh)
  - Dropout(0.2)
  - Dense(125) → output cho 25 node × 5 features
- Dữ liệu được normalize về [0,1]
- Hiệu suất ban đầu: RMSE ≈ 0.1043 (normalized space)

### 4. Dashboard

🎯 Để làm gì?

Xây dựng dashboard giám sát dữ liệu cảm biến từ ThingSpeak theo thời gian thực, giúp:

- Theo dõi nhiệt độ, độ ẩm, khí CO, gas dễ cháy, bụi PM2.5…
- Hiển thị trực quan bằng thẻ số liệu, heatmap lưới 5×5, biểu đồ dòng thời gian
- Overlay dự báo từ mô hình AI
- Cảnh báo khi giá trị vượt ngưỡng nguy hiểm (màu sắc + thông báo)

👉 Phù hợp cho hệ thống giám sát chất lượng không khí, IoT môi trường, phòng lab, nhà thông minh.

🛠 Dùng công nghệ gì?

- React.js – Giao diện dashboard component-based
- JavaScript (ES6+) – Logic xử lý, fetch API, tính màu ngưỡng, cấu hình cảm biến
- CSS – Responsive, dark mode, animation
- ThingSpeak REST API – Lấy dữ liệu real-time & historical
- Chart library (Recharts/Chart.js) – Biểu đồ đẹp, tương tác

Tóm lại: Frontend React dashboard hiện đại + dữ liệu IoT từ ThingSpeak + hiển thị thông minh với cảnh báo và dự báo.

**Chạy dashboard:**
```bash
cd Dashboard
npm install
npm start

Future Improvements

Thêm mô hình AI nâng cao (Transformer/GNN)
Mobile app
Anomaly detection thời gian thực
Public API


This project is released into the public domain under **The Unlicense**.

You are free to use, copy, modify, distribute, and use this project for any purpose, including commercial use, without asking for permission and without attribution.

This software is provided "as is", without warranty of any kind.

For more details, see: [https://unlicense.org](https://unlicense.org)



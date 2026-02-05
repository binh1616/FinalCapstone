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
├── AI/                  # AI Section: CNN–LSTM forecasting model (Jupyter notebook, sample data, requirements)
├── Dashboard/           # Dashboard Section: React web application for monitoring & data visualization
├── IOT/                 # IoT Section: ESP32 firmware (sensor_data_with_mqtt.ino, related libraries)
└── README.md            # This documentation file


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
**Model Architecture (Hybrid CNN–LSTM):**

- Task: Multi-variable spatio-temporal forecasting on a 5×5 grid
- Input: (samples, 3 previous timesteps, 5, 5, 5 features) → predict the next timestep
- Main layers:
  - TimeDistributed Conv2D(32 filters, kernel 3×3, ReLU, padding same)
  - TimeDistributed MaxPooling2D(2×2)
  - TimeDistributed Flatten
  - LSTM(64 units, tanh)
  - Dropout(0.2)
  - Dense(125) → output for 25 nodes × 5 features
- All data is normalized to the [0,1] range
- Initial performance: RMSE ≈ 0.1043 (in normalized space)

### 4. Dashboard

🎯 Purpose  
Build a real-time dashboard to monitor sensor data from ThingSpeak, enabling users to:

- Track temperature, humidity, CO, combustible gases, PM2.5…
- Visualize data through metric cards, 5×5 grid heatmaps, and time-series charts
- Overlay AI model predictions
- Receive alerts when values exceed dangerous thresholds (color changes + notifications)

👉 Suitable for air quality monitoring systems, environmental IoT, labs, smart homes.

🛠 Technologies used

- React.js – Component-based dashboard interface
- JavaScript (ES6+) – Logic handling, API fetching, threshold-based coloring, sensor configuration
- CSS – Responsive design, dark mode, animations
- ThingSpeak REST API – Fetch real-time & historical data
- Chart library (Recharts / Chart.js) – Beautiful, interactive charts

In short: Modern React frontend dashboard + IoT data from ThingSpeak + smart visualization with alerts and forecasting overlays.

## Future Improvements
- Add advanced AI models (Transformer / GNN)
- Mobile application
- Real-time anomaly detection
- Public API

This project is released into the public domain under **The Unlicense**.

You are free to use, copy, modify, distribute, and use this project for any purpose, including commercial use, without asking for permission and without attribution.

This software is provided "as is", without warranty of any kind.

For more details, see: [https://unlicense.org](https://unlicense.org)



# ESP32 Medibox

The **ESP32 Medibox** is a smart medicine box designed to help users manage their medication schedule effectively. Built using the ESP32 microcontroller, this project integrates various sensors and components to monitor environmental conditions, provide alerts, and allow remote control via MQTT.

## Features

- **WiFi Connectivity**: Remote monitoring and control using MQTT.
- **Temperature & Humidity Monitoring**: DHT22 sensor for real-time environmental data.
- **Light Intensity Detection**: LDRs to adjust a servo-controlled sliding window based on ambient light.
- **Buzzer Alerts**: Audible notifications for scheduled medication times.
- **Customizable Settings**: Modify the servo motor's behavior via MQTT commands.

## Components

- **ESP32**: Microcontroller
- **DHT22**: Temperature & humidity sensor
- **LDR**: Light-dependent resistor for measuring light intensity
- **Servo Motor**: Controls the shaded window
- **Buzzer**: Provides audio alerts
- **Buttons**: For user interaction

## How It Works

The ESP32 Medibox continuously monitors temperature, humidity, and light intensity, providing buzzer notifications at scheduled times. It also allows remote control via MQTT to adjust settings like the servo motor’s angle and controlling factors. The system can be integrated with **Node-RED** for visualization and control.

## Getting Started

1. **Clone Repository**:
   ```bash
   git clone https://github.com/YourUsername/ESP32-Medibox.git
   cd ESP32-Medibox

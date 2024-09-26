# Smart-Medi-Box

The **Smart-Medi-Box** is a medicine box designed to help users manage their medication schedule in a smart way. Built using the ESP32 microcontroller, this project integrates various sensors and components to monitor environmental conditions, provide alerts, and allow remote control via MQTT.

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

### Continuous Monitoring

- **Environmental Sensing**: Integrated with a DHT22 sensor, the Medibox continuously monitors the temperature and humidity levels to ensure medications are stored under optimal conditions. It also includes an LDR (Light Dependent Resistor) to assess ambient light intensity, which is crucial for medications sensitive to light exposure.

### Notifications and Alerts

- **Buzzer Notifications**: To assist users with adherence to medication schedules, the Medibox is equipped with a buzzer that sounds alerts at prescribed times. These alerts can be customized and managed remotely, ensuring users are timely reminded without needing to interact directly with the device.

### Remote Control and Customization

- **MQTT Integration**: Utilizing MQTT, a popular IoT messaging protocol, the Medibox allows users to monitor, control, and adjust settings remotely. This functionality includes setting or altering the angles of servo motors based on the user’s needs or environmental inputs, offering flexibility and control from anywhere.

- **Servo Motor Adjustment**: The servo motor dynamically adjusts the Medibox's internal mechanisms, such as moving partitions or modulating a cover to protect contents from light, based on automated settings or manual user input.

### Visualization and Control

- **Node-RED Dashboard**: The Node-RED dashboard serves as a central hub where users can easily configure alert schedules, monitor environmental conditions, and control actuators such as the servo motor and buzzer.

## Getting Started

1. **Clone Repository**:
   ```bash
   git clone https://github.com/DinukaMadhushan1234/Embedded-Systems-Smart-Medi-Box-project.git
   cd Embedded-Systems-Smart-Medi-Box-project
2. The complete circuit diagram and simulation can be found on [Wokwi](https://wokwi.com/projects/397564322503042049).

   

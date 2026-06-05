# smart dryHerb (project_cpe_y1_sem2)
A smart IoT-based herb drying system developed as a Computer Engineering Year 1 Semester 2 project (project_cpe_y1_sem2). 

This project aims to automate and monitor the drying process of herbs to maintain high quality and prevent mold growth. By monitoring environmental factors in real-time, the system automatically adjusts conditions to provide the optimal drying environment.

## Features
- Real-time temperature and humidity monitoring.
- Automated hardware control (e.g., ventilation fans, heating elements) based on sensor thresholds.
- Data visualization via [ระบุแพลตฟอร์ม เช่น Web Dashboard, Blynk, Firebase, or MQTT].
- Alerts and status updates for the drying cycle.

## Tech Stack & Software
- **Language:** C/C++ (Arduino Framework)
- **IDE:**  IDE เช่น Arduino IDE 
- **IoT Platform:** Blynk

## Hardware Components
- **Microcontroller:** [ระบุบอร์ด เช่น ESP32 / ESP8266 / Arduino Uno]
- **Sensors:** เช่น DHT22 for Temperature/Humidity]
- **Actuators:**  เช่น 5V Relay Module, DC Fan, Heater]
- **Others:** Breadboard, Jumper wires, Power supply

## System Architecture
1. **Data Collection:** The system reads environmental data from the connected sensors.
2. **Processing:** The microcontroller evaluates the data against predefined optimal thresholds for herb drying.
3. **Action:** If the humidity is too high or temperature is too low, the system triggers the relays to turn on the fan or heater.
4. **Monitoring:** All data is transmitted to the dashboard for user observation.


git clone [https://github.com/apfirst13/smart-dryHerb.git](https://github.com/apfirst13/smart-dryHerb.git)
<img width="1107" height="611" alt="image" src="https://github.com/user-attachments/assets/44015169-b85f-4a94-b755-4a375d65fdef" />

<img width="962" height="485" alt="image" src="https://github.com/user-attachments/assets/775ed805-17f5-4249-b658-715d7038fb04" />
<img width="1087" height="615" alt="image" src="https://github.com/user-attachments/assets/34151519-04e2-4305-812f-717304588acd" />

<img width="857" height="447" alt="image" src="https://github.com/user-attachments/assets/58bf1569-efc0-4b4e-96c0-bb52b3d62fa1" />
<img width="1536" height="2048" alt="image" src="https://github.com/user-attachments/assets/1e958fcb-a984-4887-a50c-241df88efe57" />


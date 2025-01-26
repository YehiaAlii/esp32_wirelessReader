# ESP32 Wireless Reader

ESP32 Wireless Reader is a project that allows users to manage files on an SD card via a web interface. The ESP32 acts as a WiFi Access Point which enables users to connect and interact with the file system using a web browser.

## Features

- **WiFi Access Point**: The ESP32 creates a wireless network that users can connect to.
- **File Management**: Browse, download, and delete files on the SD card.
- **Web Interface**: User-friendly interface for managing files.
- **Physical Control**: Start and stop the Access Point using a physical switch.

## Hardware Requirements

- **ESP32 Development Board**: Any ESP32 board with sufficient GPIO pins.
- **SD Card Module**: For file storage and management.
- **MicroSD Card**: To store files.
- **Push Button or Switch**: Connected to GPIO 4 for controlling the Access Point.

## Software Requirements

- **Arduino IDE**: For programming the ESP32.
- **WiFi and SD Libraries**: Included with the ESP32 board package.

## Setup Instructions

1. **Hardware Connections**:
   - Connect the SD card module to the ESP32 using SPI protocol:
     - 3.3V --------> VCC
     - GND --------> GND
     - GPIO 5 -------> CS
     - GPIO 23 ------> MOSI
     - GPIO 19 ------> MISO
     - GPIO 18 ------> SCK
   - Connect the switch to GPIO 4 and GND.

2. **Arduino IDE Setup**:
   - Install the ESP32 board package in the Arduino IDE.
   - Open the `esp32_wirelessReader.ino` file in the Arduino IDE.

3. **Upload the Code**:
   - Select the correct ESP32 board and port in the Arduino IDE.
   - Upload the code to the ESP32.

## Usage

1. **Connect to the WiFi Network**:
   - Look for the network named `ESP32-AP` and connect using the password `12345678`.

2. **Access the Web Interface**:
   - Open a web browser and navigate to `http://192.168.4.1`.
   - Use the interface to browse, download, and delete files on the SD card.

3. **Control the Access Point**:
   - Use the physical switch to start or stop the Access Point.

## Contributing

Contributions are welcome! Please fork the repository and submit a pull request for any improvements or bug fixes.

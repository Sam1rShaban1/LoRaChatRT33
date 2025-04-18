#pragma once

// Choose the device, choose it directly in the platformio.ini file
#define T_BEAM_V10 // ttgo-t-beam
// #define T_BEAM_LORA_32 // ttgo-lora32-v1
// #define NAYAD_V1
// #define NAYAD_V1R2
#define LM_GOD_MODE

// #define GPS_ENABLED // does not work with board v1.2
#if defined(NAYAD_V1) || defined(NAYAD_V1R2)
#define DISPLAY_ENABLED
#define BATTERY_ENABLED
// #define LED_ENABLED
// #define SENSORS_ENABLED
// #define METADATA_ENABLED
// #define SIMULATION_ENABLED
// #define BLUETOOTH_ENABLED
#else
#define DISPLAY_ENABLED
// #define LED_ENABLED
#define LORA_ENABLED
// #define NO_SENSOR_DATA // If the sensors are not connected
// #define SIMULATION_ENABLED
// #define BLUETOOTH_ENABLED
#endif

// Configuration

// Display Configuration
#ifdef NAYAD_V1
#define I2C_SDA 02
#define I2C_SCL 04
#elif defined(NAYAD_V1R2) || defined(T_BEAM_V10) || defined(T_BEAM_LORA_32)
#define I2C_SDA SDA
#define I2C_SCL SCL
#else
#warning "I2C_SDA and I2C_SCL not defined"
#define I2C_SDA 0
#define I2C_SCL 0
#endif

//If the device has a GPS module
#if defined(T_BEAM_V10)
#define GPS_TX 12
#define GPS_RX 34
#elif defined(NAYAD_V1)
#define GPS_RX 25
#define GPS_TX 35
#elif defined(NAYAD_V1R2)
#define GPS_RX 4
#define GPS_TX 2
#endif
#define GPS_BAUD 9600
#define UPDATE_GPS_DELAY 120000 //ms

#if defined(T_BEAM_LORA_32) && defined(GPS_ENABLED)
#warning "GPS in T_BEAM_LORA_32 is not default supported"
#endif

#if defined(T_BEAM_LORA_32)
#define DISPLAY_SDA OLED_SDA
#define DISPLAY_SCL OLED_SCL
#define DISPLAY_RST OLED_RST
#elif defined(NAYAD_V1) || defined(NAYAD_V1R2) || defined(T_BEAM_V10)
#define DISPLAY_SDA I2C_SDA
#define DISPLAY_SCL I2C_SCL
#define DISPLAY_RST -1
#else
#warning "DISPLAY_SDA and DISPLAY_SCL not defined"
#define DISPLAY_SDA 0
#define DISPLAY_SCL 0
#define DISPLAY_RST -1
#endif

//WiFi Configuration
#define WIFI_ENABLED
#define MAX_CONNECTION_TRY 10

// WiFi credentials
#define WIFI_SSID "MS"
// #define WIFI_SSID "testing"
#define WIFI_PASSWORD "12345678"

// MQTT configuration
#define MQTT_ENABLED
#define MQTT_SERVER "34.107.117.161" 
#define MQTT_PORT 1883
#define MQTT_USERNAME "admin"
#define MQTT_PASSWORD "public"
#define MQTT_TOPIC_SUB "from-server/"
#define MQTT_TOPIC_OUT "to-server/"
#define MQTT_MAX_PACKET_SIZE 2048 // 128, 256 or 512
#define MQTT_MAX_QUEUE_SIZE 10
#define MQTT_STILL_CONNECTED_INTERVAL 300000 // In milliseconds, 0 to disable

// MQTT_MON configuration
#define MQTT_MON_ENABLED
#define MON_SENDING_EVERY 10000 //ms
// #define MON_SENDING_EVERY 10000 //ms (testing)

#if defined(BLUETOOTH_ENABLED)
#undef WIFI_ENABLED
#undef MQTT_ENABLED
#endif

// Sensors Configuration
#define STORED_SENSOR_DATA 10
    //- Temperature Configuration
#define SOIL_SENSOR_PIN 12
#define SENSOR_SENDING_EVERY 60000 //ms
    //- Metadata Configuration
#define METADATA_UPDATE_DELAY 300000 //ms
// while developing
// #define METADATA_UPDATE_DELAY 10000 //ms

// Battery configuration
#define BATTERY_PIN 34
#define DEEP_SLEEP_TIME 3600 // In seconds

// Led configuration
#if defined(NAYAD_V1)
#define LED 4
#define LED_ON      LOW
#define LED_OFF     HIGH
#elif defined(T_BEAM_LORA_32)
#define LED LED_BUILTIN
#define LED_ON      HIGH
#define LED_OFF     LOW
#elif defined(T_BEAM_V10)
#define LED 4
#define LED_ON      LOW
#define LED_OFF     HIGH
#elif defined(NAYAD_V1R2)
#define LED 13
#define LED_ON      HIGH
#define LED_OFF     LOW
#else
#warning "LED not defined"
#define LED 255U
#define LED_ON      HIGH
#define LED_OFF     LOW
#endif

// LoRa Configuration
#if defined(T_BEAM_LORA_32) || defined(T_BEAM_V10)
#define LORA_MODULE_SX1276 0
#elif defined(NAYAD_V1) || defined(NAYAD_V1R2)
#define LORA_MODULE_SX1262 1
#else 
#warning "LORA_MODULE not defined"
#endif

// long range values
// #define LM_CONFIG_LORASF 12U
// #define LM_CONFIG_POWER 17

// default values
// #define LM_CONFIG_LORASF 9U
// #define LM_CONFIG_POWER 2

// testbed values
#define LM_CONFIG_LORASF 7U
#define LM_CONFIG_POWER 2


#ifndef LORA_SCK
#if defined(NAYAD_V1)
#define LORA_SCK 14
#elif defined(NAYAD_V1R2)
#define LORA_SCK 18
#else 
#define LORA_SCK LORA_SCK
#endif
#endif

#ifndef LORA_MISO
#if defined(NAYAD_V1)
#define LORA_MISO 12
#elif defined(NAYAD_V1R2)
#define LORA_MISO 19
#else
#define LORA_MISO LORA_MISO
#endif
#endif

#ifndef LORA_MOSI
#if defined(NAYAD_V1)
#define LORA_MOSI 13
#elif defined(NAYAD_V1R2)
#define LORA_MOSI 23
#else
#define LORA_MOSI LORA_MOSI
#endif
#endif

#ifndef LORA_CS
#if defined(NAYAD_V1) || defined(NAYAD_V1R2)
#define LORA_CS 15
#else
#define LORA_CS 255U
#endif
#endif

#ifndef LORA_RST
#if defined(NAYAD_V1) || defined(NAYAD_V1R2)
#define LORA_RST 27
#else
#warning "LORA_RST not defined"
#define LORA_RST 255U
#endif
#endif

#ifndef LORA_IRQ
#if defined(NAYAD_V1)
#define LORA_IRQ 26
#elif defined(NAYAD_V1R2)
#define LORA_IRQ 26
#else
#define LORA_IRQ 255U
#endif
#endif

#ifndef LORA_IO1
#if defined(NAYAD_V1) 
#define LORA_IO1 33
#elif defined(NAYAD_V1R2)
#define LORA_IO1 33
#else
#ifndef LORA_MODULE_SX1276
#warning "LORA_IO1 not defined"
#endif
#define LORA_IO1 255U
#endif
#endif

// Simulation Configuration
// The address of the device that will connect at the beginning of the simulation
#define WIFI_ADDR_CONNECTED 0x5880

#define PACKET_COUNT 3
#define PACKET_DELAY 30000
#define PACKET_SIZE 100
#define UPLOAD_PAYLOAD 0
#define LOG_MESHER 0

// If defined, there only be one sender
#define ONE_SENDER 0

// If defined 0 the packets will be sent unreliably
#define SEND_RELIABLE 1

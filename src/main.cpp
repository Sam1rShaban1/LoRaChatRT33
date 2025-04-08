#include <Arduino.h>

#include "config.h"
#include "esp_log.h"
#include "esp32-hal-log.h"
#include "message/messageManager.h"
#include "loramesh/loraMeshService.h"
#include <ArduinoJson.h>

static const char* TAG = "Main";

#pragma region WiFi
#ifdef WIFI_ENABLED
#include "wifi/wifiServerService.h"
WiFiServerService& wiFiService = WiFiServerService::getInstance();
void initWiFi() {
    wiFiService.initWiFi();
}
#endif
#pragma endregion

#ifdef BLUETOOTH_ENABLED
#pragma region SerialBT
#include "bluetooth/bluetoothService.h"
BluetoothService& bluetoothService = BluetoothService::getInstance();
void initBluetooth() {
    bluetoothService.initBluetooth(String(loraMeshService.getLocalAddress(), HEX));
}
#endif
#pragma endregion

#pragma region LoRaMesher
LoRaMeshService& loraMeshService = LoRaMeshService::getInstance();
void initLoRaMesher() {
    loraMeshService.initLoraMesherService();
}
#pragma endregion

#pragma region MQTT
#ifdef MQTT_ENABLED
#include "mqtt/mqttService.h"
MqttService& mqttService = MqttService::getInstance();
void initMQTT() {
    mqttService.initMqtt(String(loraMeshService.getLocalAddress()));
}
#endif
#pragma endregion

#pragma region MQTT_MON
#ifdef MQTT_MON_ENABLED
#include "monitor/monService.h"
MonService& mon_mqttService = MonService::getInstance();
void init_mqtt_mon() {
    mon_mqttService.init();
}
#endif

#pragma region GPS
#ifdef GPS_ENABLED
#include "gps/gpsService.h"
GPSService& gpsService = GPSService::getInstance();
void initGPS() {
    gpsService.initGPS();
}
#endif
#pragma endregion

#pragma region Led
#ifdef LED_ENABLED
#include "led/led.h"
Led& led = Led::getInstance();
void initLed() {
    led.init();
}
#endif
#pragma endregion

#pragma region Metadata
#ifdef METADATA_ENABLED
#include "sensor/metadata/metadata.h"
Metadata& metadata = Metadata::getInstance();
void initMetadata() {
    metadata.initMetadata();
}
#endif
#pragma endregion

#pragma region Sensors
#ifdef SENSORS_ENABLED
#include "sensor/sensorService.h"
SensorService& sensorService = SensorService::getInstance();
void initSensors() {
    sensorService.init();
}
#endif
#pragma endregion

#pragma region Simulator
#ifdef SIMULATION_ENABLED
#include "simulator/sim.h"
Sim& simulator = Sim::getInstance();
void initSimulator() {
    simulator.init();
}
#endif
#pragma endregion

#pragma region Battery
#ifdef BATTERY_ENABLED
#include "battery/battery.h"
Battery& battery = Battery::getInstance();
void initBattery() {
    battery.init();
}
#endif
#pragma endregion

#include "XPowersAXP192.tpp"
#include "XPowersAXP2101.tpp"
#include "XPowersLibInterface.hpp"
#define PMU_IRQ 35
#define PMU_WIRE_PORT   Wire
XPowersLibInterface *PMU = NULL;
bool pmuInterrupt;
void setPmuFlag() { pmuInterrupt = true; }
bool initPMU() {
    if (!PMU) {
        PMU = new XPowersAXP2101(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Serial.println("Warning: Failed to find AXP2101 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Serial.println("AXP2101 PMU init succeeded, using AXP2101 PMU");
        }
    }
    if (!PMU) {
        PMU = new XPowersAXP192(PMU_WIRE_PORT);
        if (!PMU->init()) {
            Serial.println("Warning: Failed to find AXP192 power management");
            delete PMU;
            PMU = NULL;
        } else {
            Serial.println("AXP192 PMU init succeeded, using AXP192 PMU");
        }
    }
    if (!PMU) {
        return false;
    }
    pinMode(PMU_IRQ, INPUT_PULLUP);
    attachInterrupt(PMU_IRQ, setPmuFlag, FALLING);
    if (PMU->getChipModel() == XPOWERS_AXP192) {
        PMU->setProtectedChannel(XPOWERS_DCDC3);
        PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
        PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
        PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
        PMU->enablePowerOutput(XPOWERS_LDO2);
        PMU->enablePowerOutput(XPOWERS_LDO3);
        PMU->setProtectedChannel(XPOWERS_DCDC1);
        PMU->setProtectedChannel(XPOWERS_DCDC3);
        PMU->enablePowerOutput(XPOWERS_DCDC1);
        PMU->disablePowerOutput(XPOWERS_DCDC2);
        PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);
        PMU->enableIRQ(XPOWERS_AXP192_VBUS_REMOVE_IRQ |
                       XPOWERS_AXP192_VBUS_INSERT_IRQ |
                       XPOWERS_AXP192_BAT_CHG_DONE_IRQ |
                       XPOWERS_AXP192_BAT_CHG_START_IRQ |
                       XPOWERS_AXP192_BAT_REMOVE_IRQ |
                       XPOWERS_AXP192_BAT_INSERT_IRQ |
                       XPOWERS_AXP192_PKEY_SHORT_IRQ
                      );
    } else if (PMU->getChipModel() == XPOWERS_AXP2101) {
        PMU->disablePowerOutput(XPOWERS_DCDC2);
        PMU->disablePowerOutput(XPOWERS_DCDC3);
        PMU->disablePowerOutput(XPOWERS_DCDC4);
        PMU->disablePowerOutput(XPOWERS_DCDC5);
        PMU->disablePowerOutput(XPOWERS_ALDO1);
        PMU->disablePowerOutput(XPOWERS_ALDO4);
        PMU->disablePowerOutput(XPOWERS_BLDO1);
        PMU->disablePowerOutput(XPOWERS_BLDO2);
        PMU->disablePowerOutput(XPOWERS_DLDO1);
        PMU->disablePowerOutput(XPOWERS_DLDO2);
        PMU->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
        PMU->enablePowerOutput(XPOWERS_VBACKUP);
        PMU->setProtectedChannel(XPOWERS_DCDC1);
        PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO2);
        PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
        PMU->enablePowerOutput(XPOWERS_ALDO3);
    }
    PMU->enableSystemVoltageMeasure();
    PMU->enableVbusVoltageMeasure();
    PMU->enableBattVoltageMeasure();
    PMU->disableTSPinMeasure();
    Serial.printf("=========================================\n");
    if (PMU->isChannelAvailable(XPOWERS_DCDC1)) { Serial.printf("DC1  : %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC1) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_DCDC1)); }
    if (PMU->isChannelAvailable(XPOWERS_DCDC2)) { Serial.printf("DC2  : %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC2) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_DCDC2)); }
    if (PMU->isChannelAvailable(XPOWERS_DCDC3)) { Serial.printf("DC3  : %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC3) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_DCDC3)); }
    if (PMU->isChannelAvailable(XPOWERS_DCDC4)) { Serial.printf("DC4  : %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC4) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_DCDC4)); }
    if (PMU->isChannelAvailable(XPOWERS_DCDC5)) { Serial.printf("DC5  : %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC5) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_DCDC5)); }
    if (PMU->isChannelAvailable(XPOWERS_LDO2)) { Serial.printf("LDO2 : %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_LDO2) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_LDO2)); }
    if (PMU->isChannelAvailable(XPOWERS_LDO3)) { Serial.printf("LDO3 : %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_LDO3) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_LDO3)); }
    if (PMU->isChannelAvailable(XPOWERS_ALDO1)) { Serial.printf("ALDO1: %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO1) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_ALDO1)); }
    if (PMU->isChannelAvailable(XPOWERS_ALDO2)) { Serial.printf("ALDO2: %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO2) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_ALDO2)); }
    if (PMU->isChannelAvailable(XPOWERS_ALDO3)) { Serial.printf("ALDO3: %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO3) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_ALDO3)); }
    if (PMU->isChannelAvailable(XPOWERS_ALDO4)) { Serial.printf("ALDO4: %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO4) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_ALDO4)); }
    if (PMU->isChannelAvailable(XPOWERS_BLDO1)) { Serial.printf("BLDO1: %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_BLDO1) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_BLDO1)); }
    if (PMU->isChannelAvailable(XPOWERS_BLDO2)) { Serial.printf("BLDO2: %s   Voltage: %04u mV \n", PMU->isPowerChannelEnable(XPOWERS_BLDO2) ? "+" : "-", PMU->getPowerChannelVoltage(XPOWERS_BLDO2)); }
    Serial.printf("=========================================\n");
    return true;
}

#ifdef DISPLAY_ENABLED
#include "display.h"
TaskHandle_t display_TaskHandle = NULL;
#define DISPLAY_TASK_DELAY 50 //ms
#define DISPLAY_LINE_TWO_DELAY 10000 //ms
#define DISPLAY_LINE_THREE_DELAY 50000 //ms
#define DISPLAY_LINE_FOUR_DELAY 20000 //ms
#define DISPLAY_LINE_FIVE_DELAY 10000 //ms
#define DISPLAY_LINE_SIX_DELAY 10000 //ms
#define DISPLAY_LINE_ONE 10000 //ms

void display_Task(void* pvParameters) {
    uint32_t lastLineOneUpdate = 0;
    uint32_t lastLineTwoUpdate = 0;
    uint32_t lastLineThreeUpdate = 0;
#ifdef GPS_ENABLED
    uint32_t lastGPSUpdate = 0;
#endif
    uint32_t lastLineFourUpdate = 0;
    uint32_t lastLineFiveUpdate = 0;
    uint32_t lastLineSixUpdate = 0;
    uint32_t lastLineSevenUpdate = 0;

    while (true) {
        if (millis() - lastLineOneUpdate > DISPLAY_LINE_ONE) {
            lastLineOneUpdate = millis();
            bool isConnected = wiFiService.isConnected() || loraMeshService.hasGateway();
            String lineOne = "LoRa 33-  " + String(isConnected ? "CON" : "NC");
            Screen.changeLineOne(lineOne);
        }
        if (millis() - lastLineTwoUpdate > DISPLAY_LINE_TWO_DELAY) {
            lastLineTwoUpdate = millis();
            String lineTwo = String(loraMeshService.getLocalAddress(), HEX);
            if (wiFiService.isConnected())
                lineTwo += " | " + wiFiService.getIP();
            Screen.changeLineTwo(lineTwo);
        }
#ifdef GPS_ENABLED
        if (millis() - lastGPSUpdate > UPDATE_GPS_DELAY) {
            lastGPSUpdate = millis();
            String lineThree = gpsService.getGPSUpdatedWait();
            if (lineThree.startsWith("G") != 1)
                Screen.changeLineThree(lineThree);
        }
#endif

        if (millis() - lastLineFourUpdate > DISPLAY_LINE_FOUR_DELAY) {
            lastLineFourUpdate = millis();
            String lineFour; // = "RoutingTable:  ";
            String rt = loraMeshService.getRoutingTable();
            if (1)
                lineFour += rt;
            Screen.changeLineFour(lineFour);
        }

        Screen.drawDisplay();
        vTaskDelay(DISPLAY_TASK_DELAY / portTICK_PERIOD_MS);
    }
}

void createUpdateDisplay() {
    int res = xTaskCreate(
        display_Task,
        "Display Task",
        2048,
        (void*) 1,
        2,
        &display_TaskHandle);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Display Task creation gave error: %d", res);
    }
}

void initDisplay() {
    Screen.initDisplay();
    createUpdateDisplay();
}
#endif

MessageManager& manager = MessageManager::getInstance();

void initManager() {
    manager.init();
    ESP_LOGV(TAG, "Manager initialized");
#ifdef BLUETOOTH_ENABLED
    manager.addMessageService(&bluetoothService);
    ESP_LOGV(TAG, "Bluetooth service added to manager");
#endif
    manager.addMessageService(&loraMeshService);
    ESP_LOGV(TAG, "LoRaMesher service added to manager");
#ifdef GPS_ENABLED
    manager.addMessageService(&gpsService);
    ESP_LOGV(TAG, "GPS service added to manager");
#endif
#ifdef WIFI_ENABLED
    manager.addMessageService(&wiFiService);
    ESP_LOGV(TAG, "WiFi service added to manager");
#endif
#ifdef MQTT_ENABLED
    manager.addMessageService(&mqttService);
    ESP_LOGV(TAG, "MQTT service added to manager");
#endif
#ifdef MQTT_MON_ENABLED
    manager.addMessageService(&mon_mqttService);
    ESP_LOGV(TAG, "MON-MQTT service added to manager");
#endif
#ifdef LED_ENABLED
    manager.addMessageService(&led);
    ESP_LOGV(TAG, "Led service added to manager");
#endif
#ifdef METADATA_ENABLED
    manager.addMessageService(&metadata);
    ESP_LOGV(TAG, "Metadata service added to manager");
#endif
#ifdef SENSORS_ENABLED
    manager.addMessageService(&sensorService);
    ESP_LOGV(TAG, "Sensors service added to manager");
#endif
#ifdef SIMULATION_ENABLED
    manager.addMessageService(&simulator);
#endif
#ifdef BATTERY_ENABLED
    manager.addMessageService(&battery);
    ESP_LOGV(TAG, "battery service added to manager");
#endif
    ESP_LOGV(TAG, "Simulator service added to manager");
    Serial.println(manager.getAvailableCommands());
}

void initWire() {
    Wire.begin((int) I2C_SDA, (int) I2C_SCL);
}

#ifndef PIO_UNIT_TESTING
void setup() {
    Serial.begin(115200);
    int rxPin = 13;
    int txPin = 14;
    long baudRate = 9600;
    ESP_LOGI(TAG, "Initializing Serial1 (RX:%d, TX:%d, Baud:%ld) for Sensor comms", rxPin, txPin, baudRate);
    Serial1.begin(baudRate, SERIAL_8N1, rxPin, txPin);

    ESP_LOGV(TAG, "Build environment name: %s", BUILD_ENV_NAME);
    initWire();
    ESP_LOGV(TAG, "Heap before initManager: %d", ESP.getFreeHeap());
    initManager();
    ESP_LOGV(TAG, "Heap after initManager: %d", ESP.getFreeHeap());
    ESP_LOGV(TAG, "Init PMU") ;
    if (initPMU()) {
      ESP_LOGV(TAG, "Init PMU OK") ;
    } else {
      ESP_LOGV(TAG, "Init PMU FAILED") ;
    }
#ifdef WIFI_ENABLED
    initWiFi();
    ESP_LOGV(TAG, "Heap after initWiFi: %d", ESP.getFreeHeap());
#endif
#ifdef BLUETOOTH_ENABLED
    initBluetooth();
    ESP_LOGV(TAG, "Heap after initBluetooth: %d", ESP.getFreeHeap());
#endif
    initLoRaMesher();
    ESP_LOGV(TAG, "Heap after initLoRaMesher: %d", ESP.getFreeHeap());
#ifdef MQTT_ENABLED
    initMQTT();
    ESP_LOGV(TAG, "Heap after initMQTT: %d", ESP.getFreeHeap());
#endif
#ifdef MQTT_MON_ENABLED
    init_mqtt_mon();
    ESP_LOGV(TAG, "Heap after init_mqtt_mon: %d", ESP.getFreeHeap());
#endif
#ifdef GPS_ENABLED
    initGPS();
    ESP_LOGV(TAG, "Heap after initGPS: %d", ESP.getFreeHeap());
#endif
#ifdef LED_ENABLED
    initLed();
#endif
#ifdef METADATA_ENABLED
    initMetadata();
    ESP_LOGV(TAG, "Heap after initMetadata: %d", ESP.getFreeHeap());
#endif
#ifdef SENSORS_ENABLED
    initSensors();
    ESP_LOGV(TAG, "Heap after init Sensors: %d", ESP.getFreeHeap());
#endif
#ifdef SIMULATION_ENABLED
    initSimulator();
#endif
#ifdef BATTERY_ENABLED
    initBattery();
#endif
#ifdef DISPLAY_ENABLED
    initDisplay();
    ESP_LOGV(TAG, "Heap after initDisplay: %d", ESP.getFreeHeap());
#endif
    ESP_LOGV(TAG, "Setup finished");
#ifdef LED_ENABLED
    led.ledBlink();
#endif
}

void loop() {
    vTaskDelay(200000 / portTICK_PERIOD_MS);

    Serial.printf("FREE HEAP: %d\n", ESP.getFreeHeap());
    Serial.printf("Min, Max: %d, %d\n", ESP.getMinFreeHeap(), ESP.getMaxAllocHeap());

#ifdef BATTERY_ENABLED
    if (battery.getVoltagePercentage() < 20) {
        ESP_LOGE(TAG, "Battery is low, deep sleeping for %d s", DEEP_SLEEP_TIME);
        #ifdef MQTT_ENABLED
        mqttService.disconnect();
        #endif
        #ifdef WIFI_ENABLED
        wiFiService.disconnectWiFi();
        esp_wifi_deinit();
        #endif
        ESP.deepSleep(DEEP_SLEEP_TIME * (uint32_t) 1000000);
    }
#endif

    if (ESP.getFreeHeap() < 40000) {
        ESP_LOGE(TAG, "Not enough memory (%d bytes), restarting device!", ESP.getFreeHeap());
        ESP.restart();
    }
}

#endif
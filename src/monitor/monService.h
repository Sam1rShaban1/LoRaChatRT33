#pragma once
#include "LoraMesher.h"
#include "config.h"
#include "message/messageManager.h"
#include "message/messageService.h"
#include "monCommandService.h"
#include "monServiceMessage.h"
#include <Arduino.h>
#include <cstdint>

#define MON_MQTT_ONE_MESSAGE

class MonService : public MessageService {
public:
  static MonService &getInstance() {
    static MonService instance;
    return instance;
  }
  void init();
  monCommandService *monCommandService_ = new monCommandService();
  void getJSON(DataMessage *message, String &json) ;
  DataMessage *getDataMessage(JsonObject data);
  void processReceivedMessage(messagePort port, DataMessage *message);
private:
  MonService() : MessageService(MonApp, "Mon") {
    commandService = monCommandService_;
    currentSensorJsonData ="{}"; // Initialize sensor data string
    running = false;
    isCreated = false;
    monMessageId = 0;
  };
  void createSendingTask();
#if defined(MON_MQTT_ONE_MESSAGE)
  static void sendingLoopOneMessage(void *pvParameters); // Accept parameter
  monOneMessage *createMONPayloadMessage(int number_of_neighbors) ;
#else
  static void sendingLoop(void *pvParameters); // Accept parameter
  void createAndSendMessage(uint16_t mcount, RouteNode *);
#endif
  TaskHandle_t sending_TaskHandle = NULL;
  bool running;
  bool isCreated;
  size_t monMessageId;
  String currentSensorJsonData; // Added member to store sensor data
};
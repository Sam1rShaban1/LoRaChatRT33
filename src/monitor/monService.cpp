#include <Arduino.h>
#include "monServiceMessage.h"
#include "monService.h"
#include "loramesh/loraMeshService.h"
#include "LoraMesher.h"
#include "gps/gpsService.h"
#include "mqtt/mqttService.h"
#include "message/messageManager.h" // Include MessageManager

#if defined(MON_MQTT_ONE_MESSAGE)
static const char *MON_TAG = "MonOMService";
#else
static const char *MON_TAG = "MonService";
#endif

void MonService::init() {
  ESP_LOGI(MON_TAG, "Initializing monitor service");
  createSendingTask();
}

void MonService::getJSON(DataMessage *message, String &json) {
  monMessage *bm = (monMessage *)message;
  StaticJsonDocument<3072> doc;
  JsonObject data = doc.createNestedObject("RT") ;

#if defined(MON_MQTT_ONE_MESSAGE)
    ESP_LOGD(MON_TAG, "getJSON: Serializing monOneMessage (ID: %d)", message->messageId);
    monOneMessage *mon = (monOneMessage *)message ;
    mon->serialize(data); // Assumes monOneMessage::serialize is correct
    ESP_LOGD(MON_TAG, "getJSON: monOneMessage serialization complete.");
#else
  // Legacy path - keep for completeness if MON_MQTT_ONE_MESSAGE might be undefined
  if ((bm->RTcount == MONCOUNT_MONONEMESSAGE) || (bm->messageSize != 17)) {
    ESP_LOGD(MON_TAG, "getJSON: Serializing monOneMessage (ID: %d)", message->messageId);
    monOneMessage *mon = (monOneMessage *)message ;
    mon->serialize(data);
     ESP_LOGD(MON_TAG, "getJSON: monOneMessage serialization complete.");
  } else {
    ESP_LOGI(MON_TAG, "getJSON: monMessage->serialize"); // Legacy path
    bm->serialize(data);
  }
#endif

  serializeJson(doc, json);
  ESP_LOGD(MON_TAG, "getJSON: Final JSON string generated (first 100 chars): %.100s", json.c_str());
}

DataMessage *MonService::getDataMessage(JsonObject data) {
  ESP_LOGI(MON_TAG, "getDataMessage");

#if defined(MON_MQTT_ONE_MESSAGE)
    ESP_LOGI(MON_TAG, "getDataMessage: Creating monOneMessage");
    int num_neighbors = 0;
    // Use V7+ syntax for checking
    if(data["number_of_neighbors"].is<int>()) {
        num_neighbors = data["number_of_neighbors"].as<int>();
    }
    uint32_t calculated_messageSize = sizeof(monOneMessage) + sizeof(routing_entry) * num_neighbors;
    uint32_t payloadSize = calculated_messageSize - sizeof(DataMessageGeneric);

    monOneMessage* mon = (monOneMessage*) pvPortMalloc(calculated_messageSize);
    if (!mon) {
         ESP_LOGE(MON_TAG, "Failed to allocate memory for monOneMessage in getDataMessage!");
         return nullptr;
    }
    new (mon) monOneMessage(); // Placement new to initialize members
    mon->deserialize(data);    // Assumes deserialize handles sensorDataJson
    mon->messageSize = payloadSize;
    return ((DataMessage *)mon);
#else
    // Handle legacy path if MON_MQTT_ONE_MESSAGE is not defined
    if(data.containsKey("RTcount") && data["RTcount"] == MONCOUNT_MONONEMESSAGE) {
     ESP_LOGI(MON_TAG, "getDataMessage: Creating monOneMessage");
     // ... Allocation/deserialization for monOneMessage ...
     return ((DataMessage *)mon);
    } else {
      ESP_LOGI(MON_TAG, "getDataMessage: monMessage (Legacy format)");
      monMessage *mon = new monMessage();
      if (!mon) {
          ESP_LOGE(MON_TAG, "Failed to allocate memory for monMessage!");
          return nullptr;
      }
      mon->deserialize(data);
      mon->messageSize = sizeof(monMessage) - sizeof(DataMessageGeneric);
      return ((DataMessage *)mon);
    }
#endif
}


void MonService::processReceivedMessage(messagePort port, DataMessage *message) {
  ESP_LOGI(MON_TAG, "Received mon data - processing not implemented for MonService.");
}

void MonService::createSendingTask() {
  if (isCreated) {
        ESP_LOGW(MON_TAG, "Sending task already created.");
        return;
  }
  BaseType_t res = xTaskCreatePinnedToCore(
#if defined(MON_MQTT_ONE_MESSAGE)
      sendingLoopOneMessage,
#else
      sendingLoop, // Keep legacy path target if needed
#endif
      "MonSendingTask",
      6000,
      (void*) this,
      1,
      &sending_TaskHandle,
      0);

  if (res != pdPASS) {
    ESP_LOGE(MON_TAG, "Mon Sending task creation failed! Error code: %d", res);
  } else {
    ESP_LOGI(MON_TAG, "Mon Sending task created successfully.");
    running = true;
    isCreated = true;
  }
}

#if defined(MON_MQTT_ONE_MESSAGE)

monOneMessage* MonService::createMONPayloadMessage(int number_of_neighbors) {
    uint32_t messageStructSize = sizeof(monOneMessage) + sizeof(routing_entry) * number_of_neighbors;
    uint32_t payloadSize = messageStructSize - sizeof(DataMessageGeneric);

    monOneMessage* MONMessage = (monOneMessage*) pvPortMalloc(messageStructSize);

    if (!MONMessage) {
        ESP_LOGE(MON_TAG, "Failed to allocate memory for monOneMessage (size: %d)!", messageStructSize);
        return nullptr;
    }

    new (MONMessage) monOneMessage(); // Placement new initializes defaults

    MONMessage->messageSize = payloadSize;
    MONMessage->RTcount = MONCOUNT_MONONEMESSAGE; // Keep if needed
    MONMessage->uptime = millis();
    MONMessage->TxQ = LoraMesher::getInstance().getSendQueueSize();
    MONMessage->RxQ = LoraMesher::getInstance().getReceivedQueueSize();
    MONMessage->number_of_neighbors = number_of_neighbors;

    #if defined(LORAMESHER_BMX)
    MONMessage->routingTableId = RoutingTableService::routingTableId;
    #else
    MONMessage->routingTableId = 0;
    #endif

    MONMessage->appPortDst = appPort::MQTTApp;
    MONMessage->appPortSrc = appPort::MonApp;
    MONMessage->addrSrc = LoraMesher::getInstance().getLocalAddress();
    MONMessage->addrDst = 0;
    // MONMessage->messageId = monMessageId; // Assign ID just before sending

    #ifdef GPS_ENABLED
    if (GPSService::getInstance().isGPSValid()) {
        MONMessage->gpsData = GPSService::getInstance().getGPSMessage();
        ESP_LOGD(MON_TAG, "Fetched valid GPS data for monitoring message.");
    } else {
        ESP_LOGW(MON_TAG, "GPS data is currently invalid.");
        // GPSMessage constructor should set defaults
    }
    #else
    ESP_LOGV(MON_TAG, "GPS Service disabled.");
    #endif

    return MONMessage;
}

void MonService::sendingLoopOneMessage(void *parameter) {
  MonService *monServiceInstance = (MonService *)parameter;
  UBaseType_t uxHighWaterMark;
  ESP_LOGI(MON_TAG, "sendingLoopOneMessage: Top of loop.");
  static String uartInputBuffer = "";
  const int MAX_UART_BUFFER_SIZE = 2048;
  while (true) {
    if (!monServiceInstance->running) {
      ESP_LOGI(MON_TAG, "Wait notification to start the task");
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      ESP_LOGI(MON_TAG, "received notification to start the task");
    } else {
      // --- Read Serial1 Data with NULL filtering ---
      while (Serial1.available()) {
          char incomingByte = Serial1.read();
          // *** Filter out NULL bytes specifically ***
          if (incomingByte == '\0') {
              ESP_LOGW(MON_TAG, "NULL byte received from UART, discarding.");
              continue; // Skip NULL bytes
          }

          if (incomingByte == '\n') {
              uartInputBuffer.trim();
              if (uartInputBuffer.length() > 0) {
                  // Assign the cleaned buffer
                  monServiceInstance->currentSensorJsonData = uartInputBuffer;
                  ESP_LOGD(MON_TAG, "Stored CLEANED UART data (len %d): %s", uartInputBuffer.length(), monServiceInstance->currentSensorJsonData.c_str());
              } else {
                  ESP_LOGV(MON_TAG, "Received empty line from UART.");
              }
              uartInputBuffer = ""; // Reset buffer
          } else if (incomingByte >= 32) { // Accept printable characters
              if (uartInputBuffer.length() < MAX_UART_BUFFER_SIZE) {
                  uartInputBuffer += incomingByte;
              } else {
                  ESP_LOGW(MON_TAG, "UART input buffer overflow, discarding data. Resetting buffer.");
                  uartInputBuffer = "";
              }
          }
          // Other non-printable characters (besides NULL and \n) are ignored implicitly
      }
      // --- End Reading Serial1 ---

      uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
      ESP_LOGD(MON_TAG, "Stack HWM before RT processing: %d", uxHighWaterMark);

      RoutingTableService::printRoutingTable();
      LM_LinkedList<RouteNode>* routingTableList = LoRaMeshService::getInstance().radio.routingTableListCopy();
      if (routingTableList) {
            uint16_t neighborCount = 0;
            routingTableList->setInUse();
            if (routingTableList->moveToStart()) {
                do {
                    RouteNode *rtn = routingTableList->getCurrent();
                    if (rtn && rtn->networkNode.address == rtn->via) {
                        neighborCount++;
                    }
                } while (routingTableList->next());
                 ESP_LOGI(MON_TAG, "Found %d neighbors.", neighborCount);
            } else {
                 ESP_LOGD(MON_TAG, "Routing table copy is empty.");
            }

            monServiceInstance->monMessageId++;
            monOneMessage *MONMessage = monServiceInstance->createMONPayloadMessage(neighborCount);
            ESP_LOGD(MON_TAG, "createMONPayloadMessage returned: 0x%X", (uint32_t)MONMessage);

            if (MONMessage) {
                MONMessage->messageId = monServiceInstance->monMessageId;

                if (neighborCount > 0) {
                    routingTableList->moveToStart();
                    int i = 0;
                    do {
                        RouteNode *rtn = routingTableList->getCurrent();
                        if (rtn && rtn->networkNode.address == rtn->via && i < neighborCount) {
                            MONMessage->rt[i++] = {
                            rtn->networkNode.address,
                            rtn->receivedSNR,
                            rtn->SRTT,
                            rtn->networkNode.metric
                            };
                        } else if (rtn && rtn->networkNode.address == rtn->via && i >= neighborCount) {
                            ESP_LOGW(MON_TAG, "Neighbor count mismatch during RT population! Expected %d, got more.", neighborCount);
                            break;
                        }
                    } while (routingTableList->next());
                }

                // Assign the potentially cleaned sensor data
                MONMessage->sensorDataJson = monServiceInstance->currentSensorJsonData;
                ESP_LOGD(MON_TAG, "Sensor Data assigned to MONMessage: '%s'", MONMessage->sensorDataJson.c_str());

                ESP_LOGI(MON_TAG, "About to call MessageManager::getInstance().sendMessage(messagePort::MqttPort, ...); ID: %d", MONMessage->messageId);
                MessageManager::getInstance().sendMessage(messagePort::MqttPort, (DataMessage *)MONMessage);

                // Memory MUST be freed by MM/MQTTService. Do NOT free here.
                ESP_LOGW(MON_TAG, "Memory Warning: Ensure MONMessage (ID: %d) is freed by MM or MQTT!", MONMessage->messageId);
            } else {
                ESP_LOGE(MON_TAG, "Failed to create MONPayloadMessage, skipping send.");
                 monServiceInstance->monMessageId--;
            }
            routingTableList->releaseInUse();
            delete routingTableList;
            routingTableList = nullptr;
      } else {
            ESP_LOGE(MON_TAG, "Failed to get routing table copy.");
      }
      ESP_LOGD(MON_TAG, "Free heap at end of loop: %d", esp_get_free_heap_size());
      vTaskDelay(MON_SENDING_EVERY / portTICK_PERIOD_MS);
    }
  }
}

#else // --- Code for the legacy monMessage format ---

void MonService::sendingLoop(void *parameter) {
  MonService &monService = MonService::getInstance();
  UBaseType_t uxHighWaterMark;
  ESP_LOGI(MON_TAG, "entering sendingLoop (Legacy Format)");
  while (true) {
    if (!monService.running) {
      ESP_LOGI(MON_TAG, "Wait notification to start the task (Legacy Format)");
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      ESP_LOGI(MON_TAG, "received notification to start the task (Legacy Format)");
    } else {
      uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
      ESP_LOGD(MON_TAG, "Stack space unused after entering the task: %d (Legacy Format)", uxHighWaterMark);

      RoutingTableService::printRoutingTable();
      LM_LinkedList<RouteNode>* routingTableList = LoRaMeshService::getInstance().radio.routingTableListCopy();

      if (routingTableList) {
            routingTableList->setInUse();
            if (routingTableList->moveToStart()) {
                monService.monMessageId++;
                uint16_t monMessagecount = 0 ;
                do {
                RouteNode *rtn = routingTableList->getCurrent() ;
                if (rtn) {
                     monService.createAndSendMessage(++monMessagecount, rtn);
                }
                } while (routingTableList->next());
            }
            else {
                ESP_LOGD(MON_TAG, "No routes (Legacy Format)") ;
            }
            routingTableList->releaseInUse();
            delete routingTableList; // Delete the copy
            routingTableList = nullptr;
       } else {
            ESP_LOGE(MON_TAG, "Failed to get routing table copy (Legacy Format).");
       }

      vTaskDelay(MON_SENDING_EVERY / portTICK_PERIOD_MS);
      ESP_LOGD(MON_TAG, "Free heap: %d (Legacy Format)", esp_get_free_heap_size());
    }
  }
}

void MonService::createAndSendMessage(uint16_t mcount, RouteNode *rtn) {
  ESP_LOGV(MON_TAG, "Sending mon data %d (Legacy Format)", this->monMessageId) ;
  monMessage *message = new monMessage();
  if (!message) {
      ESP_LOGE(MON_TAG, "Failed to allocate memory for monMessage (Legacy Format)!");
      return;
  }

  message->appPortDst = appPort::MQTTApp;
  message->appPortSrc = appPort::MonApp;
  message->messageId = this->monMessageId ; // Use current ID
  message->addrSrc = LoraMesher::getInstance().getLocalAddress();
  message->addrDst = 0;

  message->RTcount = mcount ;
  message->address = rtn->networkNode.address ;
  message->metric = rtn->networkNode.metric ;
  message->via = rtn->via ;
  message->receivedSNR = rtn->receivedSNR ;
  message->sentSNR = rtn->sentSNR ;
  message->SRTT = rtn->SRTT ;
  message->RTTVAR = rtn->RTTVAR ;

  message->messageSize = sizeof(monMessage) - sizeof(DataMessageGeneric);
  MessageManager::getInstance().sendMessage(messagePort::MqttPort, (DataMessage *)message);

  ESP_LOGW(MON_TAG, "Memory Warning: Ensure monMessage (Legacy ID: %d) allocated with 'new' is deleted by MM or MQTT!", message->messageId);
}

#endif
#include <Arduino.h>
#include "monServiceMessage.h"
#include "monService.h"
#include "loramesh/loraMeshService.h"
#include "LoraMesher.h"
#include "gps/gpsService.h" // Include if using GPS data
#include "mqtt/mqttService.h" // Include if interacting directly
#include "message/messageManager.h" // Needed for sendMessage

#if defined(MON_MQTT_ONE_MESSAGE)
static const char *MON_TAG = "MonOMService";
#else
static const char *MON_TAG = "MonService";
#endif

void MonService::init() {
  ESP_LOGI(MON_TAG, "Initializing mqtt_mon");
  createSendingTask();
}

void MonService::getJSON(DataMessage *message, String &json) {
  monMessage *bm = (monMessage *)message;
  StaticJsonDocument<3072> doc; // Increased size slightly
  JsonObject data = doc.createNestedObject("RT") ;
  if ((bm->RTcount == MONCOUNT_MONONEMESSAGE) || (bm->messageSize != 17)) {
    ESP_LOGI(MON_TAG, "getJSON: monOneMessage->serialize");
    monOneMessage *mon = (monOneMessage *)message ;
    mon->serialize(data); // Assumes monOneMessage::serialize handles sensorData
  } else {
    ESP_LOGI(MON_TAG, "getJSON: monMessage->serialize");
    bm->serialize(data);
  }
  serializeJson(doc, json);
}

DataMessage *MonService::getDataMessage(JsonObject data) {
  ESP_LOGI(MON_TAG, "getDataMessage");
  if(data.containsKey("RTcount") && data["RTcount"] == MONCOUNT_MONONEMESSAGE) {
    ESP_LOGI(MON_TAG, "getDataMessage: monOneMessage");
    int num_neighbors = 0;
    if(data.containsKey("number_of_neighbors")) {
        num_neighbors = data["number_of_neighbors"].as<int>();
    }
    uint32_t calculated_messageSize = sizeof(monOneMessage) + sizeof(routing_entry) * num_neighbors;
    uint32_t payloadSize = calculated_messageSize - sizeof(DataMessageGeneric);
    monOneMessage* mon = (monOneMessage*) pvPortMalloc(calculated_messageSize);
    if (!mon) {
         ESP_LOGE(MON_TAG, "Failed to allocate memory for monOneMessage in getDataMessage!");
         return nullptr;
    }
    mon->deserialize(data); // Assumes deserialize correctly populates rt
    mon->messageSize = payloadSize;
    if (data.containsKey("sensorData")) { // Also deserialize sensor data if present
        mon->sensorDataJson = data["sensorData"].as<String>();
    }
    return ((DataMessage *)mon);
  }
  // Handle legacy monMessage
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

void MonService::processReceivedMessage(messagePort port, DataMessage *message) {
  ESP_LOGI(MON_TAG, "Received mon data - processing not implemented.");
  // Should not route to MQTT from here - MM handles that based on destination port
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
      sendingLoop,
#endif
      "MonSendingTask",
      6000,           // Stack size
      (void*) this,   // Pass instance pointer as parameter
      1,              // Priority
      &sending_TaskHandle,
      0);             // Core ID

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
    MONMessage->messageSize = payloadSize;
    MONMessage->RTcount = MONCOUNT_MONONEMESSAGE;
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
    MONMessage->messageId = monMessageId;
    MONMessage->sensorDataJson = "{}";

    #ifdef GPS_ENABLED
    // if (GPSService::getInstance().isGPSValid()) {
    //    MONMessage->gpsData = GPSService::getInstance().getGPSMessage();
    //    ESP_LOGD(MON_TAG, "Fetched valid GPS data.");
    // } else {
    //    ESP_LOGW(MON_TAG, "GPS data invalid.");
    // }
    #endif

    return MONMessage;
}

void MonService::sendingLoopOneMessage(void *parameter) {
  MonService *monServiceInstance = (MonService *)parameter; // Correct cast
  UBaseType_t uxHighWaterMark;
  static String uartInputBuffer = "";
  const int MAX_UART_BUFFER_SIZE = 2048;

  ESP_LOGI(MON_TAG, "sendingLoopOneMessage task started."); // Log task start

  while (true) {
    ESP_LOGI(MON_TAG, "sendingLoopOneMessage: Top of loop."); // Log loop iteration
    if (!monServiceInstance->running) {
      ESP_LOGI(MON_TAG, "Task pausing.");
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      ESP_LOGI(MON_TAG, "Task resumed.");
    } else {
      // --- Read Serial1 Data ---
      while (Serial1.available()) {
          char incomingByte = Serial1.read();
          if (incomingByte == '\n') {
              uartInputBuffer.trim();
              if (uartInputBuffer.length() > 0) {
                  monServiceInstance->currentSensorJsonData = uartInputBuffer;
                  ESP_LOGD(MON_TAG, "Stored UART data (len %d): %s", uartInputBuffer.length(), monServiceInstance->currentSensorJsonData.c_str());
              } else {
                  ESP_LOGV(MON_TAG, "Received empty line from UART.");
              }
              uartInputBuffer = "";
          } else if (incomingByte >= 32) {
              if (uartInputBuffer.length() < MAX_UART_BUFFER_SIZE) {
                  uartInputBuffer += incomingByte;
              } else {
                  ESP_LOGW(MON_TAG, "UART input buffer overflow, discarding data.");
                  uartInputBuffer = "";
              }
          }
      }
      // --- End Reading Serial1 ---

      uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
      ESP_LOGD(MON_TAG, "Stack HWM before RT processing: %d", uxHighWaterMark);

      RoutingTableService::printRoutingTable(); // Optional debug

      LM_LinkedList<RouteNode>* routingTableList = LoRaMeshService::getInstance().radio.routingTableListCopy();
      uint16_t neighborCount = 0; // Initialize neighbor count for this cycle

      if (routingTableList) {
            routingTableList->setInUse();
            if (routingTableList->moveToStart()) {
                do {
                    RouteNode *rtn = routingTableList->getCurrent();
                    if (rtn && rtn->networkNode.address == rtn->via) {
                        neighborCount++;
                    }
                } while (routingTableList->next());
            } else {
                ESP_LOGD(MON_TAG, "Routing table copy is empty.");
            }

            ESP_LOGI(MON_TAG, "sendingLoopOneMessage: Found %d neighbors.", neighborCount); // Log neighbor count

            if (neighborCount >= 0) {
                monServiceInstance->monMessageId++;
                monOneMessage *MONMessage = monServiceInstance->createMONPayloadMessage(neighborCount);
                ESP_LOGD(MON_TAG, "createMONPayloadMessage returned: %p", MONMessage); // Log allocation result

                if (MONMessage) {
                    routingTableList->moveToStart(); // Reset iterator
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

                    MONMessage->sensorDataJson = monServiceInstance->currentSensorJsonData;
                    ESP_LOGD(MON_TAG, "Added sensor data to monOneMessage: %s", MONMessage->sensorDataJson.c_str());

                    ESP_LOGI(MON_TAG, "About to call MessageManager::getInstance().sendMessage(messagePort::MqttPort, ...); ID: %d", MONMessage->messageId);
                    MessageManager::getInstance().sendMessage(messagePort::MqttPort, (DataMessage *)MONMessage);

                    // !!! MEMORY WARNING: MessageManager or MqttService MUST free MONMessage later !!!
                    ESP_LOGW(MON_TAG, "Memory Warning: Ensure MONMessage (ID: %d) is freed by MM or MQTT!", MONMessage->messageId);

                } else {
                    ESP_LOGE(MON_TAG, "Failed to create MONPayloadMessage! Skipping send.");
                }
            } else {
                 ESP_LOGI(MON_TAG, "Skipping MQTT send because neighborCount is 0.");
            }

            routingTableList->releaseInUse(); // Release the copy
            delete routingTableList;          // Delete the copy
            routingTableList = nullptr;       // Avoid dangling pointer

      } else {
            ESP_LOGE(MON_TAG, "Failed to get routing table copy.");
      }

      vTaskDelay(MON_SENDING_EVERY / portTICK_PERIOD_MS);
      ESP_LOGD(MON_TAG, "Free heap at end of loop: %d", esp_get_free_heap_size());
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
            } else {
                ESP_LOGD(MON_TAG, "No routes (Legacy Format)") ;
            }
            routingTableList->releaseInUse();
            delete routingTableList;
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
  message->messageId = this->monMessageId ;
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
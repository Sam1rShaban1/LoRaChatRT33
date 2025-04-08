#pragma once

#include <Arduino.h>
#include "message/dataMessage.h"
#include <ArduinoJson.h>
#include "gps/gpsMessage.h" // Ensure GPS struct definition is included

#define MONCOUNT_MONONEMESSAGE UINT16_MAX // Keep if used elsewhere, otherwise removable

#pragma pack(push, 1)

class monMessage: public DataMessageGeneric {
public:
  uint16_t RTcount = 0;
  uint16_t address = 0;
  uint16_t via = 0;
  uint8_t metric = 0;
  int8_t receivedSNR = 0;
  int8_t sentSNR = 0;
  unsigned long SRTT = 0;
  unsigned long RTTVAR = 0;

  void serialize(JsonObject &doc) {
    ((DataMessageGeneric *)(this))->serialize(doc);
    doc["RTcount"] = RTcount ;
    doc["address"] = address;
    doc["via"] = via;
    doc["metric"] = metric;
    doc["receivedSNR"] = receivedSNR;
    doc["sentSNR"] = sentSNR;
    doc["SRTT"] = SRTT;
    doc["RTTVAR"] = RTTVAR;
  }

  void deserialize(JsonObject &doc) {
    ((DataMessageGeneric *)(this))->deserialize(doc);
    if (doc["RTcount"].is<uint16_t>()) RTcount = doc["RTcount"];
    if (doc["address"].is<uint16_t>()) address = doc["address"];
    if (doc["via"].is<uint16_t>()) via = doc["via"];
    if (doc["metric"].is<uint8_t>()) metric = doc["metric"];
    if (doc["receivedSNR"].is<int8_t>()) receivedSNR = doc["receivedSNR"];
    if (doc["sentSNR"].is<int8_t>()) sentSNR = doc["sentSNR"];
    if (doc["SRTT"].is<unsigned long>()) SRTT = doc["SRTT"];
    if (doc["RTTVAR"].is<unsigned long>()) RTTVAR = doc["RTTVAR"];
  }
};

struct routing_entry {
  uint32_t neighbor ;
  int8_t RxSNR ;
  unsigned long SRTT ;
  uint8_t metric ;
};

class monOneMessage: public DataMessageGeneric {
public:
  uint16_t RTcount = MONCOUNT_MONONEMESSAGE ; // May no longer be needed if monMessage removed
  unsigned long uptime ;
  uint16_t TxQ ;
  uint16_t RxQ ;
  uint32_t number_of_neighbors ;
  uint8_t routingTableId ;
  String sensorDataJson = "{}"; // Default value constructor
  GPSMessage gpsData; // Ensure GPSMessage has default constructor

  routing_entry rt[] ;

  void operator delete(void *ptr) {
    ESP_LOGI("monOneMessage", "Custom delete operator called");
    vPortFree(ptr);
  };

  void serialize(JsonObject &doc) {
    ((DataMessageGeneric *)(this))->serialize(doc);
    doc["RTcount"] = RTcount ; // Keep for potential backward compatibility checks
    doc["uptime"] = uptime ;
    doc["TxQ"] = TxQ ;
    doc["RxQ"] = RxQ ;
    doc["number_of_neighbors"] = number_of_neighbors ;
    doc["routingTableId"] = routingTableId ;

    // Log value just before adding to JSON
    ESP_LOGD("monOneMessageSerialize", "Serializing sensorDataJson: '%s'", sensorDataJson.c_str());
    doc["sensorData"] = sensorDataJson;

    JsonObject gpsObj = doc["gps"].to<JsonObject>();
    gpsData.serialize(gpsObj);

    JsonArray rtArray = doc["rt"].to<JsonArray>();
    for (int i = 0; i < number_of_neighbors ; i++) {
      JsonObject entry = rtArray.add<JsonObject>();
      entry["neighbor"] = rt[i].neighbor ;
      entry["RxSNR"] = rt[i].RxSNR ;
      entry["SRTT"] = rt[i].SRTT ;
      entry["metric"] = rt[i].metric ;
    }
  }

  void deserialize(JsonObject &doc) {
    ((DataMessageGeneric *)(this))->deserialize(doc);
    if (doc["RTcount"].is<uint16_t>()) RTcount = doc["RTcount"] ;
    if (doc["uptime"].is<unsigned long>()) uptime = doc["uptime"] ;
    if (doc["TxQ"].is<uint16_t>()) TxQ = doc["TxQ"] ;
    if (doc["RxQ"].is<uint16_t>()) RxQ  = doc["RxQ"] ;
    if (doc["routingTableId"].is<uint8_t>()) routingTableId = doc["routingTableId"] ;
    if (doc["sensorData"].is<const char*>()) sensorDataJson = doc["sensorData"].as<String>();

    if (doc["gps"].is<JsonObjectConst>()) {
      JsonObjectConst gpsObj = doc["gps"];
      // gpsData.deserialize(gpsObj); // Assuming GPSMessage has deserialize
    }

    if (doc["number_of_neighbors"].is<uint32_t>()) {
      number_of_neighbors = doc["number_of_neighbors"];
      if (doc["rt"].is<JsonArrayConst>()) {
        JsonArrayConst rtArray = doc["rt"];
        int countToRead = rtArray.size() < number_of_neighbors ? rtArray.size() : number_of_neighbors;
         if (rtArray.size() != number_of_neighbors) {
            ESP_LOGW("monOneMessage", "Deserialize: number_of_neighbors (%d) mismatch with rt array size (%d)", number_of_neighbors, rtArray.size());
        }
        for (int i = 0; i < countToRead; i++) {
          JsonObjectConst entry = rtArray[i];
          if (entry["neighbor"].is<uint32_t>()) rt[i].neighbor = entry["neighbor"];
          if (entry["RxSNR"].is<int8_t>()) rt[i].RxSNR = entry["RxSNR"];
          if (entry["SRTT"].is<unsigned long>()) rt[i].SRTT = entry["SRTT"];
          if (entry["metric"].is<uint8_t>()) rt[i].metric = entry["metric"];
        }
      }
    } else {
      number_of_neighbors = 0;
    }
  }
};

#pragma pack(pop)
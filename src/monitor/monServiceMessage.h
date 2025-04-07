#pragma once

#include <Arduino.h>
#include "message/dataMessage.h"
#include <ArduinoJson.h>
// #include "gps/gpsMessage.h" // Include if GPSMessage struct is added

#define MONCOUNT_MONONEMESSAGE UINT16_MAX

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
    if (doc.containsKey("RTcount")) RTcount = doc["RTcount"];
    if (doc.containsKey("address")) address = doc["address"];
    if (doc.containsKey("via")) via = doc["via"];
    if (doc.containsKey("metric")) metric = doc["metric"];
    if (doc.containsKey("receivedSNR")) receivedSNR = doc["receivedSNR"];
    if (doc.containsKey("sentSNR")) sentSNR = doc["sentSNR"];
    if (doc.containsKey("SRTT")) SRTT = doc["SRTT"];
    if (doc.containsKey("RTTVAR")) RTTVAR = doc["RTTVAR"];
  }
};

struct routing_entry {
  uint32_t neighbor ;
  int8_t RxSNR ;
  unsigned long SRTT ;
  uint8_t metric ;
} ;

class monOneMessage: public DataMessageGeneric {
public:
  uint16_t RTcount = MONCOUNT_MONONEMESSAGE ;
  unsigned long uptime ;
  uint16_t TxQ ;
  uint16_t RxQ ;
  uint32_t number_of_neighbors ;
  uint8_t routingTableId ;
  String sensorDataJson = "{}";
  // GPSMessage gpsData; // Add this line if including structured GPS data

  routing_entry rt[] ;
  void operator delete(void *ptr) {
    ESP_LOGI("monOneMessage", "Custom delete operator called");
    vPortFree(ptr);
  };

  void serialize(JsonObject &doc) {
    ((DataMessageGeneric *)(this))->serialize(doc);
    doc["RTcount"] = RTcount ;
    doc["uptime"] = uptime ;
    doc["TxQ"] = TxQ ;
    doc["RxQ"] = RxQ ;
    doc["number_of_neighbors"] = number_of_neighbors ;
    doc["routingTableId"] = routingTableId ;
    doc["sensorData"] = sensorDataJson; // Serialize sensor data

    // Uncomment if adding structured GPS data
    // JsonObject gpsObj = doc.createNestedObject("gps");
    // gpsData.serialize(gpsObj);

    JsonArray rtArray = doc.createNestedArray("rt");
    for (int i = 0; i < number_of_neighbors ; i++) {
      JsonObject entry = rtArray.createNestedObject();
      entry["neighbor"] = rt[i].neighbor ;
      entry["RxSNR"] = rt[i].RxSNR ;
      entry["SRTT"] = rt[i].SRTT ;
      entry["metric"] = rt[i].metric ;
    }
  }

  void deserialize(JsonObject &doc) {
    ((DataMessageGeneric *)(this))->deserialize(doc);
    if (doc.containsKey("RTcount")) RTcount = doc["RTcount"] ;
    if (doc.containsKey("uptime")) uptime = doc["uptime"] ;
    if (doc.containsKey("TxQ")) TxQ = doc["TxQ"] ;
    if (doc.containsKey("RxQ")) RxQ  = doc["RxQ"] ;
    if (doc.containsKey("routingTableId")) routingTableId = doc["routingTableId"] ;
    if (doc.containsKey("sensorData")) sensorDataJson = doc["sensorData"].as<String>();

    // Uncomment if adding structured GPS data
    // if (doc.containsKey("gps")) {
    //   JsonObjectConst gpsObj = doc["gps"];
    //   gpsData.deserialize(gpsObj); // Assuming GPSMessage has deserialize
    // }

    if (doc.containsKey("number_of_neighbors")) {
      number_of_neighbors = doc["number_of_neighbors"];
      if (doc.containsKey("rt") && doc["rt"].is<JsonArrayConst>()) {
        JsonArrayConst rtArray = doc["rt"];
        int countToRead = rtArray.size() < number_of_neighbors ? rtArray.size() : number_of_neighbors;
         if (rtArray.size() != number_of_neighbors) {
            ESP_LOGW("monOneMessage", "Deserialize: number_of_neighbors (%d) mismatch with rt array size (%d)", number_of_neighbors, rtArray.size());
        }
        for (int i = 0; i < countToRead; i++) {
          JsonObjectConst entry = rtArray[i];
          if (entry.containsKey("neighbor")) rt[i].neighbor = entry["neighbor"];
          if (entry.containsKey("RxSNR")) rt[i].RxSNR = entry["RxSNR"];
          if (entry.containsKey("SRTT")) rt[i].SRTT = entry["SRTT"];
          if (entry.containsKey("metric")) rt[i].metric = entry["metric"];
        }
      }
    } else {
      number_of_neighbors = 0;
    }
  }
};

#pragma pack(pop)
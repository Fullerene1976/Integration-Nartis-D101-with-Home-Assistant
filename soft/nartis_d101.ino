/*************************************************************************
 * 
 *  ESP32 Nartis-D101 Reader
 *  Copyright (C) 2026 Mr. Jirandon (Dmitry Ponomarev)  
 *    
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *************************************************************************
 *
 *  This project steals some code from 
 *  https://github.com/lmcapacho/ESP32_LogicAnalyzer I2S DMA
 * 
 */ 

/*************************************************************************
 * To see in the Home Assistant panel the time when data was last received from Nartis-D101
 * in the configuration.yaml needs to add the following lines
 *
 *template:
 *  - sensor:
 *      - name: "Timestamp"
 *        unique_id: NartisTimestamp
 *        state: "{{ states.sensor.nartis_d101_total.last_updated }}"
 *        device_class: timestamp
 */

#include "nartis_capture.h"

NartisD101 nartis_d101;

#include <WiFi.h>
#include <PubSubClient.h>

// =========================
// CONFIG
// =========================
const char* ssid = "MY_OWN_SSID";
const char* password = "MY_OWN_PASSWORD";
const char* mqtt_server = "MY_OWN_MQTT_SERVER";
const char* mqtt_user = "mqtt";
const char* mqtt_pass = "mqtt";
const char* device_id = "nartis_d101";

// =========================
// MQTT
// =========================
WiFiClient espClient;
PubSubClient client(espClient);
MeterData meter;

// =========================
// WIFI
// =========================
bool connect_wifi(){

  Serial_Debug_Port.printf("Connecting to WiFi %s...", ssid);
  WiFi.begin(ssid,password);

  uint32_t start = millis();

  while(WiFi.status()!=WL_CONNECTED){

    delay(500);
    Serial_Debug_Port.print(".");

    if(millis() - start > 60000){
      Serial_Debug_Port.println("ERROR");
      return false;
    }
  }

  Serial_Debug_Port.print("OK with IP: ");
  Serial_Debug_Port.println(WiFi.localIP());

  return true;
}

// =========================
// MQTT
// =========================
bool connect_mqtt(){

  Serial_Debug_Port.printf("Connecting to MQTT Server %s...\n", mqtt_server);  

  uint32_t start = millis();

  while(!client.connected()){

    if(client.connect(device_id, mqtt_user, mqtt_pass)){
      Serial_Debug_Port.println("Connected to MQTT");
      return true;
    } else {
      Serial_Debug_Port.printf("MQTT connection FAILED, rc=%d retrying...\n", client.state());
      delay(500);
    }

    if (millis() - start > 30000){
      Serial_Debug_Port.println("MQTT connection FAILED (timeout)");
      return false;
    }
  }

  Serial_Debug_Port.println("Connected to MQTT");  
  return true;
}

// =========================
// ENSURE CONNECTION
// =========================
bool ensure_connection(){

  if (WiFi.status() != WL_CONNECTED) {
    if(!connect_wifi()) return false;
  }

  if (!client.connected()) {
    if(!connect_mqtt()) return false;
    //discovery();
  }

  return true;
}

// =========================
// MQTT DISCOVERY
// =========================
void discovery(){

  char topic[128],payload[512];

  const char* dev =
    "\"device\":{\"identifiers\":[\"nartis_d101\"],\"name\":\"Nartis D101\"}";

  auto send=[&](const char* name,const char* id,const char* unit,
                const char* dc,const char* sc,const char* vt){

    snprintf(topic,sizeof(topic),
      "homeassistant/sensor/%s/%s/config",device_id,id);

    snprintf(payload,sizeof(payload),
      "{\"name\":\"%s\",\"state_topic\":\"nartis/data\","
      "\"value_template\":\"%s\",\"unit_of_measurement\":\"%s\","
      "\"unique_id\":\"%s_%s\",\"device_class\":\"%s\","
      "\"state_class\":\"%s\",%s}",
      name,vt,unit,device_id,id,dc,sc,dev);

    //Correct data length check -> thanks to ChatGPT)
    //Serial.print("topic len = ");
    //Serial.println(strlen(topic));
    //Serial.print("payload len = ");
    //Serial.println(strlen(payload));
    
    bool ok;
    ok = client.publish(topic, payload, true);

    if (!ok) {
      Serial_Debug_Port.print("DISCOVERY -> ");
      Serial_Debug_Port.print(topic);
      Serial_Debug_Port.print(" : ");
      Serial_Debug_Port.println(ok ? "OK" : "FAIL");
    }  
  };

  send("Total","total","kWh","energy","total_increasing","{{ value_json.total }}");
  send("T1","t1","kWh","energy","total_increasing","{{ value_json.t1 }}");
  send("T2","t2","kWh","energy","total_increasing","{{ value_json.t2 }}");
  send("Voltage","voltage","V","voltage","measurement","{{ value_json.voltage }}");
  send("Current","current","A","current","measurement","{{ value_json.current }}");
  send("Power","power","W","power","measurement","{{ value_json.power }}");
  send("Freq","freq","Hz","frequency","measurement","{{ value_json.freq }}");
}

void print_meter_data(const MeterData& m)
{
    Serial_Debug_Port.println("=== Nartis-D101 collected data ===");

    Serial_Debug_Port.print("Total energy: ");
    Serial_Debug_Port.print(m.total, 3);
    Serial_Debug_Port.println(" kWh");

    Serial_Debug_Port.print("T1 energy:    ");
    Serial_Debug_Port.print(m.t1, 3);
    Serial_Debug_Port.println(" kWh");

    Serial_Debug_Port.print("T2 energy:    ");
    Serial_Debug_Port.print(m.t2, 3);
    Serial_Debug_Port.println(" kWh");

    Serial_Debug_Port.print("Voltage:      ");
    Serial_Debug_Port.print(m.voltage, 1);
    Serial_Debug_Port.println(" V");

    Serial_Debug_Port.print("Current:      ");
    Serial_Debug_Port.print(m.current, 3);
    Serial_Debug_Port.println(" A");

    Serial_Debug_Port.print("Power:        ");
    Serial_Debug_Port.print(m.power, 0);
    Serial_Debug_Port.println(" W");

    Serial_Debug_Port.print("Frequency:    ");
    Serial_Debug_Port.print(m.freq, 2);
    Serial_Debug_Port.println(" Hz");

    Serial_Debug_Port.println("==================================");
}

bool valid_meter_data(const MeterData& m) {
  
  //Serial_Debug_Port.println("T, T1 и Т2 check");  
  if (m.total < 0 || m.t1 < 0 || m.t2 < 0) return false;
  if (m.t1 + m.t2 > m.total + 5) return false;
  
  //Serial_Debug_Port.println("Voltage check");  
  if (m.voltage < 100 || m.voltage > 300) return false;
  
  //Serial_Debug_Port.println("Current check");  
  if (m.current < 0 || m.current > 100) return false;
  
  //Serial_Debug_Port.println("Power check");  
  if (m.power > 50000) return false;
  
  //Serial_Debug_Port.println("Frequency check");  
  if (m.freq < 45 || m.freq > 55) return false;

  //Serial_Debug_Port.println("Data validation complete");  

  return true;
}

void publish_mqtt(const MeterData& m)
{
    Serial_Debug_Port.println("Preparing data for MQTT");  
    
    char payload[256];

    snprintf(payload, sizeof(payload),
        "{\"total\": %.3f,"
        "\"t1\": %.3f,"
        "\"t2\": %.3f,"
        "\"voltage\": %.1f,"
        "\"current\": %.3f,"
        "\"power\": %.0f,"
        "\"freq\": %.2f}",
        m.total, m.t1, m.t2,
        m.voltage, m.current,
        m.power, m.freq
    );

    int n = snprintf(payload, sizeof(payload), 
        "{\"total\": %.3f,"
        "\"t1\": %.3f,"
        "\"t2\": %.3f,"
        "\"voltage\": %.1f,"
        "\"current\": %.3f,"
        "\"power\": %.0f,"
        "\"freq\": %.2f}",
        m.total, m.t1, m.t2,
        m.voltage, m.current,
        m.power, m.freq
        );

    Serial_Debug_Port.printf("snprintf=%d, payload_size=%d, strlen=%d\n", n, sizeof(payload), strlen(payload));

    if (n < 0 || n >= sizeof(payload)) {
      Serial_Debug_Port.println("PAYLOAD TRUNCATED!");
    } else {
      Serial_Debug_Port.println("Payload NOT truncated!");
    }
   
    bool ok;

    //Correct data length check
    //Serial.print("topic len = ");
    //Serial.println(strlen(topic));
    //Serial.print("payload len = ");
    //Serial.println(strlen(payload));

    Serial_Debug_Port.println("Sending data to MQTT");
    ok = client.publish("nartis/data", payload, true);

    uint32_t start = millis();
    while (millis() - start < 500) {
      client.loop();
      delay(10);
    }

    if (ok) {
      Serial_Debug_Port.printf("MQTT payload: %s\n", payload);
    } else {
      Serial_Debug_Port.print("Error sending data to MQTT!");
    }    
}

void IRAM_ATTR i2s_wrapper(void *arg)
{
  nartis_d101.i2s_isr(arg);
}

void setup(void) {  
  
  Serial_Debug_Port.begin(Serial_Debug_Port_Baud);
  
  //Pause for Serial port initialization
  delay(2000);

  Serial_Debug_Port.println("Starting Nartis-D101 integration with Home Assistant");

  client.setServer(mqtt_server, 1883);
  
  //MQTT buffer must be extended for DISCOVERY successiful sending
  client.setBufferSize(1024);
  connect_wifi();
  connect_mqtt();
  discovery();  
  nartis_d101.begin();

}

void loop()
{
  
  meter = nartis_d101.get_meter_data();
  print_meter_data(meter);

  if (valid_meter_data(meter)) {
    if(ensure_connection()){
      //client.loop()
      //delay(10);
      publish_mqtt(meter);    
    } else {
      Serial_Debug_Port.println("Cannot send data -> not connected to MQTT");
    }
  } else {
    Serial_Debug_Port.println("Received Nartis-D101 data not valid");
  }

  //Pause
  uint32_t start = millis();   

  Serial_Debug_Port.println("1 minute pause before rebooting ESP32");

  while(millis() - start < 60000){
    delay(5000);
    Serial_Debug_Port.print(".");
  }
  //Serial_Debug_Port.println();
  ESP.restart();
}  

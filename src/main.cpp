#include <Arduino.h>
#include "credentials.h"
#include <LittleFS.h>
#define SPIFFS LittleFS
#include <ESP8266WiFi.h>
#include <CertStoreBearSSL.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <IRutils.h>
#include <IRrecv.h>
#include <time.h>
#include <regex>
#include "Dio.h"
#include "uUtils.h"
#include "Debouncer.h"
#include "MQTT_CERT.h"

/* WiFi Credentials */
const char *ssid_sta = WIFI_SSID;
const char *password = WIFI_PASS;

/* MQTT Refs */
BearSSL::X509List cert(mqtt_cert);
BearSSL::WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

/* IR Refs */
decode_results ir_result;
IRrecv irrecv(IR_RECV_PIN);

/* Pin Refs */
Debouncer eventEmitter;
Dio dio;

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    if (!strcmp(topic, mqtt_topic_dio))
    {
        char req[length + 1];
        req[length] = '\0';
        strncpy(req, (char *)payload, length);
        u8 respCode = dio.write(req);
        if (respCode < 100)
        {
            String topic = String(mqtt_topic_dio);
            topic.replace("/req/", "/res/");
            mqttClient.publish(topic.c_str(), dio.read().c_str());
        }
        else if (respCode < 255)
        {
            eventEmitter.start();
        }
    }
}

void emittEvent()
{
    if (mqttClient.connected())
    {
        String topic = String(mqtt_topic_dio);
        topic.replace("/req/", "/res/");
        mqttClient.publish(topic.c_str(), dio.read().c_str());
    }
}

void setup()
{
    Serial.begin(9600);
    LittleFS.begin();
    dio.load("/dio.json");
    dio.setMode(OUTPUT);
    dio.write(HIGH);

    pinMode(LED_BUILTIN, OUTPUT);
    analogWrite(LED_BUILTIN, 254);
    irrecv.enableIRIn();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_sta, password);
    WiFi.setAutoReconnect(true);

    eventEmitter.setCallback(emittEvent, 500);
    wifiClient.setTrustAnchors(&cert);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

bool ledFlash = false;
void loop()
{
    // Infrared Remote
    if (irrecv.decode(&ir_result))
    {
        bool notify = true;
        switch (ir_result.value)
        {
        case IR_POWER:
            dio.write(HIGH);
            break;
        case IR_EQ:
            dio.flip(LOW);
            break;
        case IR_MODE:
            dio.flip();
            break;
        case IR_1:
            dio.flip(0);
            break;
        case IR_2:
            dio.flip(1);
            break;
        case IR_3:
            dio.flip(2);
            break;
        case IR_4:
            dio.flip(3);
            break;
        default:
            notify = false;
            break;
        }
        irrecv.resume();
        // Notify Mqtt;
        if (notify)
        {
            eventEmitter.start();
        }
    }

    // MQTT Subscribtions;
    if (mqttClient.connected())
    {
        mqttClient.loop();
    }
    else if (WiFi.isConnected())
    {
        configTime(5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
        if (mqttClient.connect(uuid("ESP8266JST-").c_str()))
        {
            mqttClient.subscribe(mqtt_topic_dio);
            analogWrite(LED_BUILTIN, 254);
            Serial.println("Subscribed!");
        }
    }
    else
    {
        analogWrite(LED_BUILTIN, ledFlash ? 254 : 255);
        ledFlash = !ledFlash;
    }

    eventEmitter.loop();
    delay(100);
}

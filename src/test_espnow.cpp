#ifdef ESP_RCTRL_TEST
#include <Helper.h>
#include <DigiOut.h>
#include <app_espnow.h>

DigiOut digiOut;
u8_t slaveMac[6];

void espnowCallback(u8_t *mac, u8_t *payload, u8_t len)
{
    DEBUG_LOG_LN("---------ESPNOW Data Recived---------")
    MsgPacketizer::feed(payload, len);
}

void onDigiOutEvent_(pkt_digiout_events_t event)
{
    DEBUG_LOG("DigiOut Event (");
    DEBUG_LOG(event.id);
    DEBUG_LOG(") ");
    DEBUG_LOG_LN(event.data);

    if (event.data == "[1,0,0,1]")
    {
        DEBUG_LOG_LN("Test Passed::OK");
    }
}

void onRegReq_(pkt_device_info_t devInfo)
{

    DEBUG_LOG("Got Registation Request: ");
    DEBUG_LOG_LN(devInfo.mac);

    pkt_gateway_status_t status = GATEWAY_STATUS_STD_CONNECTED;
    u8_t mac[6];
    str2mac(devInfo.mac.c_str(), mac);

    const auto &packet = MsgPacketizer::encode(PKT_GATEWAY_STATUS, status);
    esp_now_send(mac, (u8_t *)packet.data.data(), packet.data.size());

    delay(100);

    pkt_digiout_writes_t dData;
    dData.states = digiOut.reads();
    dData.trigger = true;
    const auto &packet1 = MsgPacketizer::encode(PKT_DIGIOUT_WRITES, dData);
    esp_now_send(mac, (u8_t *)packet1.data.data(), packet1.data.size());

    delay(100);

    pkt_digiout_write_t data;
    data.index = 0;
    data.state = 1;
    data.trigger = true;

    const auto &packet2 = MsgPacketizer::encode(PKT_DIGIOUT_WRITE, data);
    esp_now_send(mac, (u8_t *)packet2.data.data(), packet2.data.size());
}

void setup()
{
    Serial.begin(9600);
    WiFi.mode(WIFI_STA);
    Serial.println(WiFi.macAddress());
    if (esp_now_init() == 0)
    {
        esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
        esp_now_register_recv_cb(&espnowCallback);
        // MsgPacketizer::subscribe_manual(PKT_WIFI_BOOT, &booEventRecived);

        MsgPacketizer::subscribe_manual(PKT_REGISTER_DEVICE, &onRegReq_);
        MsgPacketizer::subscribe_manual(PKT_DIGIOUT_EVENTS, &onDigiOutEvent_);
        // MsgPacketizer::subscribe_manual(PKT_, &on_pkt_gateway_link_test);
        DEBUG_LOG_LN("ESPNOW Setup Complate!");
    }
    u8_t pins[4] = {5, 4, 14, 12};

    digiOut.load(pins, 4);
    digiOut.write(3, HIGH);
}

void loop(){};

#endif
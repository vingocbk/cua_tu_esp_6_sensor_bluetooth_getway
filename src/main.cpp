#include <Arduino.h>
#include "BluetoothSerial.h"
#include "ArduinoJson.h"
#include "Ticker.h"
#include "EEPROM.h"
#include <ESP32CAN.h>
#include <CAN_config.h>
#include "AppDebug.h"
#include "config.h"
#include "soc/soc.h"  //Brownout detector was triggered
#include "soc/rtc_cntl_reg.h"
CAN_device_t CAN_cfg;               // CAN Config
const int rx_queue_size = 10;       // Receive Queue size
BluetoothSerial SerialBT;


void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
	switch (event)
	{
	case ESP_SPP_SRV_OPEN_EVT:
		ECHOLN("Client Connected");
		break;
	case ESP_SPP_CLOSE_EVT:
		ECHOLN("Client Disconnected");
		break;
	case ESP_SPP_DATA_IND_EVT:
		if (param->data_ind.len < 512) {
			String data;
			for(int i = 0; i < param->data_ind.len; i++){
				data += (char)param->data_ind.data[i];
			}
			ECHO("data: ");
			ECHOLN(data);
			DynamicJsonDocument doc(1024);
			deserializeJson(doc, data);
			JsonObject obj = doc.as<JsonObject>();
			// String type = obj["sensor"].isNull();
			if(obj["type"].isNull())
			{
				String type = obj["type"].as<String>();
				if(type.equals("get_name"))
				{

				}
				else if(type.equals("change_rgb"))
				{

				}
				else if(type.equals("on_off_led"))
				{

				}
				else if(type.equals("change_alpha"))
				{

				}
				else if(type.equals("control"))
				{
					String data = obj["data"].as<String>();
					if(data.equals("open"))
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 2;
						// tx_frame.data.u8[0] = device_id[i];
						tx_frame.data.u8[1] = MSG_CONTROL_OPEN;
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					
				}
				else if(type.equals("mapping_name"))
				{

				}
				else if(type.equals("set_name"))
				{

				}
				else if(type.equals("set_all_device"))
				{

				}
				else if(type.equals("setting"))
				{

				}
			}
			else
			{
				ECHOLN("wrong format!");
			}
		}
	default:
        break;
	}
	
}


void setup() {
  // put your setup code here, to run once:
  	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    Serial.begin(115200);
    EEPROM.begin(512);

	CAN_cfg.speed = CAN_SPEED_125KBPS;
	CAN_cfg.tx_pin_id = GPIO_NUM_5;
	CAN_cfg.rx_pin_id = GPIO_NUM_4;
	CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
	// Init CAN Module
	ESP32Can.CANInit();

	SerialBT.flush();
    SerialBT.end(); 
    delay(10);
    if (!SerialBT.begin("Avy Cabinet")) {
        ECHOLN("An error occurred initializing Bluetooth");
        // digitalWrite(ledTestWifi, LOW);
    } else {
        ECHOLN("Bluetooth initialized: ");
        // ECHOLN(GetFullSSID().c_str());
    }
    SerialBT.register_callback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:
}
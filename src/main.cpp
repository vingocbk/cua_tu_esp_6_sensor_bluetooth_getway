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
int device_id_start, device_id_end;
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
void sendDeviceName();
void loadDataBegin();

void sendDeviceName()
{
	StaticJsonDocument<512> doc;
	String dataSend;
	
	doc["id_start"] = device_id_start;
  	doc["id_end"] = device_id_end;
	JsonArray dataName = doc.createNestedArray("name");
	for(int i = 0; i <= abs(device_id_end - device_id_start); i++)
	{
		ECHOLN(i);
		String name = "";
		for (int j = MSG_CHANGE_NAME_START+MSG_MAX_NAME_LENGTH*i; j < MSG_CHANGE_NAME_START + MSG_MAX_NAME_LENGTH*(i+1); j++){
			if(char(EEPROM.read(MSG_CHANGE_NAME_START+MSG_MAX_NAME_LENGTH*i)) == 0 || char(EEPROM.read(MSG_CHANGE_NAME_START+MSG_MAX_NAME_LENGTH*i) == 255))
			{
				name = String(device_id_start + i);
				break;
			}
			if(char(EEPROM.read(j)) == 255){
				break;
			}
			name += char(EEPROM.read(j));
		}
		dataName.add(name);
	}
	serializeJson(doc, dataSend);
	ECHOLN(dataSend);
    for(int i = 0; i<dataSend.length(); i++){
        SerialBT.write(dataSend[i]);
    }
}

void loadDataBegin()
{
	ECHOLN("Reading EEPROM Device ID START");
    device_id_start = EEPROM.read(EEPROM_BLUE_DEVICE_ID_START);
    ECHO("ID: ");
    ECHOLN(device_id_start);

    ECHOLN("Reading EEPROM Device ID END");
    device_id_end = EEPROM.read(EEPROM_BLUE_DEVICE_ID_END);
    ECHO("ID: ");
    ECHOLN(device_id_end);
}

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
			DynamicJsonDocument doc(RESPONSE_LENGTH);
			deserializeJson(doc, data);
			JsonObject obj = doc.as<JsonObject>();
			int device_id;
			if(!obj["id"].isNull())
			{
				device_id = obj["type"].as<int>();
			}
			if(!obj["type"].isNull())
			{
				String type = obj["type"].as<String>();
				if(type.equals("get_name"))
				{
					sendDeviceName();
				}
				else if(type.equals("change_rgb"))
				{
					JsonArray arrayRgb = obj["data"].as<JsonArray>();
					int controlled[3];
					for (int i = 0; i < 3; i++) { //Iterate through results
						controlled[i] = arrayRgb[i].as<int>();  //Implicit cast
						ECHOLN(controlled[i]);
					}
					CAN_frame_t tx_frame;
					tx_frame.FIR.B.FF = CAN_frame_std;
					tx_frame.MsgID = MSG_MASTER_ID;
					tx_frame.FIR.B.DLC = 5;
					tx_frame.data.u8[0] = device_id;
					tx_frame.data.u8[1] = MSG_CONTROL_LED_HAND;
					tx_frame.data.u8[2] = controlled[0];
					tx_frame.data.u8[3] = controlled[1];
					tx_frame.data.u8[4] = controlled[2];
					ESP32Can.CANWriteFrame(&tx_frame);
				}
				// else if(type.equals("on_off_led"))
				// {

				// }
				// else if(type.equals("change_alpha"))
				// {

				// }
				else if(type.equals("control"))
				{
					String data = obj["data"].as<String>();
					if(data.equals("open"))
					{
						
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 2;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_CONTROL_OPEN;
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					else if(data.equals("close"))
					{
						
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 2;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_CONTROL_CLOSE;
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					
				}
				else if(type.equals("mapping_name"))
				{
					int index = obj["index"].as<int>();
					String name = obj["name"].as<String>();
					ECHO("Wrote: ");
					for (int i = 0; i < name.length(); ++i){
						EEPROM.write(index*MSG_MAX_NAME_LENGTH+MSG_CHANGE_NAME_START + i, 255);             
					}
					for (int i = 0; i < name.length(); ++i){
						EEPROM.write(index*MSG_MAX_NAME_LENGTH+MSG_CHANGE_NAME_START + i, name[i]);             
						ECHO(name[i]);
					}
					ECHOLN("");
					EEPROM.commit();
					sendDeviceName();
				}
				else if(type.equals("set_id"))
				{
					int id = obj["id"].as<int>();
					CAN_frame_t tx_frame;
					tx_frame.FIR.B.FF = CAN_frame_std;
					tx_frame.MsgID = MSG_MASTER_ID;
					tx_frame.FIR.B.DLC = 2;
					tx_frame.data.u8[0] = MSG_SET_ID;
					tx_frame.data.u8[1] = id;
					ESP32Can.CANWriteFrame(&tx_frame);
				}
				else if(type.equals("set_all_device"))
				{
					int device_id_start = obj["id_start"].as<int>();
					int device_id_end = obj["id_end"].as<int>();
					ECHOLN("writing eeprom device id start:"); 
					ECHO("Wrote: ");
					EEPROM.write(EEPROM_BLUE_DEVICE_ID_START, device_id_start);
					ECHOLN(device_id_start);

					ECHOLN("writing eeprom device id end:");
					ECHO("Wrote: ");
					EEPROM.write(EEPROM_BLUE_DEVICE_ID_END, device_id_end);
					ECHOLN(device_id_end);
					EEPROM.commit();
				}
				else if(type.equals("setting"))
				{
					// 	0 - time return
					//  1 - set mode run
					//  2 - delay push (analog)
					//  3 - max value push
					//  4 - time auto close
					//  5 - percent slow speed in
					//  6 - percent slow speed out
					//  7 - min stop speed
					//  8 - reset distant
					JsonArray arrayData = obj["data"].as<JsonArray>();
					if(arrayData[0].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 3;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_TIME_RETURN;
						tx_frame.data.u8[2] = arrayData[0].as<int>();
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					if(arrayData[1].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
                            tx_frame.FIR.B.FF = CAN_frame_std;
                            tx_frame.MsgID = MSG_MASTER_ID;
                            tx_frame.FIR.B.DLC = 3;
                            tx_frame.data.u8[0] = device_id;
                            tx_frame.data.u8[1] = MSG_MODE_RUN;
                            tx_frame.data.u8[2] = arrayData[1].as<int>();
                            ESP32Can.CANWriteFrame(&tx_frame);
					}
					if(arrayData[2].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 3;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_DELAY_ANALOG;
						tx_frame.data.u8[2] = arrayData[2].as<int>();
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					if(arrayData[3].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 3;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_ERROR_ANALOG;
						tx_frame.data.u8[2] = arrayData[3].as<int>();
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					if(arrayData[4].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 3;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_AUTO_CLOSE;
						tx_frame.data.u8[2] = arrayData[4].as<int>();
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					if(arrayData[5].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 4;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_PERCENT_LOW;
						tx_frame.data.u8[2] = arrayData[5].as<int>();
						tx_frame.data.u8[3] = arrayData[8].as<int>();
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					if(arrayData[7].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 3;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_MIN_STOP_SPEED;
						tx_frame.data.u8[2] = arrayData[7].as<int>();
						ESP32Can.CANWriteFrame(&tx_frame);
					}
					if(arrayData[8].as<int>() != 0)
					{
						CAN_frame_t tx_frame;
						tx_frame.FIR.B.FF = CAN_frame_std;
						tx_frame.MsgID = MSG_MASTER_ID;
						tx_frame.FIR.B.DLC = 2;
						tx_frame.data.u8[0] = device_id;
						tx_frame.data.u8[1] = MSG_RESET_DISTANT;
						ESP32Can.CANWriteFrame(&tx_frame);
					}
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

	loadDataBegin();
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
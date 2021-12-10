#ifndef CONFIG_H
#define CONFIG_H

#define RESPONSE_LENGTH 512     //do dai data nhan ve tu tablet
#define EEPROM_BLUE_SSID_START 0
#define EEPROM_BLUE_SSID_END 32
#define EEPROM_BLUE_PASS_START 33
#define EEPROM_BLUE_PASS_END 64
#define EEPROM_BLUE_DEVICE_ID_START 65
#define EEPROM_BLUE_DEVICE_ID_END 66
#define EEPROM_BLUE_SERVER_START 67
#define EEPROM_BLUE_SERVER_END 128
#define EEPROM_BLUE_MAX_CLEAR 512

#define MSG_MASTER_ID         0
#define MSG_GET_STATUS        1
#define MSG_CONTROL_OPEN      2
#define MSG_CONTROL_CLOSE     3
#define MSG_CONTROL_STOP      4
#define MSG_CONTROL_LED_VOICE 5
#define MSG_CONTROL_LED_HAND  6
#define MSG_RESET_DISTANT     7
#define MSG_TIME_RETURN       8
#define MSG_MODE_RUN          9
#define MSG_PERCENT_LOW       10
#define MSG_DELAY_ANALOG      11
#define MSG_ERROR_ANALOG      12
#define MSG_AUTO_CLOSE        13
#define MSG_MIN_STOP_SPEED    14

#define MSG_SET_ID              100
#define MSG_CHANGE_NAME_START   101
#define MSG_MAX_NAME_LENGTH     16
#endif
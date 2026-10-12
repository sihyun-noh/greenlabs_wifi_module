#include <EEPROM.h>  // EEPROM.h는 AutoConnect.h 앞에 꼭 선언해야 함 그래야 컴파일 에러 안남
#include <EEPROM_def.h>
#include <global.h>

extern NEW_SERVER_INFO serverinfo;
extern NEW_SENSOR_INFO select_sensor_info;
int test = 0;

void eeprom_init()  ////adfa
{
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    //   ESP.restart();
  }
}

void each_save(int address, int value) {
  EEPROM.writeUInt(address, value);  // 60번부터
  EEPROM.commit();
}

int each_read(int address) {
  int resulte = EEPROM.readUInt(address);
  Serial.println(EEPROM.readUInt(address));
  return resulte;
}

void eeprom_read_data()  //
{
  serverinfo.NEW_SERVER_NUM = EEPROM.readUInt(ESP_ERR_CHAR_BASE_SERVERNUM);
  Serial.println(serverinfo.NEW_SERVER_NUM);

  if (serverinfo.NEW_SERVER_NUM > 0) {
    Serial.println("get setting value!");
    strcpy(serverinfo.NEW_IP, EEPROM.readString(ESP_ERR_STR_BASE_LOCALIP).c_str());  // string to char array
    strcpy(select_sensor_info.SELECT_TYPE,
           EEPROM.readString(ESP_ERR_STR_BASE_SET_SENSOR).c_str());                        // string to char array
    strcpy(serverinfo.NEW_RESET_TIME, EEPROM.readString(ESP_ERR_STR_RESET_TIME).c_str());  // string to char array

    serverinfo.NEW_PORT = EEPROM.readUInt(ESP_ERR_INT_BASE_LOCALPORT);
    serverinfo.NEW_SERVER_NUM = EEPROM.readUInt(ESP_ERR_CHAR_BASE_SERVERNUM);
    serverinfo.NEW_SERVER_SEND_DELAY_SEC = EEPROM.readUInt(ESP_ERR_INT_BASE_SEND_DELAY);
    serverinfo.NEW_RESET_SET = EEPROM.readUInt(ESP_ERR_CHAR_BASE_RESET_SET);
  } else {  // 팜모닝 향 프로그램
    Serial.println("init setting value!");

#if SERVER_LOCAL
    EEPROM.writeString(ESP_ERR_STR_BASE_LOCALIP, "192.168.50.100");
    EEPROM.writeUInt(ESP_ERR_INT_BASE_LOCALPORT, 8801);
    EEPROM.writeUInt(ESP_ERR_CHAR_BASE_SERVERNUM, 1);
    EEPROM.writeUInt(ESP_ERR_INT_BASE_SEND_DELAY, 60);
    EEPROM.writeUInt(ESP_ERR_CHAR_BASE_RESET_SET, 1);     // 하루 한번 재시작
    EEPROM.writeString(ESP_ERR_STR_RESET_TIME, "10:00");  // reset 시간
    EEPROM.writeString(ESP_ERR_STR_BASE_SET_SENSOR, "");  // sensoer_type

#else
    EEPROM.writeString(ESP_ERR_STR_BASE_LOCALIP, "192.168.50.100");
    EEPROM.writeUInt(ESP_ERR_INT_BASE_LOCALPORT, 8801);
    EEPROM.writeUInt(ESP_ERR_CHAR_BASE_SERVERNUM, 2);
    EEPROM.writeUInt(ESP_ERR_INT_BASE_SEND_DELAY, 60);
    EEPROM.writeUInt(ESP_ERR_CHAR_BASE_RESET_SET, 1);     // 하루 한번 재시작
    EEPROM.writeString(ESP_ERR_STR_RESET_TIME, "10:00");  // reset 시간
    EEPROM.writeString(ESP_ERR_STR_BASE_SET_SENSOR, "");  // sensoer_type
#endif

    EEPROM.commit();

    vTaskDelay(1000);
    strcpy(serverinfo.NEW_IP, EEPROM.readString(ESP_ERR_STR_BASE_LOCALIP).c_str());  // string to char array
    strcpy(select_sensor_info.SELECT_TYPE,
           EEPROM.readString(ESP_ERR_STR_BASE_SET_SENSOR).c_str());                        // string to char array
    strcpy(serverinfo.NEW_RESET_TIME, EEPROM.readString(ESP_ERR_STR_RESET_TIME).c_str());  // string to char array

    serverinfo.NEW_PORT = EEPROM.readUInt(ESP_ERR_INT_BASE_LOCALPORT);
    serverinfo.NEW_SERVER_NUM = EEPROM.readUInt(ESP_ERR_CHAR_BASE_SERVERNUM);
    serverinfo.NEW_SERVER_SEND_DELAY_SEC = EEPROM.readUInt(ESP_ERR_INT_BASE_SEND_DELAY);
    serverinfo.NEW_RESET_SET = EEPROM.readUInt(ESP_ERR_CHAR_BASE_RESET_SET);
  }

  Serial.println(serverinfo.NEW_IP);
  Serial.println(serverinfo.NEW_PORT);
  Serial.println(serverinfo.NEW_SERVER_NUM);
  Serial.println(serverinfo.NEW_SERVER_SEND_DELAY_SEC);
  Serial.println(serverinfo.NEW_RESET_SET);
  Serial.println(select_sensor_info.SELECT_TYPE);
  Serial.println(serverinfo.NEW_RESET_TIME);
}

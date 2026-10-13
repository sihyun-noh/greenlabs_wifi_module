#include <stdio.h>
#include <stdint.h>
#include <WiFi.h>
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>  // ArduinoJson  ver. 6
#include <AutoConnect.h>
#include <EEPROM.h>
#include <EEPROM_def.h>
#include <WebServer.h>
//#include "esp_system.h"
#include <esp_task_wdt.h>
#include <ezTime.h>
#include <global.h>
#include <time.h>

#include "esp_heap_caps.h"
#include "esp_heap_trace.h"
#include "secrets.h"

#include "spiffs_.h"
#include "log.h"
#include "syslog.h"

#include "web_serviece.h"
#include "async_webserver.h"

// extern "C" {
// #include "sysevent.h"
// }

int async_webserver_start(void);

#define VERSION_PROGRAM "4.0.0.0"

unsigned int CRC16_NOH, SEED, GP, temp_CRC16_NOH;  // for CRC16
#define SEED 0xFFFF                                // initialization for CRC16
#define GP 0xA001                                  // generating polynomial

static const char* TAG = "[M]";

size_t ret = 0;

size_t heapUsed;
size_t heapUsedAtStart;

uint8_t send_crc_check = 0;
uint8_t start_Get_flag = 0;
uint8_t SYS_RESET_FLAG = 0;

char str_empty[255] = { 0 };
char basic_url[255] = { 0 };
char get_code_buf[30] = { 0 };

char post_code_buf[30] = { 0 };
char send_str[255] = { 0 };
char dst_send_str[255] = { 0 };
char dst_send_str_time[255] = { 0 };
char str_check[255] = { 0 };

char str[255] = { 0 };
char str_buf[255] = { 0 };

int get_addr_flg = 0;
char addr_test[] = "77777777";
int korins_value[7];
char esp_chip_addr[20];
char esp_chip_addr_print[8];
char addr[20] = { 0 };

char sign = 0;
uint64_t chipid;

extern char print_str[20];
extern char send_data_node[255];
char send_data_node_buf[255];
char flow_data_node_buf[255];

volatile int interruptCounter = 0;
volatile int data_send_interruptCounter = 0;
uint8_t data_send_interruptCounter_flag = 0;
int totalInterruptCounter;

typedef union {
  byte byte_arr[4];
  float float_val;
} ByteToFloat;

union {
  float a;
  unsigned char bytes[4];
} thing;

// time_t  t;
// struct tm *tm;
// char    dateTime[24];

Timezone NewSeoul;

hw_timer_t* timer = NULL;
hw_timer_t* timer_WDT = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// set LCD address, number of columns and rows

// Replace with WebServer for ESP32

Web_Service esp32_web_service;

NEW_SERVER_INFO serverinfo;
NEW_SENSOR_INFO select_sensor_info;

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig config;
// AutoConnectAux    hello;

WiFiClient client_status;
WiFiClientSecure net_aws = WiFiClientSecure();
// HTTPClient http;

static const uint8_t mag_queue_len = 10;
static QueueHandle_t msg_Queue;
QueueHandle_t xQueue;

////////
static heap_trace_record_t trace_record[NUM_RECORDS];  // This buffer must be in internal RAM

// extern ESP32HTTPUpdate ESPhttpUpdate;

const char* ntpServer = "kr.pool.ntp.org";
////////// KST //////
// const long  gmtOffset_sec = 28800;  // list of UTC time offsets. Here are some examples for different timezones:
// korea UTF + 9 (ex For UTC +1.00 : 1 * 60 * 60 : 3600)   //1.0 const int   daylightOffset_sec = 3600; //Change the
// Daylight offset in milliseconds. If your country observes Daylight saving time set it to 3600. Otherwise, set it to
// 0. //1.0

////////// GMT //////
// const long  gmtOffset_sec = 3600;  // list of UTC time offsets. Here are some examples for different timezones: korea
// UTF + 9 (ex For UTC +1.00 : 1 * 60 * 60 : 3600) // 洹몃?��??���?? ?몴以???�� 蹂�???�� 9??���?? ?��좊쫫 const
// int daylightOffset_sec = 0; //Change the Daylight offset in milliseconds. If your country observes Daylight saving
// time set it to 3600. Otherwise, set it to 0.

////////// UTC //////
const long gmtOffset_sec = 0;  // list of UTC time offsets. Here are some examples for different timezones:  (ex For UTC
                               // +1.00 : 1 * 60 * 60 : 3600) // 洹몃?��??���?? ?몴以???��
const int daylightOffset_sec = 0;  // Change the Daylight offset in milliseconds. If your country observes Daylight
                                   // saving time set it to 3600. Otherwise, set it to 0.

void Calc_CRC(unsigned char b, unsigned int* CRC) {
  int carry, i;

  CRC[0] ^= b & 0xFF;
  for (i = 0; i < 8; i++) {
    carry = CRC[0] & 0x0001;
    CRC[0] >>= 1;
    if (carry)
      CRC[0] ^= GP;
  }
}

float bytetofloat(byte* data) {
  ByteToFloat __ByteToFloat;
  for (byte i = 0; i < 4; i++) {
    __ByteToFloat.byte_arr[3 - i] = data[i];
  }
  return __ByteToFloat.float_val;
}

void floattobyte(float a) {
  int ii;
  thing.a = a;
  for (ii = 0; ii < 4; ii++)
    printf("byte %d is %02x\n", ii, thing.bytes[ii]);
}

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;            // data check conut to sensor node
  data_send_interruptCounter++;  // send to server
  portEXIT_CRITICAL_ISR(&timerMux);
  if (data_send_interruptCounter >= serverinfo.NEW_SERVER_SEND_DELAY_SEC) {
    data_send_interruptCounter = 0;
    data_send_interruptCounter_flag = 1;
  }
}

void auxClock() {
  // t = time(NULL);
  // tm = localtime(&t);
  // sprintf(dateTime, "%04d-%02d-%02dT%02d:%02d:%02d.",
  //                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
  //                   tm->tm_hour, tm->tm_min, tm->tm_sec);
  Server.send(200, "text/plain", NewSeoul.dateTime());
}

void auxStatus() {
  Server.send(200, "text/plain", dst_send_str);
}

void auxStatus_heap() {
  Server.send(200, "text/plain", str_check);
}

void auxStatus_real_time_data() {
  Server.send(200, "text/plain", send_data_node_buf);
}

void auxFlowRealTimeData() {
  Server.send(200, "text/plain", flow_data_node_buf);
}

void auxStatus_get_code_check() {
  Server.send(200, "text/plain", get_code_buf);
}

void auxStatus_post_code_check() {
  Server.send(200, "text/plain", post_code_buf);
}

void PH_PV_read() {
  String val;
  char empty[100] = {
    0,
  };
  char empty_value[20] = {
    0,
  };
  int data_val1, data_val2, data_val_sum = 0;
  float value_tt = 0.0;

  CRC16_NOH = SEED;

  int Inputs[] = { 0x04, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 };
  for (int i = 0; i < 6; i++) {
    Calc_CRC(Inputs[i], &CRC16_NOH);
  }
  Inputs[6] = CRC16_NOH;
  CRC16_NOH = CRC16_NOH >> 8;
  Inputs[7] = CRC16_NOH;
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);

  vTaskDelay(SERIAL_TIME_OUT);

  if (!Serial.available())
    Server.send(200, "text/plain", "FAIL");
  else {
    byte leng = Serial.readBytes(empty, 100);
    val = String(empty[3], DEC);
    data_val1 = val.toInt();

    val = String(empty[4], DEC);
    data_val2 = val.toInt();
    data_val_sum = (255 * data_val1) + data_val1 + data_val2;

    value_tt = (float)data_val_sum / 100;
    sprintf(empty_value, "%.2f", value_tt);
    Server.send(200, "text/plain", empty_value);
    Serial.flush();
  }
}

void PH_918_compleat() {
  int Inputs[] = { 0xFE, 0x06, 0x00, 0x5A, 0x00, 0x0A, 0x3D, 0xD1 };
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);
  vTaskDelay(SERIAL_TIME_OUT);
  if (!Serial.available())
    Server.send(200, "text/plain", "FAIL");
  else
    Server.send(200, "text/plain", "OK");
}

void PH_686_compleat() {
  int Inputs[] = { 0xFE, 0x06, 0x00, 0x5B, 0x00, 0x0B, 0xAD, 0xD1 };
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);
  vTaskDelay(SERIAL_TIME_OUT);
  if (!Serial.available())
    Server.send(200, "text/plain", "FAIL");
  else
    Server.send(200, "text/plain", "OK");
}

void PH_400_compleat() {
  int Inputs[] = { 0xFE, 0x06, 0x00, 0x5C, 0x00, 0x0C, 0x5D, 0xD2 };
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);
  vTaskDelay(SERIAL_TIME_OUT);
  if (!Serial.available())
    Server.send(200, "text/plain", "FAIL");
  else
    Server.send(200, "text/plain", "OK");
}

void EC_PV_read() {
  String val;
  int Solution_value = 0;
  char empty[100] = {
    0,
  };
  char empty_value[40] = {
    0,
  };
  byte byte_arr[4];
  char str[40] = {
    0,
  };
  float Solution_Temp, C1, C2, EC1, rdata = 0.0;

  CRC16_NOH = SEED;

  int Inputs[] = { 0x07, 0x03, 0x00, 0x00, 0x00, 0x0C, 0x45, 0xA9 };
  for (int i = 0; i < 6; i++) {
    Calc_CRC(Inputs[i], &CRC16_NOH);
  }
  Inputs[6] = CRC16_NOH;
  CRC16_NOH = CRC16_NOH >> 8;
  Inputs[7] = CRC16_NOH;
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);

  vTaskDelay(SERIAL_TIME_OUT);

  if (!Serial.available())
    Server.send(200, "text/plain", "FAIL,FAIL,FAIL,FAIL");
  else {
    byte leng = Serial.readBytes(empty, 100);

    /////////////////////////// EC value //////////////////////////////
    byte_arr[0] = (byte)empty[3];
    byte_arr[1] = (byte)empty[4];
    byte_arr[2] = (byte)empty[5];
    byte_arr[3] = (byte)empty[6];

    rdata = bytetofloat(byte_arr);
    EC1 = rdata;
    // sprintf(str,"rdata = %.4f mS/Cm", rdata);
    sprintf(str, "%.2f,", rdata);
    strcat(empty_value, str);

    /////////////////////////// Temp value //////////////////////////////
    byte_arr[0] = (byte)empty[11];
    byte_arr[1] = (byte)empty[12];
    byte_arr[2] = (byte)empty[13];
    byte_arr[3] = (byte)empty[14];

    rdata = bytetofloat(byte_arr);
    Solution_Temp = rdata;
    sprintf(str, "%.2f,", rdata);
    strcat(empty_value, str);

    /////////////////////////// //C1 ?�???���?? //////////////////////////////

    byte_arr[0] = (byte)empty[23];
    byte_arr[1] = (byte)empty[24];
    byte_arr[2] = (byte)empty[25];
    byte_arr[3] = (byte)empty[26];

    rdata = bytetofloat(byte_arr);
    C1 = rdata;
    sprintf(str, "%.2f,", rdata);
    strcat(empty_value, str);

    /////////////////////////// //C2 ?�???���?? //////////////////////////////
    // CALIBATION
    if ((float)15.0 < Solution_Temp && Solution_Temp < (float)16.0)
      Solution_value = 1147;
    else if ((float)16.0 < Solution_Temp && Solution_Temp < (float)17.0)
      Solution_value = 1173;
    else if ((float)17.0 < Solution_Temp && Solution_Temp < (float)18.0)
      Solution_value = 1199;
    else if ((float)18.0 < Solution_Temp && Solution_Temp < (float)19.0)
      Solution_value = 1225;
    else if ((float)19.0 < Solution_Temp && Solution_Temp < (float)20.0)
      Solution_value = 1251;
    else if ((float)20.0 < Solution_Temp && Solution_Temp < (float)21.0)
      Solution_value = 1278;
    else if ((float)21.0 < Solution_Temp && Solution_Temp < (float)22.0)
      Solution_value = 1305;
    else if ((float)22.0 < Solution_Temp && Solution_Temp < (float)23.0)
      Solution_value = 1332;
    else if ((float)23.0 < Solution_Temp && Solution_Temp < (float)24.0)
      Solution_value = 1359;
    else if ((float)24.0 < Solution_Temp && Solution_Temp < (float)25.0)
      Solution_value = 1386;
    else if ((float)25.0 < Solution_Temp && Solution_Temp < (float)26.0)
      Solution_value = 1413;
    else if ((float)26.0 < Solution_Temp && Solution_Temp < (float)27.0)
      Solution_value = 1440;
    else if ((float)27.0 < Solution_Temp && Solution_Temp < (float)28.0)
      Solution_value = 1467;
    else if ((float)28.0 < Solution_Temp && Solution_Temp < (float)29.0)
      Solution_value = 1494;
    else if ((float)29.0 < Solution_Temp && Solution_Temp < (float)30.0)
      Solution_value = 1521;
    else
      Solution_value = 2000;

    C2 = Solution_value / ((EC1 * 1000) / C1);
    sprintf(str, "%.2f", C2);
    strcat(empty_value, str);

    floattobyte(C2);
    Server.send(200, "text/plain", empty_value);
  }
}

void EC_1413_compleat() {
  CRC16_NOH = SEED;

  int Inputs_CAL[] = {
    0x07, 0x10, 0x00, 0x0A, 0x00, 0x02, 0x04, thing.bytes[3], thing.bytes[2], thing.bytes[1], thing.bytes[0], 0x00, 0x00
  };  // must number 07.!!
  for (int i = 0; i < 11; i++)
    Calc_CRC(Inputs_CAL[i], &CRC16_NOH);

  temp_CRC16_NOH = CRC16_NOH;
  Inputs_CAL[11] = temp_CRC16_NOH & 0xFF;
  CRC16_NOH = CRC16_NOH >> 8;
  Inputs_CAL[12] = CRC16_NOH;

  for (int i = 0; i < 13; i++) {
    Serial.write(Inputs_CAL[i]);
  }
  vTaskDelay(SERIAL_TIME_OUT);
  if (!Serial.available())
    Server.send(200, "text/plain", "FAIL");
  else
    Server.send(200, "text/plain", "OK");
}

String checkSensor(AutoConnectAux& aux, PageArgument& arg) {
  uint8_t TOTAL_SENSOR_GROUP_MAX = 8;

  uint8_t SENSOR_GROUP_1_MAX = 5;
  uint8_t SENSOR_GROUP_2_MAX = 3;
  uint8_t SENSOR_GROUP_3_MAX = 5;
  uint8_t SENSOR_GROUP_4_MAX = 2;
  uint8_t SENSOR_GROUP_5_MAX = 3;
  uint8_t SENSOR_GROUP_6_MAX = 5;
  uint8_t SENSOR_GROUP_7_MAX = 1;
  uint8_t SENSOR_GROUP_8_MAX = 1;

  char str_checkSensor[15];
  char str_checkSensor_result[255] = { 0 };
  select_sensor_info.SELECT_MEMBER = 0;                                                  // 理쒓�???��
  memset(select_sensor_info.SELECT_TYPE, 0x00, sizeof(select_sensor_info.SELECT_TYPE));  // ?��?���???��

  AutoConnectText& result = aux["result"].as<AutoConnectText>();
  AutoConnectText& result_2 = aux["result_2"].as<AutoConnectText>();

  // strcat(str_checkSensor_result, "SET_SENSOR : ");

  for (int ii = 1; ii <= TOTAL_SENSOR_GROUP_MAX; ii++) {
    switch (ii) {
      case 1:
        for (int jj = 1; jj <= SENSOR_GROUP_1_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            if (!strcmp(str_checkSensor, "SET_SENSOR_1_1")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'J';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'K';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_1_2")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'Y';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'L';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_1_3")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'U';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'r';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'd';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_1_4")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'l';
              select_sensor_info.SELECT_MEMBER++;
            }

            //////////////////////////?��?�삎 ??��??��?�? ?���???_22.0128
            if (!strcmp(str_checkSensor, "SET_SENSOR_1_5")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'T';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'H';
              select_sensor_info.SELECT_MEMBER++;
            }
            //////////////////////////?��?�삎 ??��??��?�? ?���???_22.0128

            //   Serial.println(str_checkSensor);
            //   Serial.println(select_sensor_info.SELECT_TYPE);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }

        /* code */
        break;

      case 2:
        for (int jj = 1; jj <= SENSOR_GROUP_2_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            if (!strcmp(str_checkSensor, "SET_SENSOR_2_1")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'C';
              select_sensor_info.SELECT_MEMBER++;
            }
            if (!strcmp(str_checkSensor, "SET_SENSOR_2_2")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'c';
              select_sensor_info.SELECT_MEMBER++;
            }
            if (!strcmp(str_checkSensor, "SET_SENSOR_2_3")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'i';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'o';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'p';
              select_sensor_info.SELECT_MEMBER++;
            }

            // Serial.println(str_checkSensor);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }

        /* code */
        break;

      case 3:
        for (int jj = 1; jj <= SENSOR_GROUP_3_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            if (!strcmp(str_checkSensor, "SET_SENSOR_3_1")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'G';
              select_sensor_info.SELECT_MEMBER++;
            }
            if (!strcmp(str_checkSensor, "SET_SENSOR_3_2")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'V';
              select_sensor_info.SELECT_MEMBER++;
            }
            if (!strcmp(str_checkSensor, "SET_SENSOR_3_3")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'X';
              select_sensor_info.SELECT_MEMBER++;
            }
            if (!strcmp(str_checkSensor, "SET_SENSOR_3_4")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'R';
              select_sensor_info.SELECT_MEMBER++;
            }
            if (!strcmp(str_checkSensor, "SET_SENSOR_3_5")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'y';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'O';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'g';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'A';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 's';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'e';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'Q';
              select_sensor_info.SELECT_MEMBER++;
            }

            // Serial.println(str_checkSensor);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }
        /* code */
        break;

      case 4:
        for (int jj = 1; jj <= SENSOR_GROUP_4_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            if (!strcmp(str_checkSensor, "SET_SENSOR_4_1")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'j';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_4_2")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'x';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = '1';
              select_sensor_info.SELECT_MEMBER++;
            }
            // Serial.println(str_checkSensor);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }
        /* code */
        break;

      case 5:
        for (int jj = 1; jj <= SENSOR_GROUP_5_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            if (!strcmp(str_checkSensor, "SET_SENSOR_5_1")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'j';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_5_2")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'v';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'n';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'q';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_5_3")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'E';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'm';
              select_sensor_info.SELECT_MEMBER++;

              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'u';
              select_sensor_info.SELECT_MEMBER++;
            }

            // Serial.println(str_checkSensor);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }
        /* code */
        break;

      case 6:
        for (int jj = 1; jj <= SENSOR_GROUP_6_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            if (!strcmp(str_checkSensor, "SET_SENSOR_6_1")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'w';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_6_2")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'N';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_6_3")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'F';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_6_4")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'b';
              select_sensor_info.SELECT_MEMBER++;
            }

            if (!strcmp(str_checkSensor, "SET_SENSOR_6_5")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'M';
              select_sensor_info.SELECT_MEMBER++;
            }

            // Serial.println(str_checkSensor);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }
        /* code */
        break;

      case 7:
        for (int jj = 1; jj <= SENSOR_GROUP_7_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            if (!strcmp(str_checkSensor, "SET_SENSOR_7_1")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'B';
              select_sensor_info.SELECT_MEMBER++;
            }
            // Serial.println(str_checkSensor);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }

        select_sensor_info.FLOW_MULTIPLIER = arg.arg("FLOW_MULTIPLIER").toInt();
        if (select_sensor_info.FLOW_MULTIPLIER == 1 || select_sensor_info.FLOW_MULTIPLIER == 10 ||
            select_sensor_info.FLOW_MULTIPLIER == 100) {
          select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'f';
          select_sensor_info.SELECT_MEMBER++;
          strcat(str_checkSensor_result, "FLOW_MULTIPLIER_");
          strcat(str_checkSensor_result, String(select_sensor_info.FLOW_MULTIPLIER).c_str());
          strcat(str_checkSensor_result, ",");
        } else {
          select_sensor_info.FLOW_MULTIPLIER = 1;
        }
        /* code */
        break;

      case 8:
        for (int jj = 1; jj <= SENSOR_GROUP_8_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            // Serial.println(str_checkSensor);
            memset(select_sensor_info.SELECT_TYPE, 0x00, sizeof(select_sensor_info.SELECT_TYPE));
            select_sensor_info.SELECT_MEMBER = 0;
            select_sensor_info.FLOW_MULTIPLIER = 1;
            strcat(str_checkSensor_result, "All disable checked sensor!");
          }
        }
        /* code */
        break;

      default: break;
    }
  }

  // eeprom saved
  EEPROM.writeString(ESP_ERR_STR_BASE_SET_SENSOR, select_sensor_info.SELECT_TYPE);
  EEPROM.writeUInt(ESP_ERR_INT_FLOW_MULTIPLIER, select_sensor_info.FLOW_MULTIPLIER);
  EEPROM.commit();
  // eeprom saved

  result.value = "SET_SENSOR : " + (String)str_checkSensor_result;
  result.style = "color:red;";
  result_2.value = "KEY_CODE : " + (String)select_sensor_info.SELECT_TYPE +
                   " / FLOW_MULTIPLIER : " + String(select_sensor_info.FLOW_MULTIPLIER);
  result_2.style = "color:blue;";

  return String("");
}

String checkIPAddress(AutoConnectAux& aux, PageArgument& args) {
  AutoConnectAux& input_page = *Portal.aux("/input_page");
  AutoConnectText& result = aux["result"].as<AutoConnectText>();
  AutoConnectInput& input_passwd = input_page["passwd"].as<AutoConnectInput>();
  AutoConnectInput& input_ip = input_page["ipaddress"].as<AutoConnectInput>();
  AutoConnectInput& input_port = input_page.getElement<AutoConnectInput>("portNum");
  AutoConnectRadio& radio_select = input_page.getElement<AutoConnectRadio>("period");
  AutoConnectRadio& radio_select_server =
      input_page.getElement<AutoConnectRadio>("server_selsect");  // 1: local, 2:?��꾩꽍紐⑤?��, 3:aws
  AutoConnectRadio& radio_set_mode = input_page.getElement<AutoConnectRadio>("set_mode");
  AutoConnectRadio& radio_reset_set = input_page.getElement<AutoConnectRadio>("reset_set");
  AutoConnectInput& input_reset_time = input_page.getElement<AutoConnectInput>("reset_time");

  IPAddress ip;
  ip.fromString(input_ip.value);
  input_ip.value = ip.toString();

  if (input_passwd.isValid() && input_passwd.value == (String)PASSWD_VALUE) {
    result.value = "IP Address " + input_ip.value + " is OK.";
    result.style = "";

    strcpy(serverinfo.NEW_IP, input_ip.value.c_str());
    serverinfo.NEW_PORT = input_port.value.toInt();
    serverinfo.NEW_SERVER_NUM = radio_select_server.checked;
    serverinfo.NEW_SET_MODE = radio_set_mode.checked;
    serverinfo.NEW_RESET_SET = radio_reset_set.checked;
    strcpy(serverinfo.NEW_RESET_TIME, input_reset_time.value.c_str());

    switch (radio_select.checked) {
      case 1: serverinfo.NEW_SERVER_SEND_DELAY_SEC = radio_select.checked * 10; break;
      case 2: serverinfo.NEW_SERVER_SEND_DELAY_SEC = radio_select.checked * 15; break;
      case 3: serverinfo.NEW_SERVER_SEND_DELAY_SEC = radio_select.checked * 20; break;
      case 4: serverinfo.NEW_SERVER_SEND_DELAY_SEC = radio_select.checked * 45; break;

      default: break;
    }

    EEPROM.writeString(ESP_ERR_STR_BASE_LOCALIP, serverinfo.NEW_IP);
    EEPROM.writeUInt(ESP_ERR_INT_BASE_LOCALPORT, serverinfo.NEW_PORT);
    EEPROM.writeUInt(ESP_ERR_CHAR_BASE_SERVERNUM, serverinfo.NEW_SERVER_NUM);
    EEPROM.writeUInt(ESP_ERR_INT_BASE_SEND_DELAY, serverinfo.NEW_SERVER_SEND_DELAY_SEC);
    EEPROM.writeUInt(ESP_ERR_CHAR_BASE_RESET_SET, serverinfo.NEW_RESET_SET);
    EEPROM.writeString(ESP_ERR_STR_RESET_TIME, serverinfo.NEW_RESET_TIME);

    EEPROM.commit();

  } else {
    result.value = "IP Address " + input_ip.value + " error." + "Please confirm your password.";
    result.style = "color:red;";
  }

  return String("");
}

void rootPage() {
  char content[] = "hello_framlabs.";
  Server.send(200, "text/plain", content);
  // Server.send(200, "text/plain", WiFi.localIP().toString());
}

void I2Cinitialize() {
  // I2Cinitialize
  byte error, address;
  int nDevices;

  Wire.begin();
  USE_SERIAL.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      USE_SERIAL.print("I2C device found at address 0x");
      if (address < 16) {
        USE_SERIAL.print("0");
      }
      USE_SERIAL.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      USE_SERIAL.print("Unknow error at address 0x");
      if (address < 16) {
        USE_SERIAL.print("0");
      }
      USE_SERIAL.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    USE_SERIAL.println("No I2C devices found\n");
  } else {
    USE_SERIAL.println("done\n");
  }
  delay(500);
}

void wifi_autoconnect() {
  LOGI(TAG, "wifi_autoconnect start1");
  SLOGI(TAG, "wifi_autoconnect start1");
  config.title = "GREENLABS" + String(VERSION_PROGRAM);
  config.apid = "GREENLABS-" + String(esp_chip_addr);  // esp chip UID print
  config.psk = "admin123";
  // config.psk  = esp_chip_addr;
  digitalWrite(OUTPIN_2, LOW);  // wifi_autoconnect live LED

  /*
    //config.autoReset = false;     // Not reset the module even by intentional disconnection using AutoConnect menu.
    config.autoReset = true;     // Not reset the module even by intentional disconnection using AutoConnect menu.
    config.autoReconnect = true;  // Reconnect to known access points.
    config.reconnectInterval = 6; // Reconnection attempting interval is 3[min].
    config.retainPortal = true;   // Keep the captive portal open.
  */

  // 5/19??�� ?���???
  config.autoReconnect = true;   // autoReconnect
  config.reconnectInterval = 6;  // Reconnection attempting interval is 3[min].
  config.autoReset = false;

  Portal.config(config);
  Server.on("/", rootPage);

  WiFi.setHostname(HOSTNAME);
  if (Portal.begin()) {
    USE_SERIAL.println("WiFi connected: " + WiFi.localIP().toString());
    // USE_SERIAL.println("WiFi connected: " + WiFi.localIP().toString());
  }
  // delay(5000);

  // active wifi live led
  digitalWrite(OUTPIN_2, HIGH);  // wifi_autoconnect live LED
  delay(1000);
  digitalWrite(OUTPIN_2, LOW);  // wifi_autoconnect live LED
  delay(1000);
  digitalWrite(OUTPIN_2, HIGH);  // wifi_autoconnect live LED
  delay(1000);
  digitalWrite(OUTPIN_2, LOW);  // wifi_autoconnect live LED
  delay(1000);
  digitalWrite(OUTPIN_2, HIGH);  // wifi_autoconnect live LED

  digitalWrite(OUTPIN_3, LOW);   // wifi status led on
  digitalWrite(OUTPIN_7, HIGH);  // // sensor_node wifi status led on
}

void otadive_update() {
  String url = "http://otadrive.com/deviceapi/update?";
  url += "k=94b620cc-5609-4742-8e23-eae6485f4a60";
  url += "&v=" + String(VERSION_PROGRAM);
  // url += "&v=1.0.0.1";
  url += "&s=" + String(esp_chip_addr);

  Serial.println(url);

  WiFiClient client;
  httpUpdate.update(client, url, String(VERSION_PROGRAM));
  // httpUpdate.update(client, url, "1.0.0.1");
}

int XOR_CRC(char* str, int num) {
  int crc = 0;
  String val;
  int strTodec = 0;
  int count = 0;

  for (int i = 0; i < num; i++) {
    val = String(str[i], DEC);
    strTodec = val.toInt();
    crc = crc ^ strTodec;
    // Serial.println(crc,HEX);
  }

  // Serial.print("return : ");
  // Serial.println(crc,HEX);
  return crc;
}

void get_version(char* str) {
  char buf[20] = { 0 };

  buf[0] = 0x46;
  buf[1] = 0x33;
  buf[2] = 0x0f;
  buf[3] = str[0];
  buf[4] = str[1];
  buf[5] = str[2];
  buf[6] = str[3];
  buf[7] = str[4];
  buf[8] = str[5];
  buf[9] = str[6];
  buf[10] = str[7];
  buf[11] = 0x56;
  buf[12] = 0x00;

  Serial2.write(0x46);
  Serial2.write(0x33);
  Serial2.write(0x0f);
  Serial2.write(str[0]);
  Serial2.write(str[1]);
  Serial2.write(str[2]);
  Serial2.write(str[3]);
  Serial2.write(str[4]);
  Serial2.write(str[5]);
  Serial2.write(str[6]);
  Serial2.write(str[7]);
  Serial2.write(0x56);
  Serial2.write(0x00);
  Serial2.write(0x00);
  Serial2.write(0x00);
  Serial2.write(0x00);

  Serial2.write(XOR_CRC(buf, 17));
}

void basic_sensor_get_last(char* addr, Sensor_type* newSensor) {
  String val;
  int data_len = 0;
  char hexbuf[10] = { 0 };
  char total_hexbuf[100] = { 0 };
  char Inputs[] = {
   0x46,
   0x33,
   0x0f,
   0x00,
   0x00,
   0x00,
   0x00,
   0x00,
   0x00,
   0x00,
   0x00,
   0x50,
   0x00,
   0x00,
   0x00,
   0x00,
   0x00
  };

  for (int i = 0; i < 8; i++)
    Inputs[i + 3] = addr[i];

  // 12 bitset
  if (strchr(newSensor->type, 'H') != NULL || strchr(newSensor->type, 'T') != NULL)
    bitSet(Inputs[12], 7);
  if (strchr(newSensor->type, 'C') != NULL)
    bitSet(Inputs[12], 6);
  if (strchr(newSensor->type, 'c') != NULL)
    bitSet(Inputs[12], 5);
  if (strchr(newSensor->type, 'W') != NULL)
    bitSet(Inputs[12], 4);
  if (strchr(newSensor->type, 'P') != NULL)
    bitSet(Inputs[12], 3);
  if (strchr(newSensor->type, 'D') != NULL)
    bitSet(Inputs[12], 2);
  if (strchr(newSensor->type, 'S') != NULL)
    bitSet(Inputs[12], 1);
  if (strchr(newSensor->type, 'R') != NULL)
    bitSet(Inputs[12], 0);

  // 13 bitset
  if (strchr(newSensor->type, 'E') != NULL || strchr(newSensor->type, 'm') != NULL ||
      strchr(newSensor->type, 'u') != NULL)
    bitSet(Inputs[13], 7);

  if (strchr(newSensor->type, 'B') != NULL)
    bitSet(Inputs[13], 6);

  if (strchr(newSensor->type, 'w') != NULL)
    bitSet(Inputs[13], 5);
  if (strchr(newSensor->type, 'f') != NULL)
    bitSet(Inputs[13], 4);

  if (strchr(newSensor->type, 'U') != NULL || strchr(newSensor->type, 'r') != NULL ||
      strchr(newSensor->type, 'd') != NULL)
    bitSet(Inputs[13], 3);

  if (strchr(newSensor->type, 'N') != NULL)
    bitSet(Inputs[13], 2);
  if (strchr(newSensor->type, 'F') != NULL)
    bitSet(Inputs[13], 1);
  if (strchr(newSensor->type, 'b') != NULL)
    bitSet(Inputs[13], 0);

  // 14 bitset
  if (strchr(newSensor->type, 'Y') != NULL)
    bitSet(Inputs[14], 7);
  if (strchr(newSensor->type, 'L') != NULL)
    bitSet(Inputs[14], 6);
  if (strchr(newSensor->type, 'M') != NULL)
    bitSet(Inputs[14], 5);
  if (strchr(newSensor->type, 'J') != NULL || strchr(newSensor->type, 'K') != NULL)
    bitSet(Inputs[14], 4);

  if (strchr(newSensor->type, 'i') != NULL || strchr(newSensor->type, 'o') != NULL ||
      strchr(newSensor->type, 'p') != NULL)
    bitSet(Inputs[14], 3);  // SH-VT260

  if (strchr(newSensor->type, 'y') != NULL || strchr(newSensor->type, 'O') != NULL ||
      strchr(newSensor->type, 'g') != NULL || strchr(newSensor->type, 'A') != NULL ||
      strchr(newSensor->type, 's') != NULL || strchr(newSensor->type, 'e') != NULL ||
      strchr(newSensor->type, 'Q') != NULL)
    bitSet(Inputs[14], 2);

  if (strchr(newSensor->type, 'l') != NULL)
    bitSet(Inputs[14], 1);

  // 15 bitset
  if (strchr(newSensor->type, 'G') != NULL)
    bitSet(Inputs[15], 6);
  if (strchr(newSensor->type, 'V') != NULL)
    bitSet(Inputs[15], 5);
  if (strchr(newSensor->type, 'X') != NULL)
    bitSet(Inputs[15], 4);
  if (strchr(newSensor->type, 'j') != NULL)
    bitSet(Inputs[15], 3);

  if (strchr(newSensor->type, 'v') != NULL || strchr(newSensor->type, 'n') != NULL ||
      strchr(newSensor->type, 'q') != NULL)
    bitSet(Inputs[15], 2);

  if (strchr(newSensor->type, 'x') != NULL || strchr(newSensor->type, '1') != NULL)
    bitSet(Inputs[15], 0);

  Inputs[16] = XOR_CRC(Inputs, sizeof(Inputs));

  for (int i = 0; i < 17; i++) {
    val = String(Inputs[i], DEC);
    data_len = val.toInt();
    snprintf(hexbuf, sizeof(hexbuf), "%x", data_len);
    strcat(total_hexbuf, (const char*)hexbuf);
  }
  Serial.println();

  SLOGI(TAG, "TX_Node_test : %s", total_hexbuf);

  for (int i = 0; i < 17; i++)
    Serial2.write(Inputs[i]);
}

int addr_version(char* temp, byte leng) {
  char input_crc, ch;
  int crc = 0;
  String val;
  int strTodec = 0;

  for (int i = 0; 12 + i < leng; i++) {
    val = String(temp[12 + i]);
    ch = val.charAt(0);
    Serial.print(ch);
  }

  //// CRC check!////
  val = String(temp[leng - 1], DEC);
  strTodec = val.toInt();
  input_crc = strTodec;
  Serial.print("Input data crc : ");
  Serial.println(input_crc, HEX);

  for (int i = 0; i < leng - 1; i++) {
    val = String(temp[i], DEC);
    strTodec = val.toInt();
    crc = crc ^ strTodec;
    // Serial.print(crc,HEX);
    if (i == leng - 2)
      Serial.println(crc, HEX);
  }

  if (input_crc == crc) {
    Serial.println("CRC_ok");
    return 1;

  }

  else {
    Serial.println("CRC_error");
    return 0;
  }
}

void Basic_sensor_get_responsive_last(char* temp, byte leng, linkedList* L, Sensor_type* newSensor) {
  char input_crc, ch;
  String val;
  int strTodec, data_len, crc = 0;
  int count_val = 13;

  //// Error_Code check!////
  val = String(temp[12], DEC);
  data_len = val.toInt();
  // SLOGI(TAG, "Error_Code : %d", data_len, HEX);

  //// Sensor_Code check!////

  ////////////////////////////////////////////////////////////////////////////////////
  L->cur = NULL;
  L->head = NULL;
  L->tail = NULL;

  // for(int i=0; i < newSensor->member ; i++) {
  while (ch != END_CODE) {
    val = String(temp[count_val]);
    ch = val.charAt(0);

    if (ch == 'R' || ch == 'w' || ch == 'N' || ch == 'F' || ch == 'b' || ch == 'M' || ch == 's')
      count_val = Sensor_parsing_1BYTE(temp, count_val, L, newSensor);
    else if (ch == END_CODE)
      continue;
    else
      count_val = Sensor_parsing_2BYTE(temp, count_val, L);
  }
  ////////////////////////////////////////////////////////////////////////////////////

  ////End_Code check!////

  val = String(temp[leng - 2]);
  ch = val.charAt(0);
  // SLOGI(TAG, "End_code : %c", ch);

  //// CRC check!////
  val = String(temp[leng - 1], DEC);
  strTodec = val.toInt();
  input_crc = strTodec;
  // SLOGI(TAG, "Input data crc : %c", input_crc, HEX);

  for (int i = 0; i < leng - 1; i++) {
    val = String(temp[i], DEC);
    strTodec = val.toInt();
    crc = crc ^ strTodec;
    // Serial.print(crc,HEX);
    if (i == leng - 2)
      Serial.println(crc, HEX);
  }

  if (input_crc == crc) {
    SLOGI(TAG, "CRC_OK");
    send_crc_check = 1;

  } else {
    SLOGI(TAG, "CRC_ERROR");
    send_crc_check = 0;
  }
}

void Basic_Sonser_Get() {
  static char buf[100];
  String val;
  int data_len = 0;

  if (!get_addr_flg) {
    get_version(addr_test);
    SLOGI(TAG, "request node addr...");
  }

  do {
  } while (!Serial2.available());

  if (Serial2.available()) {
    byte len = Serial2.readBytes(buf, 100);

    SLOGI(TAG, "Input data Lenght %x ", len);

    SLOGI(TAG, "Input data ");
    for (int i = 0; i < len; i++) {
      val = String(buf[i], DEC);
      data_len = val.toInt();
      Serial.print(data_len, HEX);
    }
    Serial.println();

    //// datalen check!////
    val = String(buf[2], DEC);
    data_len = val.toInt();
    SLOGI(TAG, "Input senson_node_data Lenght : %d ", data_len);

    //// address check!////

    for (int i = 0; i < 8; i++) {
      val = String(buf[3 + i]);
      addr[i] = val.charAt(0);
    }
    SLOGI(TAG, "addr : %s", addr);

    //// OP_Code check!////
    val = String(buf[11], DEC);
    data_len = val.toInt();
    SLOGI(TAG, "OP_Code : %d", data_len, HEX);
    // Serial.println(data_len, HEX);

    switch (data_len) {
      case 118:  // 0x76 version ??���?? op code
        get_addr_flg = addr_version(buf, len);

        break;
      default: get_addr_flg = 0; break;
    }
  }
}

void Basic_Sonser_Get_last(linkedList* L, Sensor_type* newSensor) {
  char buf[100] = { 0 };
  char hexbuf[10] = { 0 };
  char total_hexbuf[100] = { 0 };
  String val;
  int data_len = 0;

  SLOGI(TAG, "Set_Code : %s ", newSensor->type);
  digitalWrite(OUTPIN_6, HIGH);  // sensor node to wifi module status check LED

  basic_sensor_get_last(addr, newSensor);
  do {
  } while (!Serial2.available());
  byte len = Serial2.readBytes(buf, 100);

  basic_sensor_get_last(addr, newSensor);
  do {
  } while (!Serial2.available());

  if (Serial2.available()) {
    if (start_Get_flag)
      digitalWrite(OUTPIN_6, LOW);  // sensor node to wifi module status check LED

    byte len = Serial2.readBytes(buf, 100);
    // SLOGI(TAG, "Input data Lenght : %d", len);
    //  SLOGI(TAG, "node response data : ");

    for (int i = 0; i < len; i++) {
      val = String(buf[i], DEC);
      data_len = val.toInt();
      // Serial.print(data_len, HEX);
      snprintf(hexbuf, sizeof(hexbuf), "%x", data_len);
      strcat(total_hexbuf, (const char*)hexbuf);
    }
    Serial.println();

    //// datalen check!////
    val = String(buf[2], DEC);
    data_len = val.toInt();
    // SLOGI(TAG, "Input senson_node_data Lenght : %d", data_len);

    SLOGI(TAG, "len : %d, RX_Node : %s", data_len, total_hexbuf);

    //// address check!////

    for (int i = 0; i < 8; i++) {
      val = String(buf[3 + i]);
      addr[i] = val.charAt(0);
    }
    SLOGI(TAG, "addr : %s", addr);

    //// OP_Code check!////
    val = String(buf[11], DEC);
    data_len = val.toInt();
    // SLOGI(TAG, "OP_Code : %d", data_len, HEX);

    switch (data_len) {
      case 112: Basic_sensor_get_responsive_last(buf, len, L, newSensor); break;
      case 118:  // 0x76 version ??���?? op code
        get_addr_flg = addr_version(buf, len);

        break;
      default: get_addr_flg = 0; break;
    }
  }
}

void Httpdatesend_GET(Sensor_type* newSensor) {
  if ((WiFi.status() == WL_CONNECTED)) {
    digitalWrite(OUTPIN_3, LOW);
    WiFiClient client;
    HTTPClient http;

    // configure traged server and url
    // sprintf(basic_url,"http://%s/api/v1/basic?address=%d" ,FARMLABS_URL, address_basic); //
#if SERVER_LOCAL
    sprintf(basic_url, "http://%s/api/v1/wifi-sensor?address=%s", SERVER_IP, esp_chip_addr);
#else
    // sprintf(basic_url,"http://%s/api/v1/basic?address=%s" ,FARMLABS_URL, addr);
    sprintf(basic_url, "http://%s/api/v1/basic?address=%s", FARMLABS_URL, esp_chip_addr);  // 07.01

#endif
    Serial.println(basic_url);
    Serial.print(basic_url);
    http.begin(client, basic_url);  // HTTP

    http.addHeader("Content-Type", "application/json");

    Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      const String& payload = http.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        digitalWrite(OUTPIN_1, LOW);

        StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc, payload);
        // Test if parsing succeeds.
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }

        // value ["J","K"]
        const char* return_code = doc["code"];
        char response_code[] = "0000";
        int ret = strcmp(return_code, response_code);
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        newSensor->member = doc["result"].size();
        Serial.print("result value_member : ");
        Serial.println(newSensor->member);

        const char* data;

        for (int ab = 0; ab < doc["result"].size(); ab++) {
          data = doc["result"][ab];
          Serial.print("data value :");
          Serial.println(data);
          newSensor->type_str[ab] = data;
          char* ptr = strchr(Sersor_CODE_array, *data);
          // char* ptr = strchr(array_1bytevalue, *data);

          if (ptr != NULL) {
            newSensor->type[ab] = *ptr;
            if (*ptr == 'c')
              newSensor->member += 2;  // Serial.println("key 'c' input");

            Serial.print("newSensor->type:");
            Serial.println(newSensor->type[ab]);

            Serial.print("newSensor->type_str:");
            Serial.println(newSensor->type_str[ab]);
          }
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (!ret) {
          start_Get_flag = 1;  /// get_send_return ok!
        } else {
          start_Get_flag = 0;  /// get_send_return ok!
        }
      } else {
        digitalWrite(OUTPIN_1, HIGH);
        start_Get_flag = 0;  /// get_send_return ok!
      }

    } else {
      start_Get_flag = 0;  /// get_send_return ok!
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());

      Serial.println("[HTTP] GET..failed");
      digitalWrite(OUTPIN_1, HIGH);
    }
    http.end();
    client.flush();
    ;
    // client.stop();
  } else if (WiFi.status() ==
             WL_IDLE_STATUS) {  // it is a temporary status assigned when WiFi.begin() is called and remains active
                                // until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a
                                // connection is established (resulting in WL_CONNECTED);
    Serial.println("WL_IDLE_STATUS");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    // wifi_autoconnect();
    for (;;)
      ;                                            // induce wdt !!
  } else if (WiFi.status() == WL_NO_SSID_AVAIL) {  // assigned when no SSID are available
    Serial.println("WL_NO_SSID_AVAIL");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    // wifi_autoconnect();
    for (;;)
      ;                                             // induce wdt !!
  } else if (WiFi.status() == WL_CONNECT_FAILED) {  // assigned when the connection fails for all the attempts
    Serial.println("WL_CONNECT_FAILED");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    // wifi_autoconnect();
    for (;;)
      ;                                              // induce wdt !!
  } else if (WiFi.status() == WL_CONNECTION_LOST) {  // assigned when the connection is lost
    Serial.println("WL_CONNECTION_LOST");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    // wifi_autoconnect();
    for (;;)
      ;                                           // induce wdt !!
  } else if (WiFi.status() == WL_DISCONNECTED) {  // assigned when disconnected from a network;
    Serial.println("WL_DISCONNECTED");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    // wifi_autoconnect();
    for (;;)
      ;  // induce wdt !!
  } else {
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);

    // wifi_autoconnect();
    for (;;)
      ;  // induce wdt !!
  }
}

void Httpdatesend_GET_NEW(Sensor_type* _newSensor) {
  if ((WiFi.status() == WL_CONNECTED)) {
    digitalWrite(OUTPIN_3, LOW);
    WiFiClient client;
    HTTPClient http;

    if (serverinfo.NEW_SERVER_NUM == 1) {
      sprintf(basic_url, "http://%s:%d/api/v1/wifi-sensor?address=%s", serverinfo.NEW_IP, serverinfo.NEW_PORT,
              esp_chip_addr);
      // sprintf(basic_url,"http://%s/api/v1/wifi-sensor?address=%s" ,SERVER_IP, esp_chip_addr);
      Serial.println(basic_url);
      http.begin(client, basic_url);  // HTTP
      http.addHeader("Content-Type", "application/json");
    } else {
      // sprintf(basic_url,"http://%s/api/v1/basic?address=%s" ,FARMLABS_URL, addr);
      sprintf(basic_url, "https://%s/?address=%s", FARMLABS_URL_AWS_API_GATEWAY_SENSOR_TYPE,
              esp_chip_addr);  // 07.01
      // sprintf(basic_url,"%s/?address=%s" ,FARMLABS_URL_AWS_API_GATEWAY, esp_chip_addr);   // 07.01

      net_aws.setCACert(AWS_CERT_CA);  // Root CA 1 cer
      http.begin(net_aws, basic_url);  // HTTP

      http.addHeader("x-api-key", (String)X_API_KEY_API_GATEWAY_SENSOR_TYPE);
      http.addHeader("Content-Type", "application/json");
    }

    int httpCode = http.GET();
    Serial.print("[HTTP] GET...\n");

    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      sprintf(get_code_buf, "[HTTP] GET... code: %d\n", httpCode);
      Serial.println(get_code_buf);

      const String& payload = http.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        digitalWrite(OUTPIN_1, LOW);

        strcpy(str_buf, payload.c_str());  // web log

        StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc, payload);
        // Test if parsing succeeds.
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }

        // value ["J","K"]
        const char* return_code = doc["code"];
        char response_code[] = "0000";
        int ret = strcmp(return_code, response_code);
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        _newSensor->member = doc["result"].size();
        Serial.print("result value_member : ");
        Serial.println(_newSensor->member);

        const char* data;

        for (int ab = 0; ab < doc["result"].size(); ab++) {
          data = doc["result"][ab];
          Serial.print("data value :");
          Serial.println(data);
          _newSensor->type_str[ab] = data;
          char* ptr = strchr(Sersor_CODE_array, *data);
          // char* ptr = strchr(array_1bytevalue, *data);

          if (ptr != NULL) {
            _newSensor->type[ab] = *ptr;
            if (*ptr == 'c')
              _newSensor->member += 2;  // Serial.println("key 'c' input");

            Serial.print("newSensor->type:");
            Serial.println(_newSensor->type[ab]);

            Serial.print("newSensor->type_str:");
            Serial.println(_newSensor->type_str[ab]);
          }
        }
        // free(_newSensor);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        if (!ret) {
          start_Get_flag = 1;  /// get_send_return ok!
        } else {
          start_Get_flag = 0;  /// get_send_return ok!
        }
      } else {
        digitalWrite(OUTPIN_1, HIGH);
        start_Get_flag = 0;  /// get_send_return ok!
      }

    } else {
      start_Get_flag = 0;  /// get_send_return ok!
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      Serial.println("[HTTP] GET..failed");
      digitalWrite(OUTPIN_1, HIGH);
    }
    http.end();
    client.flush();
    // client.stop();

  } else if (WiFi.status() ==
             WL_IDLE_STATUS) {  // it is a temporary status assigned when WiFi.begin() is called and remains active
                                // until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a
                                // connection is established (resulting in WL_CONNECTED);
    Serial.println("WL_IDLE_STATUS");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    wifi_autoconnect();
    // induce wdt !!
  } else if (WiFi.status() == WL_NO_SSID_AVAIL) {  // assigned when no SSID are available
    Serial.println("WL_NO_SSID_AVAIL");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    wifi_autoconnect();
    // induce wdt !!
  } else if (WiFi.status() == WL_CONNECT_FAILED) {  // assigned when the connection fails for all the attempts
    Serial.println("WL_CONNECT_FAILED");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    wifi_autoconnect();
    // induce wdt !!
  } else if (WiFi.status() == WL_CONNECTION_LOST) {  // assigned when the connection is lost
    Serial.println("WL_CONNECTION_LOST");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    wifi_autoconnect();
    // induce wdt !!
  } else if (WiFi.status() == WL_DISCONNECTED) {  // assigned when disconnected from a network;
    Serial.println("WL_DISCONNECTED");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    start_Get_flag = 0;  /// get_send_return ok!

    wifi_autoconnect();

  } else {
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);

    wifi_autoconnect();
  }
}

void Httpdatesend_GET_UPDATE_REQUEST() {
  WiFiClient client;
  HTTPClient http;

  // sprintf(basic_url,"http://%s/api/v1/basic?address=%s" ,FARMLABS_URL, addr);
  sprintf(basic_url, "https://%s/?address=%s&version=%s", FARMLABS_URL_AWS_API_GATEWAY_SENSOR_UPDATE, esp_chip_addr,
          VERSION_PROGRAM);  // 07.01
  // sprintf(basic_url,"%s/?address=%s" ,FARMLABS_URL_AWS_API_GATEWAY, esp_chip_addr);   // 07.01

  net_aws.setCACert(AWS_CERT_CA);  // Root CA 1 cer
  http.begin(net_aws, basic_url);  // HTTP
  http.addHeader("x-api-key", (String)X_API_KEY_API_GATEWAY_SENSOR_UPDATE);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.GET();
  Serial.print("[HTTP] GET...\n");
  Serial.printf(basic_url);

  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled

    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    const String& payload = http.getString();
    Serial.println("received payload:\n<<");
    Serial.println(payload);
    Serial.println(">>");

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      strcpy(str_buf, payload.c_str());

      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

      const char* return_code = doc["code"];
      char response_code[] = "0000";
      int ret = strcmp(return_code, response_code);
      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      if (!ret) {
        /* update loop */
        otadive_update();
      }

      ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    }

  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    Serial.println("[HTTP] GET..failed");
  }

  http.end();
  client.flush();
}

void Httpdatesend_POST(linkedList* L, Sensor_type* newSensor) {
  memset(send_str, 0x00, sizeof(char) * 255);
  memset(dst_send_str, 0x00, sizeof(char) * 255);
  memset(dst_send_str_time, 0x00, sizeof(char) * 255);
  char tail[] = { "}}" };

  if ((WiFi.status() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;

    digitalWrite(OUTPIN_3, LOW);
    digitalWrite(OUTPIN_7, HIGH);

    SLOGI(TAG, "RSSI : %d", WiFi.RSSI());
    // LOGI(TAG, "IP : %s", WiFi.localIP().c_str());
    //  Serial.print("IP : ");
    //  Serial.println(WiFi.localIP());

    // SLOGI(TAG, "[HTTP] begin...");
    if (serverinfo.NEW_SERVER_NUM == 1) {
      sprintf(basic_url, "http://%s:%d/api/v1/wifi-sensor", serverinfo.NEW_IP, serverinfo.NEW_PORT);
      SLOGI(TAG, "URL:%s", basic_url);
      http.begin(client, basic_url);  // HTTP
      http.addHeader("Content-Type", "application/json");

    } else {
      // http.begin(client, "http://" FARMLABS_URL "/api/v1/basic"); //HTTP
      sprintf(basic_url, "https://%s", FARMLABS_URL_AWS_API_GATEWAY_SENSOR_DATA);  // 07.01
      SLOGI(TAG, "URL:%s", basic_url);
      net_aws.setCACert(AWS_CERT_CA);  // Root CA 1 cer
      http.begin(net_aws, basic_url);  // HTTP
      http.addHeader("x-api-key", (String)X_API_KEY_API_GATEWAY_SENSOR_DATA);
      http.addHeader("Content-Type", "application/json");
    }
    // SLOGI(TAG, "[HTTP] POST...send Sensor date");
    ////////////////////////////////////////////////////////////////////////////////
    // herder
    sprintf(str, "{\"address\":\"%s\",\"values\":{", esp_chip_addr);
    strcat(send_str, str);
    strcat(send_str, send_data_node);  // sensor value
    // add date
    sprintf(dst_send_str_time, "\"time\":\"%s\",", UTC.dateTime(ATOM).c_str());
    strcat(send_str, dst_send_str_time);
    // tail
    strncpy(dst_send_str, send_str, (strlen(send_str)) - 1);
    strcat(dst_send_str, tail);

    SLOGI(TAG, "SEND_DATA : %s", dst_send_str);
    // send json message!!

    int httpCode = http.POST(dst_send_str);
    //////////////////////////////////////////////////////////////////////////////

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      SLOGI(TAG, "POST...HTTP_CODE : %d", httpCode);

      const String& payload = http.getString();
      SLOGI(TAG, "POST received payload : %s", payload.c_str());

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        digitalWrite(OUTPIN_1, LOW);  // Server status checked!!

        StaticJsonDocument<500> doc;
        DeserializationError error = deserializeJson(doc, payload);
        // Test if parsing succeeds.
        if (error) {
          SLOGI(TAG, "deserializeJson() failed: %s", error.c_str());
          // Serial.print(F("deserializeJson() failed: "));
          // Serial.println(error.c_str());
          return;
        }
        const char* trens_code = doc["code"];
        const char* trens_meg = doc["result"];
        char response_code[] = "0000";
        int ret = strcmp(trens_code, response_code);

        if (!ret) {
          start_Get_flag = 1;  /// get_send_return ok!
        } else {
          start_Get_flag = 0;  /// retry!
        }
      } else {
        start_Get_flag = 0;  /// get_send_return ok!
        digitalWrite(OUTPIN_1, HIGH);
      }
    } else {
      SLOGI(TAG, "[HTTP] POST... failed, error: %s", http.errorToString(httpCode).c_str());

      start_Get_flag = 0;  /// retry!

      digitalWrite(OUTPIN_7, LOW);
      digitalWrite(OUTPIN_3, HIGH);
      vTaskDelay(500);
      digitalWrite(OUTPIN_7, HIGH);
      digitalWrite(OUTPIN_3, LOW);
      vTaskDelay(500);
      digitalWrite(OUTPIN_7, LOW);
      digitalWrite(OUTPIN_3, HIGH);
      vTaskDelay(500);
      digitalWrite(OUTPIN_7, HIGH);
      digitalWrite(OUTPIN_3, LOW);
      vTaskDelay(500);
      digitalWrite(OUTPIN_7, LOW);
      digitalWrite(OUTPIN_3, HIGH);
    }
    http.end();
    client.flush();

  } else if (WiFi.status() ==
             WL_IDLE_STATUS) {  // it is a temporary status assigned when WiFi.begin() is called and remains active
                                // until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a
                                // connection is established (resulting in WL_CONNECTED);
    SLOGI(TAG, "WL_IDLE_STATUS : %d", WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    digitalWrite(OUTPIN_7, LOW);
    start_Get_flag = 0;
    wifi_autoconnect();
  } else if (WiFi.status() == WL_CONNECTION_LOST)  // assigned when the connection is lost
  {
    SLOGI(TAG, "WL_IDLE_STATUS : %d", WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    digitalWrite(OUTPIN_7, LOW);
    start_Get_flag = 0;
    wifi_autoconnect();
  } else {
    SLOGI(TAG, "WL_ERROR_CODE : %d", WiFi.status());
    start_Get_flag = 0;
    digitalWrite(OUTPIN_3, HIGH);
    digitalWrite(OUTPIN_7, LOW);
    wifi_autoconnect();
  }
}

void AWS_IOT_PUBLISH(linkedList* L) {
  char send_str[255] = { 0 };
  char dst_send_str[255] = { 0 };
  char tail[] = { "}}}" };
  char desired[] = { "{\"state\":{\"desired\": " };
  char reported[] = { "{\"state\":{\"reported\": " };

  /////////////////////////////////////Json_string_start////////////////////////////////////////////////////////
  strcat(send_str, reported);

  sprintf(str, "{\"address\":\"%s\",", esp_chip_addr);
  strcat(send_str, str);
  strcat(send_str, send_data_node);

  // tail
  strncpy(dst_send_str, send_str, (strlen(send_str)) - 1);
  strcat(dst_send_str, tail);

  Serial.println(dst_send_str);

  // AWS send messge!!
  // DiconnetAWS();
  connected_check();
  AWS_IOT_shadow_update_SensorNode(dst_send_str);
  AWS_IOT_publishMessage_SensorNode(dst_send_str);

  // publishMessage();
  /////////////////////////////////////Json_string_end////////////////////////////////////////////////////////
}

void vTaskCode_web(void* pvParameters) {
  const TickType_t xDelay_tt = 100 / portTICK_PERIOD_MS;
  for (;;) {
    vTaskDelay(xDelay_tt);
    Portal.handleClient();
    Mqtt_loop();
  }
}

void vTast_CPU_0_Function(void) {
  xTaskCreatePinnedToCore(vTaskCode_web, "vTaskCode_web", 20000, NULL, 1, NULL, 1);  // cpu
  msg_Queue = xQueueCreate(mag_queue_len, sizeof(Autoconnet_Web_setting));
}

void setup() {
  pinMode(OUTPIN_1, OUTPUT);
  pinMode(OUTPIN_2, OUTPUT);
  pinMode(OUTPIN_3, OUTPUT);
  pinMode(OUTPIN_4, OUTPUT);
  pinMode(OUTPIN_5, OUTPUT);
  pinMode(OUTPIN_6, OUTPUT);
  pinMode(OUTPIN_7, OUTPUT);

  Serial.begin(9600);
  Serial.setTimeout(SERIAL_TIME_OUT);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  chipid = ESP.getEfuseMac();  // The chip ID is essentially its MAC address(length: 6 bytes).
  sprintf(str, "%08X", htonl((uint32_t)chipid));
  strncpy(esp_chip_addr, str, 8);
  sprintf(str, "%04X", htons((uint16_t)(chipid >> 32)));
  strcat(esp_chip_addr, str);

  LOGI(TAG, "ESP32_arduino_paltform");
  LOGI(TAG, "chip_id %s", esp_chip_addr);

  digitalWrite(OUTPIN_1, LOW);  // init
  digitalWrite(OUTPIN_2, LOW);
  digitalWrite(OUTPIN_3, LOW);
  digitalWrite(OUTPIN_5, LOW);
  digitalWrite(OUTPIN_6, HIGH);
  digitalWrite(OUTPIN_7, HIGH);
  delay(1500);                   // basic init time
  digitalWrite(OUTPIN_1, HIGH);  // init
  digitalWrite(OUTPIN_2, HIGH);
  digitalWrite(OUTPIN_3, HIGH);
  digitalWrite(OUTPIN_5, HIGH);
  digitalWrite(OUTPIN_6, LOW);
  digitalWrite(OUTPIN_7, LOW);
  delay(1500);  // basic init time

  LOGI(TAG, "VERSION_PROGRAM %s", VERSION_PROGRAM);

  vTaskDelay(2000);  // basic init time
                     //////////////////////////////////////////////////////////////////////////////
  syslog_init();
  spiffs_init();
  // spiffs_format();

  /////////////////////////////////////////////////////////////////////////////
  // 1ms timer
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);

  /////////////// wifi_autoconnect Task Watchdog Timer init///////////////
  // esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts

  esp_task_wdt_init(300, true);  // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);        // add current thread to WDT watch

  Basic_Sonser_Get();
  do {
    Basic_Sonser_Get();
  } while (!get_addr_flg);

  ///////////////////////////////eeprom/////////////////////////////////
  eeprom_init();
  eeprom_read_data();
  if (serverinfo.NEW_SERVER_NUM == 1)
    digitalWrite(OUTPIN_5, LOW);
  else
    digitalWrite(OUTPIN_5, HIGH);

  ///////////////////////////////eeprom/////////////////////////////////

  Serial.println(isspace(select_sensor_info.SELECT_TYPE[0]));

  LOGI(TAG, "Input_sensor_page");
  Portal.load(FPSTR(esp32_web_service.input_sensor_page));
  Portal.on("/Select_sensor_check", checkSensor);

  wifi_autoconnect();
  // AWS_OTA_UPDATE();
  otadive_update();

  /////////////// wifi_autoconnect Task Watchdog Timer init out///////////////

  if (esp_task_wdt_delete(NULL) == ESP_OK) {
    delay(1000);
    Serial.println("wifi_autoconnect wdt delate!!");
  }

  /////////////// main loop Task Watchdog Timer init ///////////////

  esp_task_wdt_init(WDT_TIMEOUT, true);  // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);                // add current thread to WDT watch

  while (esp_task_wdt_status(NULL) == ESP_ERR_INVALID_STATE) {};
  Serial.println("main loop init wdt init!!");

  //////////////////////////////////////////////////////////////////////

  // Using Web Browser PAGE_AUTH
  config.auth = AC_AUTH_DIGEST;
  config.authScope = AC_AUTHSCOPE_PORTAL;
  config.username = "farmlabs";
  config.password = "123qwe";
  Portal.config(config);
  // Portal.load(FPSTR(PAGE_AUTH));

  // Using Web Browser OTA UPdate
  config.ota = AC_OTA_BUILTIN;  // ota update add!!
  Portal.config(config);        // page add!!

  /////////////////////////////////////////////////////////////////////

  Portal.load(FPSTR(esp32_web_service.input_page));
  Portal.on("/check", checkIPAddress);

  Portal.load(FPSTR(esp32_web_service.Sensor_cal));
  Server.on("/PH_PV_read", PH_PV_read);
  Server.on("/PH_918_result", PH_918_compleat);
  Server.on("/PH_686_result", PH_686_compleat);
  Server.on("/PH_400_result", PH_400_compleat);

  Server.on("/EC_PV_read", EC_PV_read);
  Server.on("/EC_1413_result", EC_1413_compleat);

  Portal.load(esp32_web_service.auxPage);

  Portal.on("/echo", [](AutoConnectAux& aux, PageArgument& args) {
    AutoConnectText& ac_echo = aux.getElement<AutoConnectText>("echo");
    AutoConnectText& ac_echo_1 = aux.getElement<AutoConnectText>("echo_1");

    sprintf(str_buf, "SETTING : %s:%d / %s / %d", serverinfo.NEW_IP, serverinfo.NEW_PORT,
            select_sensor_info.SELECT_TYPE, serverinfo.NEW_SERVER_SEND_DELAY_SEC);

    if (serverinfo.NEW_SERVER_NUM == 1) {
      sprintf(str_empty, " / %s", "LOCAL");
      strcat(str_buf, str_empty);
    } else if (serverinfo.NEW_SERVER_NUM == 2) {
      sprintf(str_empty, " / %s", "MINI");
      strcat(str_buf, str_empty);
    } else {
      sprintf(str_empty, " / %s", "AWS");
      strcat(str_buf, str_empty);
    }

    if (serverinfo.NEW_RESET_SET == 1) {
      sprintf(str_empty, " / %s", "RESET");
      strcat(str_buf, str_empty);
    } else {
      sprintf(str_empty, " / %s", "NON");
      strcat(str_buf, str_empty);
    }

    sprintf(str_empty, " / %s", serverinfo.NEW_RESET_TIME);
    strcat(str_buf, str_empty);

    if (serverinfo.NEW_SET_MODE == 1) {
      sprintf(str_empty, " / %s", "REAL_MODE");
      strcat(str_buf, str_empty);
    } else {
      sprintf(str_empty, " / %s", "TEST_MODE");
      strcat(str_buf, str_empty);
    }

    ac_echo.value = basic_url;
    ac_echo_1.value = str_buf;
    ac_echo_1.style = "color:red;";
    return String();
  });

  Portal.on("/input_page", [](AutoConnectAux& aux, PageArgument& args) {
    AutoConnectInput& input_ip = aux.getElement<AutoConnectInput>("ipaddress");
    AutoConnectInput& input_port = aux.getElement<AutoConnectInput>("portNum");
    AutoConnectRadio& radio_select = aux.getElement<AutoConnectRadio>("period");
    AutoConnectRadio& radio_select_server = aux.getElement<AutoConnectRadio>("server_selsect");
    AutoConnectRadio& radio_set_mode = aux.getElement<AutoConnectRadio>("set_mode");
    AutoConnectRadio& radio_reset_set = aux.getElement<AutoConnectRadio>("reset_set");
    AutoConnectInput& input_reset_time = aux.getElement<AutoConnectInput>("reset_time");

    radio_reset_set.checked = serverinfo.NEW_RESET_SET;
    input_reset_time.value = serverinfo.NEW_RESET_TIME;

    switch (serverinfo.NEW_SERVER_SEND_DELAY_SEC) {
      case 10: radio_select.checked = serverinfo.NEW_SERVER_SEND_DELAY_SEC / 10; break;
      case 30: radio_select.checked = serverinfo.NEW_SERVER_SEND_DELAY_SEC / 15; break;
      case 60: radio_select.checked = serverinfo.NEW_SERVER_SEND_DELAY_SEC / 20; break;
      case 180: radio_select.checked = serverinfo.NEW_SERVER_SEND_DELAY_SEC / 45; break;
      default: break;
    }

    radio_set_mode.checked = serverinfo.NEW_SET_MODE;
    radio_select_server.checked = serverinfo.NEW_SERVER_NUM;
    input_ip.value = serverinfo.NEW_IP;
    input_port.value = serverinfo.NEW_PORT;
    return String();
  });

  Server.on("/clock", auxClock);
  Server.on("/status_log", auxStatus);
  Server.on("/status_log_heap", auxStatus_heap);
  Server.on("/status_log_real_time_data", auxStatus_real_time_data);
  Server.on("/flow_real_time_data", auxFlowRealTimeData);
  Server.on("/get_code_check", auxStatus_get_code_check);
  Server.on("/post_code_check", auxStatus_post_code_check);

  ////////////////time/////////////////////////////////////////////////

  // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  waitForSync();
  Serial.println("UTC: " + UTC.dateTime());
  NewSeoul.setLocation("Asia/Seoul");
  Serial.println("Asia/Seoul time: " + NewSeoul.dateTime());

  //////////////////////////////////////////////////////////////////////

  // Portal.disableMenu(AC_MENUITEM_OPENSSIDS);
  // Portal.begin();                   // Step #6.d

  //////////////////////////////////////////////////////////////////////
  ConnectAWS();
  // AWS_OTA_UPDATE();

  //////////////////////////////////////////////////////////////////////
  vTast_CPU_0_Function();

  //////////////////////////////////////////////////////////////////////

  // heap_trace_init_standalone(trace_record, NUM_RECORDS);
  if (!async_webserver_start()) {
    LOGI(TAG, "Success_async_webserver_start");
  } else {
    LOGI(TAG, "Error_async_webserver_start");
  }

}
  

void loop() {
  Sensor_type* newSensor = (Sensor_type*)malloc(sizeof(Sensor_type));
  Autoconnet_Web_setting* _Auto_setting = (Autoconnet_Web_setting*)malloc(sizeof(Autoconnet_Web_setting));
  linkedList* L = (linkedList*)malloc(sizeof(linkedList));

  memset(newSensor, 0x00, sizeof(Sensor_type));
  memset(_Auto_setting, 0x00, sizeof(Autoconnet_Web_setting));
  memset(L, 0x00, sizeof(linkedList));

  // dbg_syslog();
  // dbg_syslog_len_count();
  // listDir();
  // readFile("/log_1.txt");

  while (1) {
    // if(!start_Get_flag) Httpdatesend_GET_NEW(newSensor);
    //  Serial.println(isspace(newSensor->type[0]));

    if (WiFi.status() == WL_DISCONNECTED) {
      wifi_autoconnect();
    }

    if (newSensor->type[0] != NULL)
      start_Get_flag = 1;  // ??�듬갚寃??�?
    else
      start_Get_flag = 0;

    strcpy(newSensor->type, select_sensor_info.SELECT_TYPE);  // key cp
    esp_task_wdt_reset();                                     // wdt reset
    digitalWrite(OUTPIN_2, HIGH);                             // status live LED

    ///////////////////////////// reset_time /////////////////////////////////////
    if (NewSeoul.dateTime("H:i") == serverinfo.NEW_RESET_TIME && serverinfo.NEW_RESET_SET == 1 && !SYS_RESET_FLAG) {
      SYS_RESET_FLAG = 1;
      vTaskDelay(1000 * 50);
      ESP.restart();
    } else
      SYS_RESET_FLAG = 0;
    ///////////////////////////// reset_time /////////////////////////////////////

    ///////////////2021.6.03 Queue add!!///////////////////////////////////

    if (xQueueReceive(msg_Queue, &(_Auto_setting), 0) == pdTRUE) {
      SLOGI(TAG, "RECEIVE TASK_1!!!");
      SLOGI(TAG, "SET_IP : %s", _Auto_setting->ip);
      SLOGI(TAG, "SET_PORT : %s", _Auto_setting->port);
      SLOGI(TAG, "SET_MEMBER : %s", _Auto_setting->member);
      SLOGI(TAG, "SET_DELAY_TIME : %s", _Auto_setting->delay_time);
      SLOGI(TAG, "SET_SERVER_NUM : %s", _Auto_setting->server_num);
    }

    ///////////////2021.6.03///////////////////////////////////

    if (interruptCounter > 0) {
      portENTER_CRITICAL(&timerMux);
      interruptCounter--;
      portEXIT_CRITICAL(&timerMux);

      memset(print_str, 0x00, sizeof(print_str));
      memset(send_data_node, 0x00, sizeof(send_data_node));

      if (serverinfo.NEW_SET_MODE == 1) {
        if (strchr(select_sensor_info.SELECT_TYPE, 'f') != NULL) {
          Sensor_type flowSensor = {};
          flowSensor.type[0] = 'f';
          flowSensor.type[1] = '\0';

          Basic_Sonser_Get_last(L, &flowSensor);
          strncpy(flow_data_node_buf, send_data_node, sizeof(flow_data_node_buf) - 1);
          flow_data_node_buf[sizeof(flow_data_node_buf) - 1] = '\0';
        } else {
          flow_data_node_buf[0] = '\0';
        }
        digitalWrite(OUTPIN_2, LOW);  // status live LED
      } else {
        flow_data_node_buf[0] = '\0';
        digitalWrite(OUTPIN_2, LOW);
      }  // status live LED

      heapUsed = heap_caps_get_free_size(MALLOC_CAP_8BIT);
      sprintf(str_check, "Heap used at end %d, therefore total heap used %d.\n", heapUsed, heapUsedAtStart - heapUsed);
      heapUsedAtStart = heapUsed;
    }

    if (data_send_interruptCounter_flag)  // 10==10000ms
    {
      data_send_interruptCounter_flag = 0;
      memset(send_data_node, 0x00, sizeof(send_data_node));
      // if (send_crc_check) {
      if (1) {
        switch (serverinfo.NEW_SERVER_NUM) {
          case 1:
            if (start_Get_flag) {
              Basic_Sonser_Get_last(L, newSensor);
              strncpy(send_data_node_buf, send_data_node, sizeof(send_data_node_buf) - 1);  // web log
              send_data_node_buf[sizeof(send_data_node_buf) - 1] = '\0';
              Httpdatesend_POST(L, newSensor);
              break;
            }
          case 2:
            if (start_Get_flag) {
              Basic_Sonser_Get_last(L, newSensor);
              strncpy(send_data_node_buf, send_data_node, sizeof(send_data_node_buf) - 1);  // web log
              send_data_node_buf[sizeof(send_data_node_buf) - 1] = '\0';
              Httpdatesend_POST(L, newSensor);
              //  Httpdatesend_GET_UPDATE_REQUEST();
              break;
            }
          case 3:
            if (start_Get_flag)
              AWS_IOT_PUBLISH(L);
            break;

          default: break;
        }
        start_Get_flag = 0;
        break;

      }  // send_crc_check
      else
        vTaskDelay(1000);
    }
  }

  free(L);
  free(_Auto_setting);
  free(newSensor);
}

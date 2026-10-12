//프로그램 수정 사항
// 5/19 config.autoReconnect = true;  // autoReconnect STA모드로... 추가
// '20/6/03 리카센서 RK-330 센서추가에 따른 프로그램 변경
#include <stdio.h>
#include <WiFi.h>
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>  // ArduinoJson  ver. 6
#include <AutoConnect.h>
#include <EEPROM.h>  // EEPROM.h는 AutoConnect.h 앞에 꼭 선언해야 함 그래야 컴파일 에러 안남
#include <EEPROM_def.h>
#include <WebServer.h>
//#include "esp_system.h"
#include <esp_task_wdt.h>
#include "secrets.h"
#include <global.h>
#include <time.h>
#include <ezTime.h>
#include "esp_heap_trace.h"
#include "esp_heap_caps.h"

//#define VERSION_PROGRAM  1.0 // ver 0.1
//#define VERSION_PROGRAM  1.1 // '20/5/4 박만권 적용(WIFI.status handler 보강)
//#define VERSION_PROGRAM  1.2 // '20/5/19 EC센서 추가
//#define VERSION_PROGRAM  1.31 // '20/6/03 리카센서 RK-330 센서추가에 따른 프로그램 변경 영하권 추가
//#define VERSION_PROGRAM  1.32 // '20/6/30 esp32 chip id 8자리 추가
//#define VERSION_PROGRAM  1.33 // '20/6/30 esp32 chip id 8자리 추가
//#define VERSION_PROGRAM가  1.34 // '20/7/15 외부기상대 센서 추가
//#define VERSION_PROGRAM  1.35 // '20/8/03 RK500-13 추가 (EC/지온/지습)
//#define VERSION_PROGRAM  1.36 // h/w wtd 추가 / Using Web Browser OTA Update
//#define VERSION_PROGRAM  1.37 // wtd logic 변경
//#define VERSION_PROGRAM  1.39 // 건습구 추가, 코린스 추가
//#define VERSION_PROGRAM  "1.9.0.0" // task 추가
//#define VERSION_PROGRAM  "2.0.0.1" // aws api-gateway key 추가 분석모델 // 2021.07.26 최신버전
// otadrive update 추가 분석 모델만
#define VERSION_PROGRAM \
  "3.0.0.0"  // WIFI 보드 GLE21E010203
             // GET 제거, 센서선택 URL 추가
             // 센서노드 소형화 LED 컨트롤러 만
             // otadrive update 초기화시(reboot 시) 1회
             // ezTime 추가 UTC-ISO8601 추가(POST 시 타임랩스 추가)

//   #define VERSION_PROGRAM  2.05 // task 추가

unsigned int CRC16_NOH, SEED, GP, temp_CRC16_NOH;  // for CRC16
#define SEED 0xFFFF                                // initialization for CRC16
#define GP 0xA001                                  // generating polynomial

size_t ret = 0;

size_t heapUsed;
size_t heapUsedAtStart;

uint8_t send_crc_check = 0;
uint8_t start_Get_flag = 0;
char basic_url[255] = {
  0,
};
char get_code_buf[30] = {
  0,
};
char post_code_buf[30] = {
  0,
};
char send_str[255] = {
  0,
};
char dst_send_str[255] = {
  0,
};
char dst_send_str_time[255] = {
  0,
};
char str_check[255] = {
  0,
};
;
char str[255] = {
  0,
};
char str_buf[255] = {
  0,
};

int get_addr_flg = 0;
char addr_test[] = "77777777";
int korins_value[7];
char esp_chip_addr[20];
char esp_chip_addr_print[8];
char addr[20] = {
  0,
};

char sign = 0;
uint64_t chipid;

extern char print_str[20];
extern char send_data_node[255];
char send_data_node_buf[255];

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
// UTF + 9 (ex For UTC +1.00 : 1 * 60 * 60 : 3600) // 그리니치 표준시 보다 9시간 빠름 const int   daylightOffset_sec =
// 0; //Change the Daylight offset in milliseconds. If your country observes Daylight saving time set it to 3600.
// Otherwise, set it to 0.

////////// UTC //////
const long gmtOffset_sec = 0;  // list of UTC time offsets. Here are some examples for different timezones:  (ex For UTC
                               // +1.00 : 1 * 60 * 60 : 3600) // 그리니치 표준시
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

static const char PAGE_AUTH[] PROGMEM = R"(
{
  "uri": "/auth",
  "title": "Auth",
  "menu": true,
  "element": [
    {
      "name": "text",
      "type": "ACText",
      "value": "AutoConnect has authorized",
      "style": "font-family:Arial;font-size:18px;font-weight:400;color:#191970"
    }
  ]
}
)";

static const char input_sensor_page[] PROGMEM = R"(
[
 {
  "uri": "/select_sensor",
  "title": "센서설정",
  "menu": true,
  "element":
   [
     {
        "name": "header",
        "type": "ACText",
        "value": "<h2>센서등록설정</h2>",
        "style": "text-align:left;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "tablestyle",
        "type": "ACStyle",
        "value": "table.style
        {
          font-family:arial,sans-serif;
          border-collapse:collapse;
          width:100%;
          color:black;
        }
        table.style td,table.style th
        {
          border:1px solid #dddddd;
          text-align:left;
          padding:8px;
        }
        table.style tr:nth-child(even)
        {
          background-color:#ffffff;
        }"
      },
      
     {
        "name": "sensor",
        "type": "ACElement",
        "value": "<table class='style'>      
                    <tr>
                    <td>구 분</td>
                    <td>센서 종류</td>
                    <td>센서 코드</td>
                    <td>체크</td>
                    </tr>   
                    <tr>
                    <td rowspan = '4'>온/습도 센서</td>
                    <td>백엽상일체형 센서, 리카(RK330-01) 센서</td>
                    <td> J , K </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_1'> 선택</td>
                    </tr>
                    <tr>
                    <td>건/습구 센서</td>
                    <td> Y , L </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_2'> 선택</td>
                    </tr>
                    <tr>
                    <td>서리 온도/습도/노점온도 센서</td>
                    <td> U , r , d </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_3'> 선택</td>
                    </tr>
                    <tr>
                    <td>엽온 센서</td>
                    <td> l(소문자) </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_1_4'> 선택</td>
                    </tr>
                   
                    <tr>
                    <td rowspan = '3'>Co2 센서</td>
                    <td>Co2(KCD-AN300) 센서</td>
                    <td> C </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_2_1'> 선택</td>
                    </tr>
                    <tr>
                    <!--<td>Co2(KCD-AN300)</td>-->
                    <td>Co2(KCD-HP100) 센서</td>
                    <td> c </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_2_2'> 선택</td>
                    </tr>
                    <tr>
                    <!--<td>Co2(KCD-AN300)</td>-->
                    <td>Co2(SH-VT260) 센서</td>
                    <td> i , o , p </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_2_3'> 선택</td>
                    </tr>
                 
                    <tr>
                    <td rowspan = '5'>기상대 센서</td>
                    <td>일사량(코리아 디지털, SWSR-7500) 센서</td>
                    <td> G </td> 
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_1'> 선택</td>
                    </tr>
                    <tr>
                    <td>리카 풍향(RK110-02) 센서</td>
                    <td> V </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_2'> 선택</td>
                    </tr>
                    <tr>
                    <td>리카 풍속(RK100-02) 센서</td>
                    <td> X </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_3'> 선택</td>
                    </tr>
                    <tr>
                    <td>우적 센서</td>
                    <td> R </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_4'> 선택</td>
                    </tr>
                    <tr>
                    <td>코린스 센서</td>
                    <td>y,O,g,A,s,e,Q </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_3_5'> 선택</td>
                    </tr>

                    <tr>
                    <td rowspan = '2'>용액용 센서</td>
                    <td>리카 PH(RK500-02) 센서</td>
                    <td> j </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_4_1'> 선택</td>
                    </tr>
                    <tr>
                    <!--<td>Co2(KCD-AN300)</td>-->
                    <td>리카 EC,수온(RK500-13) 센서</td>
                    <td> x , 1 </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_4_2'> 선택</td>
                    </tr>
                                   
                    <tr>
                    <td rowspan = '3'>토지용 센서</td>
                    <td>리카 PH(RK500-02) 센서</td>
                    <td> j </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_5_1'> 선택</td>
                    </tr>
                    <tr>
                    <td>리카 EC/지온/지습(RK520-02) 센서</td>
                    <td> v , n , q </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_5_2'> 선택</td>
                    </tr>
                    <tr>
                    <td>미래 EC/지온/지습(RK520-02) 센서</td>
                    <td> E , m , u </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_5_3'> 선택</td>
                    </tr>

                    <tr>
                    <td rowspan = '5'>감지 센서</td>
                    <td>수위감지 센서</td>
                    <td> w </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_1'> 선택</td>
                    </tr>
                    <tr>
                    <td>창문감지 센서</td>
                    <td> N </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_2'> 선택</td>
                    </tr>
                    <tr>
                    <td>화재감지 센서</td>
                    <td> F </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_3'> 선택</td>
                    </tr>
                    <tr>
                    <td>정전알림 센서</td>
                    <td> b </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_4'> 선택</td>
                    </tr>
                    <tr>
                    <td>출입문감지 센서</td>
                    <td> M </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_6_5'> 선택</td>
                    </tr>

                    <tr>
                    <td rowspan = '2'>기타 센서</td>
                    <td>Battery 잔량체크 센서</td>
                    <td> B </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_7_1'> 선택</td>
                    </tr>
                    <tr>
                    <td>유량 체크 센서</td>
                    <td> f </td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_7_2'> 선택</td>
                    </tr>

                    <tr>
                    <td>해 제</td>
                    <td>모든 설정센서 해제</td>
                    <td>없음</td>
                    <td><input type='checkbox' id='picker_0' name='SET_SENSOR_8_1'> 선택</td>
                    </tr>

                  </table>"
      },

      {
        "name": "send",
        "type": "ACSubmit",
        "value": "SEND",
        "uri": "/Select_sensor_check"
      }
    ]
  },

 {
    "title": "Sensor_result",
    "uri": "/Select_sensor_check",
    "menu": false,
    "element": 
    [
      {
        "name": "result",
        "type": "ACText"
        
      },
      {
        "name": "result_2",
        "type": "ACText"
        
      }
    ]
  }
]
)";

static const char input_page[] PROGMEM = R"raw(
[
  {
    "title": "환경설정",
    "uri": "/input_page",
    "menu": true,
    "element": 
    [
     {
        "name": "header",
        "type": "ACText",
        "value": "<h2>환경 설정</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },

      {
        "name": "set_mode",
        "type": "ACRadio",
        "value": [
          "REAL_MODE",
          "TEST_MODE"
        ], 
        "arrange": "horizontal",
        "checked": 1
      },
      {
        "name": "server_selsect",
        "type": "ACRadio",
        "value": [
          "로컬서버",
          "분석모델서버",
          "AWS서버"
        ], 
        "arrange": "horizontal",
        "checked": 1
      },

      {
        "name": "ipaddress",
        "type": "ACInput",
        "label": "IP Address",
        "pattern": "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$"
      },

      {
        "name": "portNum",
        "type": "ACInput",
        "label": "PORT",
        "pattern": "^[0-9]{4}$"
      },

      {
        "name": "passwd",
        "type": "ACInput",
        "label": "비밀번호",
        "pattern": "^[a-zA-Z0-9]{6}$"
      },
     

      {
        "name": "period",
        "type": "ACRadio",
        "value": [
          "10 sec.",
          "30 sec.",
          "60 sec.",
          "180 sec."
        ],
        "label": "데이터 전송시간",
        "arrange": "vertical",
        "checked": 1
      },

      {
        "name": "send",
        "type": "ACSubmit",
        "value": "SEND",
        "uri": "/check"
      }
    ]
  },

  {
    "title": "IP Address",
    "uri": "/check",
    "menu": false,
    "element": [
      {
        "name": "result",
        "type": "ACText"
       
      }
    
    ]
  }
]
)raw";

const static char Sensor_cal[] PROGMEM = R"raw(
[
  { "title":"CAL설정", "uri":"/calibration", "menu":true, "element":[
    
      {
          "name": "tablestyle",
          "type": "ACStyle",
          "value": "table.style
          {
            font-family:arial,sans-serif;
            border-collapse:collapse;
            width:100%;
            color:black;
          }

          table.style td,table.style th
          {
            border:1px solid #dddddd;
            text-align:left;
            padding:8px;
          }

          table.style tr:nth-child(even)
          {
            background-color:#ffffff;
          }"
      },

      {
        "name": "header",
        "type": "ACText",
        "value": "<h3>RK500-02_PH_CAL</h3>",
        "style": "text-align:left;color:#2f4f4f;padding:5px;"
      },

      {
        "name": "PH_SENSOR_CAL",
        "type": "ACElement",
        "value": "<table class='style'>      
                   <tr>
                    <td rowspan = '4'>PH</td>
                    <td>현재 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(1);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_1' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>9.18 용액 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(2);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_2' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='button' id='button1' onclick='ph_918_cal_compleat();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>6.86 용액 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(3);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_3' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='button' id='button1' onclick='ph_686_cal_compleat();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>4.00 용액 값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ph_read(4);' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='PH_answer_4' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='button' id='button1' onclick='ph_400_cal_compleat();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                  </table>"
              
      },
     {
        "name": "header_1",
        "type": "ACText",
        "value": "<h3>RK500-02_PH_CAL</h3>",
        "style": "text-align:left;color:#2f4f4f;padding:5px;"
      },
     {
        "name": "EC_SENSOR_CAL",
        "type": "ACElement",
        "value": "<table class='style'>      
                   
                   <tr>
                    <td rowspan = '4'>EC</td>
                    <td>현재값 읽기</td>
                    <td><input type='button' id='button1' onclick='PV_ec_read();' value='READ' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>PV_EC</td>
                    <td>PV_T</td>
                    <td>PV_C1</td>
                    <td>PV_C2</td>
                    </tr>

                    <tr>
                    <td><input type='text' value='0' id='EC_answer_1' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='EC_answer_2' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='EC_answer_3' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td><input type='text' value='0' id='EC_answer_4' size='5' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>

                    <tr>
                    <td>CAL</td>
                    <td><input type='button' id='button1' onclick='EC_cal_step_3();' value='CAL' style='font-size:14px; margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    <td colspan='2'><input type='text' value='0' id='EC_answer_5' size='10' style='margin:1px 1px 1px 1px; padding:1px 1px 1px 1px;' /></td>
                    </tr>
                   
                  </table>"
              
      },
      {
        "name": "js",
        "type": "ACElement",
        "value": "<script type='text/javascript'>    
                  
                  var PV_ph_read_result;
                  var ph_918_handler; 
                  var ph_686_handler; 
                  var ph_400_handler;  
                  var PV_ec_read_result;  

                  function PV_ph_read(a)
                   {
                      PV_ph_read_result = new XMLHttpRequest();
                      PV_ph_read_result.onreadystatechange=function()
                      {
                        if(this.readyState==4 && PV_ph_read_result.status==200)
                          { 
                           if(a == '1') document.getElementById('PH_answer_1').value=this.responseText;                          
                           if(a == '2') document.getElementById('PH_answer_2').value=this.responseText;                          
                           if(a == '3') document.getElementById('PH_answer_3').value=this.responseText;                          
                           if(a == '4') document.getElementById('PH_answer_4').value=this.responseText;                          
                          }
                      }
                      PV_ph_read_result.open('GET', '/PH_PV_read');
                      PV_ph_read_result.send();                  
                    }
  
                  function ph_918_cal_compleat() 
                  {
                      ph_918_handler = new XMLHttpRequest();
                      ph_918_handler.onreadystatechange=function()
                      {
                        if(this.readyState==4 && ph_918_handler.status==200)
                          { 
                            document.getElementById('PH_answer_2').value=this.responseText; 
                          }                         
                      }
                      ph_918_handler.open('GET', '/918_result');
                      ph_918_handler.send();                  
                    }
                 
                  function ph_686_cal_compleat()
                   {
                      ph_686_handler = new XMLHttpRequest();
                      ph_686_handler.onreadystatechange=function()
                      {
                        if(this.readyState==4 && ph_686_handler.status==200)
                          { 
                            document.getElementById('PH_answer_3').value=this.responseText; 
                          }                         
                      }
                      ph_686_handler.open('GET', '/686_result');
                      ph_686_handler.send();       
                    } 
                  
                  function ph_400_cal_compleat() 
                  {
                    ph_400_handler = new XMLHttpRequest();
                    ph_400_handler.onreadystatechange=function()
                    {
                      if(this.readyState==4 && ph_400_handler.status==200)
                        { 
                          document.getElementById('PH_answer_4').value=this.responseText; 
                        }                         
                    }
                    ph_400_handler.open('GET', '/400_result');
                    ph_400_handler.send();       
                    }

                   function PV_ec_read()
                   {
                      PV_ec_read_result = new XMLHttpRequest();
                      PV_ec_read_result.onreadystatechange=function()
                      {
                        if(this.readyState==4 && PV_ec_read_result.status==200)
                          { 
                            var str = this.responseText;
                            var arr = str.split(',');
                            document.getElementById('EC_answer_1').value = arr[0];
                            document.getElementById('EC_answer_2').value = arr[1];
                            document.getElementById('EC_answer_3').value = arr[2];
                            document.getElementById('EC_answer_4').value = arr[3];
                          }
                      }
                      PV_ec_read_result.open('GET', '/EC_PV_read');
                      PV_ec_read_result.send();                  
                    }
                 

                  function  EC_cal_step_3() {
                    document.getElementById('EC_answer_3').value=3;
                    }

                    </script>"
      }
    
    ]
  }
]
)raw";

const static char auxPage[] PROGMEM = R"raw(
[
  { "title":"로그", "uri":"/echo", "menu":true, "element":[
    
    { "name": "time", "type": "ACText"},
    { "name":"status_log_real_time_data", "type":"ACText" },
    { "name":"status_log", "type":"ACText" },
    { "name":"post_code_check", "type":"ACText" },
    { "name":"get_code_check", "type":"ACText" },
    { "name":"status_log_heap", "type":"ACText" },
    
    {
    "name": "js_st",
    "type": "ACElement",
    "value": "<script type='text/javascript'>
    
    var xhr_get_code_check;
    var xhr_post_code_check;
    var xhr_st_real_time_data;
    var xhr_heap;
    var xhr_st;
    var xhr;

    function get_code_check(){
      xhr_get_code_check.open('GET', '/get_code_check');
      xhr_get_code_check.responseType='text';
      xhr_get_code_check.send();
      }
    
    function post_code_check(){
      xhr_post_code_check.open('GET', '/post_code_check');
      xhr_post_code_check.responseType='text';
      xhr_post_code_check.send();
      }

    function status_log_real_time_data(){
      xhr_st_real_time_data.open('GET', '/status_log_real_time_data');
      xhr_st_real_time_data.responseType='text';
      xhr_st_real_time_data.send();
      }

    function status_log_heap(){
      xhr_heap.open('GET', '/status_log_heap');
      xhr_heap.responseType='text';
      xhr_heap.send();
      }
    
    function status_log(){
      xhr_st.open('GET', '/status_log');
      xhr_st.responseType='text';
      xhr_st.send();
      }
    
    function clock(){
      xhr.open('GET', '/clock');
      xhr.responseType='text';
      xhr.send();
      }  

    window.onclose=function(){
      xhr_get_code_check.abort();
      xhr_post_code_check.abort();
      xhr_st_real_time_data.abort();
      xhr_heap.abort();
      xhr_st.abort();
      xhr.abort();
      };

    window.onload=function()
    {
        xhr_post_code_check=new XMLHttpRequest();
        xhr_post_code_check.onreadystatechange=function(){
          if(this.readyState==4&&xhr_post_code_check.status==200){
            document.getElementById('post_code_check').innerHTML=this.responseText;
            }
          };
          setInterval(post_code_check,5000);

        xhr_get_code_check=new XMLHttpRequest();
        xhr_get_code_check.onreadystatechange=function(){
          if(this.readyState==4&&xhr_get_code_check.status==200){
            document.getElementById('get_code_check').innerHTML=this.responseText;
            }
          };
          setInterval(get_code_check,5000);


        xhr_st_real_time_data=new XMLHttpRequest();
        xhr_st_real_time_data.onreadystatechange=function(){
          if(this.readyState==4&&xhr_st_real_time_data.status==200){
            document.getElementById('status_log_real_time_data').innerHTML=this.responseText;
            }
          };
          setInterval(status_log_real_time_data,1000);
          
        xhr_heap=new XMLHttpRequest();
        xhr_heap.onreadystatechange=function(){
          if(this.readyState==4&&xhr_heap.status==200){
            document.getElementById('status_log_heap').innerHTML=this.responseText;
            }
          };
          setInterval(status_log_heap,3000);

        xhr_st=new XMLHttpRequest();
        xhr_st.onreadystatechange=function(){
          if(this.readyState==4&&xhr_st.status==200){
            document.getElementById('status_log').innerHTML=this.responseText;
            }
          };
          setInterval(status_log,2000);

        xhr=new XMLHttpRequest();
        xhr.onreadystatechange=function(){
          if(this.readyState==4&&xhr.status==200){
            document.getElementById('time').innerHTML=this.responseText;
            }
          };
        setInterval(clock,2000);
      };

    </script>"},

    { "name":"echo", "type":"ACText" },
    { "name":"echo_1", "type":"ACText" }
    

    ]
  }
]
)raw";

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

void auxStatus_get_code_check() {
  Server.send(200, "text/plain", get_code_buf);
}

void auxStatus_post_code_check() {
  Server.send(200, "text/plain", post_code_buf);
}

void PH_PV_read() {
  String val;
  int count = 0;
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

  while (!Serial.available()) {
    count++;
    if (count >= 100000)
      break;
  }

  byte leng = Serial.readBytes(empty, 100);
  val = String(empty[3], DEC);
  data_val1 = val.toInt();

  val = String(empty[4], DEC);
  data_val2 = val.toInt();
  data_val_sum = (255 * data_val1) + data_val1 + data_val2;

  value_tt = (float)data_val_sum / 100;
  sprintf(empty_value, "%.2f", value_tt);
  Server.send(200, "text/plain", empty_value);
}

void PH_918_compleat() {
  int Inputs[] = { 0xFE, 0x06, 0x00, 0x5A, 0x00, 0x0A, 0x3D, 0xD1 };
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);
  Server.send(200, "text/plain", "OK");
}

void PH_686_compleat() {
  int Inputs[] = { 0xFE, 0x06, 0x00, 0x5A, 0x00, 0x0B, 0xAD, 0xD1 };
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);
  Server.send(200, "text/plain", "OK");
}

void PH_400_compleat() {
  int Inputs[] = { 0xFE, 0x06, 0x00, 0x5A, 0x00, 0x0C, 0x5D, 0xD2 };
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);
  Server.send(200, "text/plain", "OK");
}

void EC_PV_read() {
  String val;
  int count, Solution_value = 0;
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

  while (!Serial.available()) {
    count++;
    if (count >= 100000)
      break;
  }

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

  /////////////////////////// //C1 상수값 //////////////////////////////

  byte_arr[0] = (byte)empty[23];
  byte_arr[1] = (byte)empty[24];
  byte_arr[2] = (byte)empty[25];
  byte_arr[3] = (byte)empty[26];

  rdata = bytetofloat(byte_arr);
  C1 = rdata;
  sprintf(str, "%.2f,", rdata);
  strcat(empty_value, str);

  /////////////////////////// //C2 상수값 //////////////////////////////
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

  Server.send(200, "text/plain", empty_value);
}

void EC_1413_compleat() {
  int Inputs[] = { 0xFE, 0x06, 0x00, 0x5A, 0x00, 0x0C, 0x5D, 0xD2 };
  for (int i = 0; i < 8; i++)
    Serial.write(Inputs[i]);
  Server.send(200, "text/plain", "OK");
}

String checkSensor(AutoConnectAux& aux, PageArgument& arg) {
  uint8_t TOTAL_SENSOR_GROUP_MAX = 8;

  uint8_t SENSOR_GROUP_1_MAX = 4;  // 온습도 관련 센서
  uint8_t SENSOR_GROUP_2_MAX = 3;  // co2 관련센서
  uint8_t SENSOR_GROUP_3_MAX = 5;  // 기상대 센서 관련 센서
  uint8_t SENSOR_GROUP_4_MAX = 2;  // 용액용 센서 관련 센서
  uint8_t SENSOR_GROUP_5_MAX = 3;  // 토지용 센서 관련 센서
  uint8_t SENSOR_GROUP_6_MAX = 5;  // 알림 감지 관련 센서
  uint8_t SENSOR_GROUP_7_MAX = 2;  // 기타 관련 센서
  uint8_t SENSOR_GROUP_8_MAX = 1;  // 센서해제

  char str_checkSensor[15];
  char str_checkSensor_result[255];
  select_sensor_info.SELECT_MEMBER = 0;                                                  // 최기화
  memset(select_sensor_info.SELECT_TYPE, 0x00, sizeof(select_sensor_info.SELECT_TYPE));  // 초기화

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

            if (!strcmp(str_checkSensor, "SET_SENSOR_7_2")) {
              select_sensor_info.SELECT_TYPE[select_sensor_info.SELECT_MEMBER] = 'f';
              select_sensor_info.SELECT_MEMBER++;
            }

            // Serial.println(str_checkSensor);
            strcat(str_checkSensor_result, str_checkSensor);
            strcat(str_checkSensor_result, ",");
          }
        }
        /* code */
        break;

      case 8:
        for (int jj = 1; jj <= SENSOR_GROUP_8_MAX; jj++) {
          sprintf(str_checkSensor, "SET_SENSOR_%d_%d", ii, jj);
          if (arg.arg(str_checkSensor) == "on") {
            // Serial.println(str_checkSensor);
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
  EEPROM.commit();
  // eeprom saved

  result.value = "SET_SENSOR : " + (String)str_checkSensor_result;
  result.style = "color:red;";
  result_2.value = "KEY_CODE : " + (String)select_sensor_info.SELECT_TYPE;
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
      input_page.getElement<AutoConnectRadio>("server_selsect");  // 1: local, 2:분석모델, 3:aws
  AutoConnectRadio& radio_set_mode = input_page.getElement<AutoConnectRadio>("set_mode");

  IPAddress ip;
  ip.fromString(input_ip.value);
  input_ip.value = ip.toString();

  if (input_passwd.isValid() && input_passwd.value.toInt() == PASSWD_VALUE) {
    result.value = "IP Address " + input_ip.value + " is OK.";
    result.style = "";

    strcpy(serverinfo.NEW_IP, input_ip.value.c_str());
    serverinfo.NEW_PORT = input_port.value.toInt();
    serverinfo.NEW_SERVER_NUM = radio_select_server.checked;
    serverinfo.NEW_SET_MODE = radio_set_mode.checked;

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
  USE_SERIAL.println("wifi_autoconnect start1 ");
  config.title = "그린랩스_" + String(VERSION_PROGRAM);
  config.apid = "그린랩스-" + String(esp_chip_addr);  // esp chip UID print
  config.psk = "admin123";                            // 무조건 8자리 이상
  // config.psk  = esp_chip_addr;
  digitalWrite(OUTPIN_2, LOW);  // wifi_autoconnect live LED

  /*
    //config.autoReset = false;     // Not reset the module even by intentional disconnection using AutoConnect menu.
    config.autoReset = true;     // Not reset the module even by intentional disconnection using AutoConnect menu.
    config.autoReconnect = true;  // Reconnect to known access points.
    config.reconnectInterval = 6; // Reconnection attempting interval is 3[min].
    config.retainPortal = true;   // Keep the captive portal open.
  */

  // 5/19일 추가
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
  char buf[20] = {
    0,
  };

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
  char Inputs[] = {
    0x46, 0x33, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  for (int i = 0; i < 8; i++)
    Inputs[i + 3] = addr[i];

  // 12 bitset
  if (strchr(newSensor->type, 'H') != NULL || strchr(newSensor->type, 'T') != NULL)
    bitSet(Inputs[12], 7);  //온습도 일반센서(HTG3535,3513,AM2302)

  if (strchr(newSensor->type, 'C') != NULL)
    bitSet(Inputs[12], 6);  //  Co2(KCD-AN300)의 값
  if (strchr(newSensor->type, 'c') != NULL)
    bitSet(Inputs[12], 5);  //  RS-485 통신용 Co2센서(KCD-HP100) Co2,온도,습도값 요청
  if (strchr(newSensor->type, 'W') != NULL)
    bitSet(Inputs[12], 4);  //  일사량센서(미래센서, PYRARNO-70)값 요청
  if (strchr(newSensor->type, 'P') != NULL)
    bitSet(Inputs[12], 3);  //  PH센서 HAOSHI(H-101)값 요청
  if (strchr(newSensor->type, 'D') != NULL)
    bitSet(Inputs[12], 2);  //  풍향센서(QS-FX 360)값 요청
  if (strchr(newSensor->type, 'S') != NULL)
    bitSet(Inputs[12], 1);  //  풍속센서(QS-FS)값 요청
  if (strchr(newSensor->type, 'R') != NULL)
    bitSet(Inputs[12], 0);  // 우적

  // 13 bitset
  if (strchr(newSensor->type, 'E') != NULL || strchr(newSensor->type, 'm') != NULL ||
      strchr(newSensor->type, 'u') != NULL)
    bitSet(Inputs[13], 7);  // 미래센서 EC 지온 지습

  if (strchr(newSensor->type, 'B') != NULL)
    bitSet(Inputs[13], 6);  // 베터리잔량값

  if (strchr(newSensor->type, 'w') != NULL)
    bitSet(Inputs[13], 5);  // 수위감지 센서
  if (strchr(newSensor->type, 'f') != NULL)
    bitSet(Inputs[13], 4);  // 유량계 count

  if (strchr(newSensor->type, 'U') != NULL || strchr(newSensor->type, 'r') != NULL ||
      strchr(newSensor->type, 'd') != NULL)
    bitSet(Inputs[13], 3);  // 서리센서 온도, 습도, 노점온도값

  if (strchr(newSensor->type, 'N') != NULL)
    bitSet(Inputs[13], 2);  // 창문감지 센서
  if (strchr(newSensor->type, 'F') != NULL)
    bitSet(Inputs[13], 1);  // 화재감지 센서
  if (strchr(newSensor->type, 'b') != NULL)
    bitSet(Inputs[13], 0);  // 정전알림 센서

  // 14 bitset
  if (strchr(newSensor->type, 'Y') != NULL)
    bitSet(Inputs[14], 7);  // 건구
  if (strchr(newSensor->type, 'L') != NULL)
    bitSet(Inputs[14], 6);  // 습구
  if (strchr(newSensor->type, 'M') != NULL)
    bitSet(Inputs[14], 5);  // 출입감지
  if (strchr(newSensor->type, 'J') != NULL || strchr(newSensor->type, 'K') != NULL)
    bitSet(Inputs[14], 4);  //리카 RK330-1 온습도

  if (strchr(newSensor->type, 'i') != NULL || strchr(newSensor->type, 'o') != NULL ||
      strchr(newSensor->type, 'p') != NULL)
    bitSet(Inputs[14], 3);  // SH-VT260

  // if(strchr(newSensor->type,'y') != NULL) bitSet(Inputs[14], 2);// 코린스 기상대
  if (strchr(newSensor->type, 'y') != NULL || strchr(newSensor->type, 'O') != NULL ||
      strchr(newSensor->type, 'g') != NULL || strchr(newSensor->type, 'A') != NULL ||
      strchr(newSensor->type, 's') != NULL || strchr(newSensor->type, 'e') != NULL ||
      strchr(newSensor->type, 'Q') != NULL)
    bitSet(Inputs[14], 2);  // 코린스 기상대

  if (strchr(newSensor->type, 'l') != NULL)
    bitSet(Inputs[14], 1);  // 엽온센서

  // 15 bitset
  if (strchr(newSensor->type, 'G') != NULL)
    bitSet(Inputs[15], 6);  //코리아 일사
  if (strchr(newSensor->type, 'V') != NULL)
    bitSet(Inputs[15], 5);  // 리카 풍향
  if (strchr(newSensor->type, 'X') != NULL)
    bitSet(Inputs[15], 4);  // 리카 풍속
  if (strchr(newSensor->type, 'j') != NULL)
    bitSet(Inputs[15], 3);  // 리카 PH RK500-02

  if (strchr(newSensor->type, 'v') != NULL || strchr(newSensor->type, 'n') != NULL ||
      strchr(newSensor->type, 'q') != NULL)
    bitSet(Inputs[15], 2);  // 리카 EC/지온/지습

  if (strchr(newSensor->type, 'x') != NULL || strchr(newSensor->type, '1') != NULL)
    bitSet(Inputs[15], 0);  //리카용액용 EC

  Inputs[16] = XOR_CRC(Inputs, sizeof(Inputs));

  // Serial.print("wifi to node : ");

  for (int i = 0; i < 17; i++) {
    val = String(Inputs[i], DEC);
    data_len = val.toInt();
    Serial.print(data_len, HEX);
  }

  for (int i = 0; i < 17; i++)
    Serial2.write(Inputs[i]);
}

int addr_version(char* temp, byte leng) {
  char input_crc, ch;
  int crc = 0;
  String val;
  int strTodec = 0;
  unsigned int val_dec;
  int data_len = 0;
  int data_val1, data_val2, data_val_sum = 0;

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
  int val_dec_int, i;
  int count_val = 13;  //시작
  const char* ch_c;

  //// Error_Code check!////
  val = String(temp[12], DEC);
  data_len = val.toInt();
  Serial.print("Error_Code : ");
  Serial.println(data_len, HEX);

  //// Sensor_Code check!////

  ////////////////////////////////////////////////////////////////////////////////////
  L->cur = NULL;
  L->head = NULL;
  L->tail = NULL;

  Serial.print("result value_member_last: ");
  Serial.println(newSensor->member);

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

    /*
    //char* ptr_2 = strchr(array_2bytevalue, ch);
    if(ptr_ != NULL) {count_val=Sensor_parsing_2BYTE(temp,count_val, L); }
    else{count_val=Sensor_parsing_1BYTE(temp,count_val, L); }
 //   else if(ptr_2 != NULL) count_val=Sensor_parsing_1BYTE(temp,count_val, L);
 //   else continue;
*/
  }
  ////////////////////////////////////////////////////////////////////////////////////

  ////End_Code check!////

  val = String(temp[leng - 2]);
  ch = val.charAt(0);
  Serial.print("End_code : ");
  Serial.println(ch);

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
    send_crc_check = 1;

  } else {
    Serial.println("CRC_error");
    send_crc_check = 0;
  }
}

void Basic_Sonser_Get() {
  char temp[100];
  char ch;
  String val;
  int data_len = 0;
  int count = 0;

  // 시현0519
  if (!get_addr_flg) {
    get_version(addr_test);  // 시작하면 무조건 addr get 요청  77777777
    Serial.println("requst node addr!!");
  } else {
    /*
      if(http_get_EC && !http_get_TH) get_serson_value_RIKA_RK520_02(addr); //리카센서 EC/지온/지습
      else if(http_get_TH && !http_get_JK) get_serson_value(addr); //HTG3535
      else if(http_get_JK && !http_get_TH) get_serson_value_RIKA_RK330_02(addr); //리카센서 온/습도
    */
    // if(1) basic_sensor_get_last(addr);

    //  else  Serial.println("not sensor select!!");
  }

  while (!Serial2.available())
    ;
  ;

  if (Serial2.available()) {
    byte leng = Serial2.readBytes(temp, 100);

    Serial.print("Input data Lenght : ");
    Serial.println(leng);

    Serial.print("Input data : ");
    for (int i = 0; i < leng; i++) {
      val = String(temp[i], DEC);
      data_len = val.toInt();
      Serial.print(data_len, HEX);
    }
    Serial.println();

    //// datalen check!////
    val = String(temp[2], DEC);
    data_len = val.toInt();
    Serial.print("Input senson_node_data Lenght : ");
    Serial.println(data_len);

    //// address check!////

    for (int i = 0; i < 8; i++) {
      val = String(temp[3 + i]);
      addr[i] = val.charAt(0);
    }
    Serial.print("addr : ");
    Serial.println(addr);

    //// OP_Code check!////
    val = String(temp[11], DEC);
    data_len = val.toInt();
    Serial.print("OP_Code : ");
    Serial.println(data_len, HEX);

    switch (data_len) {
      case 118:  // 0x76 version 요청 op code
        get_addr_flg = addr_version(temp, leng);
        Serial.println();

        break;
      default: get_addr_flg = 0; break;
    }
  }
}

void Basic_Sonser_Get_last(linkedList* L, Sensor_type* newSensor) {
  char temp[100];
  String val;
  int data_len = 0;

  // 시현0519
  if (!get_addr_flg) {
    get_version(addr_test);  // 시작하면 무조건 addr get 요청  77777777
    Serial.println("requst node addr!!");
  } else {
    /*
      if(http_get_EC && !http_get_TH) get_serson_value_RIKA_RK520_02(addr); //리카센서 EC/지온/지습
      else if(http_get_TH && !http_get_JK) get_serson_value(addr); //HTG3535
      else if(http_get_JK && !http_get_TH) get_serson_value_RIKA_RK330_02(addr); //리카센서 온/습도
    */
    if (1) {
      Serial.print("newSensor->type : ");
      Serial.println(newSensor->type);

      if (start_Get_flag)
        digitalWrite(OUTPIN_6, HIGH);  // sensor node to wifi module status check LED
      basic_sensor_get_last(addr, newSensor);
    }

    else
      Serial.println("not sensor select!!");
  }

  while (!Serial2.available())
    ;
  ;

  if (Serial2.available()) {
    if (start_Get_flag)
      digitalWrite(OUTPIN_6, LOW);  // sensor node to wifi module status check LED

    byte leng = Serial2.readBytes(temp, 100);
    Serial.print("Input data Lenght : ");
    Serial.println(leng);

    Serial.print("Input data : ");
    for (int i = 0; i < leng; i++) {
      val = String(temp[i], DEC);
      data_len = val.toInt();
      Serial.print(data_len, HEX);
    }
    Serial.println();

    //// datalen check!////
    val = String(temp[2], DEC);
    data_len = val.toInt();
    Serial.print("Input senson_node_data Lenght : ");
    Serial.println(data_len);

    //// address check!////

    for (int i = 0; i < 8; i++) {
      val = String(temp[3 + i]);
      addr[i] = val.charAt(0);
    }
    Serial.print("addr : ");
    Serial.println(addr);

    //// OP_Code check!////
    val = String(temp[11], DEC);
    data_len = val.toInt();
    Serial.print("OP_Code : ");
    Serial.println(data_len, HEX);

    switch (data_len) {
      case 112:
        Basic_sensor_get_responsive_last(
            temp, leng, L,
            newSensor);  // 0x70온습도센서 요청 op code 다른 센서값 요청이여도 OP 값을 같은 주의해야함
        // if(http_get_EC) Basic_EC(temp, leng);
        // else Basic_Temp_RH(temp, leng);

        break;
      case 118:  // 0x76 version 요청 op code
        get_addr_flg = addr_version(temp, leng);
        Serial.println();

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
        char* response_code = "0000";
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

        if (!ret)  // addr 확인 후 코드 확인
        {
          start_Get_flag = 1;  /// get_send_return ok!
        } else {
          start_Get_flag = 0;  /// get_send_return ok!
        }
      } else {
        digitalWrite(OUTPIN_1, HIGH);  // 랜선 및 서버접속 불량
        start_Get_flag = 0;            /// get_send_return ok!
      }

    } else {
      start_Get_flag = 0;  /// get_send_return ok!
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());

      Serial.println("[HTTP] GET..failed");
      digitalWrite(OUTPIN_1, HIGH);  // 랜선 및 서버접속 불량
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

      net_aws.setCACert(AWS_CERT_CA);  // Root CA 1 cer 인증서 인증
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

        strcpy(str_buf, payload.c_str());  // web log를 위한거

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
        char* response_code = "0000";
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

        if (!ret)  // addr 확인 후 코드 확인
        {
          start_Get_flag = 1;  /// get_send_return ok!
        } else {
          start_Get_flag = 0;  /// get_send_return ok!
        }
      } else {
        digitalWrite(OUTPIN_1, HIGH);  // 랜선 및 서버접속 불량
        start_Get_flag = 0;            /// get_send_return ok!
      }

    } else {
      start_Get_flag = 0;  /// get_send_return ok!
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      Serial.println("[HTTP] GET..failed");
      digitalWrite(OUTPIN_1, HIGH);  // 랜선 및 서버접속 불량
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

void Httpdatesend_GET_UPDATE_REQUEST() {
  WiFiClient client;
  HTTPClient http;

  // sprintf(basic_url,"http://%s/api/v1/basic?address=%s" ,FARMLABS_URL, addr);
  sprintf(basic_url, "https://%s/?address=%s&version=%s", FARMLABS_URL_AWS_API_GATEWAY_SENSOR_UPDATE, esp_chip_addr,
          VERSION_PROGRAM);  // 07.01
  // sprintf(basic_url,"%s/?address=%s" ,FARMLABS_URL_AWS_API_GATEWAY, esp_chip_addr);   // 07.01

  net_aws.setCACert(AWS_CERT_CA);  // Root CA 1 cer 인증서 인증
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
      strcpy(str_buf, payload.c_str());  // web log를 위한거

      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, payload);
      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }

      const char* return_code = doc["code"];
      char* response_code = "0000";
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

// void Httpdatesend_POST(linkedList *L)
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

    Serial.print("RSSI : ");
    Serial.println(WiFi.RSSI());
    Serial.print("IP : ");
    Serial.println(WiFi.localIP());

    Serial.print("[HTTP] begin...\n");
    if (serverinfo.NEW_SERVER_NUM == 1) {
      sprintf(str, "http://%s:%d/api/v1/wifi-sensor", serverinfo.NEW_IP, serverinfo.NEW_PORT);
      http.begin(client, str);  // HTTP
      http.addHeader("Content-Type", "application/json");
      Serial.print("[HTTP] POST...send Sensor date\n");
    } else {
      // http.begin(client, "http://" FARMLABS_URL "/api/v1/basic"); //HTTP
      sprintf(basic_url, "https://%s", FARMLABS_URL_AWS_API_GATEWAY_SENSOR_DATA);  // 07.01

      net_aws.setCACert(AWS_CERT_CA);  // Root CA 1 cer 인증서 인증
      http.begin(net_aws, basic_url);  // HTTP
      http.addHeader("x-api-key", (String)X_API_KEY_API_GATEWAY_SENSOR_DATA);
      http.addHeader("Content-Type", "application/json");
    }

    ////////////////////////////////////////////////////////////////////////////////
    // herder
    sprintf(str, "{\"address\":\"%s\",\"values\":{", esp_chip_addr);
    strcat(send_str, str);

    // Json value Sensor_parsing

    /*
                      char *s2 = printNodes(L);
                      strcat(send_str,s2);
                      free(s2);
    */

    strcat(send_str, send_data_node);  // sensor value

    // add date

    sprintf(dst_send_str_time, "\"time\":\"%s\",", UTC.dateTime(ATOM).c_str());
    strcat(send_str, dst_send_str_time);

    // tail
    strncpy(dst_send_str, send_str, (strlen(send_str)) - 1);  // 마지막 ','제거

    strcat(dst_send_str, tail);

    Serial.println(dst_send_str);

    // send json message!!

    int httpCode = http.POST(dst_send_str);
    //////////////////////////////////////////////////////////////////////////////

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled

      sprintf(post_code_buf, "[HTTP] post... code: %d\n", httpCode);
      Serial.println(post_code_buf);

      //  Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      //  Serial.println();

      const String& payload = http.getString();
      Serial.println("POST received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");

      // file found at server
      if (httpCode == HTTP_CODE_OK) {
        digitalWrite(OUTPIN_1, LOW);  // Server status checked!!

        StaticJsonDocument<500> doc;
        DeserializationError error = deserializeJson(doc, payload);
        // Test if parsing succeeds.
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          return;
        }
        const char* trens_code = doc["code"];
        const char* trens_meg = doc["result"];
        char* response_code = "0000";
        int ret = strcmp(trens_code, response_code);

        if (!ret)  // addr 확인 후 코드 확인
        {
          start_Get_flag = 1;  /// get_send_return ok!
        } else {
          start_Get_flag = 0;  /// retry!
        }
      } else {
        start_Get_flag = 0;  /// get_send_return ok!
        /*등록된 센서가 아닐경우에도 동작함.*/
        digitalWrite(OUTPIN_1, HIGH);  // 서버에 접근은 가능하나 리턴 코드가 200이 아닐경우 off
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
      start_Get_flag = 0;  /// retry!
      Serial.println("[HTTP] POST..failed");
      // 네트웨크 인터넷 에러일시..
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

  }

  else if (WiFi.status() ==
           WL_IDLE_STATUS) {  // it is a temporary status assigned when WiFi.begin() is called and remains active
                              // until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a connection
                              // is established (resulting in WL_CONNECTED);
    Serial.println("WL_IDLE_STATUS");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    digitalWrite(OUTPIN_7, LOW);
    start_Get_flag = 0;
    // wifi_autoconnect();
    for (;;)
      ;                                            // induce wdt !!
  } else if (WiFi.status() == WL_CONNECTION_LOST)  // assigned when the connection is lost
  {
    Serial.println("WL_CONNECTION_LOST");
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    digitalWrite(OUTPIN_3, HIGH);
    digitalWrite(OUTPIN_7, LOW);
    start_Get_flag = 0;
    // wifi_autoconnect();
    for (;;)
      ;  // induce wdt !!
  } else {
    Serial.print("WiFi.status()_GET_CODE : ");
    Serial.println(WiFi.status());
    start_Get_flag = 0;
    digitalWrite(OUTPIN_3, HIGH);
    digitalWrite(OUTPIN_7, LOW);
    // wifi_autoconnect();
    for (;;)
      ;  // induce wdt !!
  }
}

void AWS_IOT_PUBLISH(linkedList* L) {
  char send_str[255] = {
    0,
  };
  char dst_send_str[255] = {
    0,
  };
  char tail[] = { "}}}" };
  // char desired[]={"{\"state\":{\"reported\": { "};
  char desired[] = { "{\"state\":{\"desired\": " };
  char reported[] = { "{\"state\":{\"reported\": " };

  /////////////////////////////////////Json_string_start////////////////////////////////////////////////////////
  // herder
  // sprintf(str,"{\"address\":\"%s\",\"values\":{", esp_chip_addr);
  strcat(send_str, reported);

  sprintf(str, "{\"address\":\"%s\",", esp_chip_addr);
  strcat(send_str, str);

  // Json value Sensor_parsing

  // char *print_str = printNodes(L);
  //  strcat(send_str, print_str);

  strcat(send_str, send_data_node);

  // tail
  strncpy(dst_send_str, send_str, (strlen(send_str)) - 1);  // 마지막 ','제거
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

/*
void vTaskCode_web( void * pvParameters )
{
  const TickType_t xDelay_tt = 100 / portTICK_PERIOD_MS;
  IPAddress ip;

//////////////////////////////input_page/////////////////////////////////////////////////

  AutoConnectAux&  aux = *Portal.aux("/input_page");
  AutoConnectAux&  aux_result = *Portal.aux("/check");
  AutoConnectInput& input_passwd = aux.getElement<AutoConnectInput>("passwd");
  AutoConnectInput& input_ip = aux["ipaddress"].as<AutoConnectInput>();
  AutoConnectInput& input_port = aux.getElement<AutoConnectInput>("portNum");
  AutoConnectRadio& radio_select = aux.getElement<AutoConnectRadio>("period");
  AutoConnectRadio& radio_select_server = aux.getElement<AutoConnectRadio>("server_selsect");  // 1: local, 2:분석모델,
3:aws AutoConnectText&  result = aux_result["result"].as<AutoConnectText>();

  ip.fromString(input_ip.value);
  input_ip.value = ip.toString();


//////////////////////////////input_page/////////////////////////////////////////////////
 for(;;)
 {

     vTaskDelay(xDelay_tt);
     Portal.handleClient();


    if(input_passwd.value.toInt() == PASSWD_VALUE)
    {
      result.value = "IP Address " + input_ip.value + " is OK.";
      result.style = "";

      Autoconnet_Web_setting *Auto_setting = (Autoconnet_Web_setting *)malloc(sizeof(Autoconnet_Web_setting));
      memset(Auto_setting,0x00,sizeof(Autoconnet_Web_setting));

      Auto_setting->ip=input_ip.value;
      Auto_setting->port=input_port.value.toInt();
     // Auto_setting->member=input_passwd.value.toInt();

      switch (radio_select.checked)
      {
      case 1 :
       Auto_setting->delay_time=radio_select.checked*10;
        break;
       case 2 :
       Auto_setting->delay_time=radio_select.checked*15;
        break;
       case 3 :
       Auto_setting->delay_time=radio_select.checked*20;
        break;
       case 4 :
       Auto_setting->delay_time=radio_select.checked*45;
        break;

      default:
        break;
      }

      Auto_setting->server_num=radio_select_server.checked;

      Serial.println("send_task_2_eeprom save");

      strcpy(serverinfo.NEW_IP ,Auto_setting->ip.c_str()); // string to char array
      serverinfo.NEW_PORT = Auto_setting->port;
      serverinfo.NEW_SERVER_NUM = Auto_setting->server_num;
      serverinfo.NEW_SERVER_SEND_DELAY_SEC = Auto_setting->delay_time;
      EEPROM.writeString(ESP_ERR_STR_BASE_LOCALIP, serverinfo.NEW_IP);
      EEPROM.writeUInt(ESP_ERR_INT_BASE_LOCALPORT, serverinfo.NEW_PORT);
      EEPROM.writeUInt(ESP_ERR_CHAR_BASE_SERVERNUM, serverinfo.NEW_SERVER_NUM);
      EEPROM.writeUInt(ESP_ERR_INT_BASE_SEND_DELAY, serverinfo.NEW_SERVER_SEND_DELAY_SEC);
      EEPROM.commit();


    if( xQueueSend( msg_Queue, (void * )&Auto_setting, 10) != pdTRUE )
      {
      Serial.println("Queue full_msg");
      free(Auto_setting);

      // Failed to post the message, even after 10 ticks.
      }

     input_passwd.value="000000"; // reset passwd!

    }



 } //for;;


}

*/

void vTaskCode_web(void* pvParameters) {
  const TickType_t xDelay_tt = 100 / portTICK_PERIOD_MS;
  // Autoconnet_Web_setting *Auto_setting = (Autoconnet_Web_setting *)malloc(sizeof(Autoconnet_Web_setting));
  // memset(Auto_setting,0x00,sizeof(Autoconnet_Web_setting));

  //////////////////////////////input_page/////////////////////////////////////////////////
  for (;;) {
    vTaskDelay(xDelay_tt);
    Portal.handleClient();
    Mqtt_loop();

    /*
      if( xQueueSend( msg_Queue, (void * )&Auto_setting, 10) != pdTRUE )
       {
       Serial.println("Queue full_msg");

       // Failed to post the message, even after 10 ticks.
       }
   */

  }  // for;;
}

void vTast_CPU_0_Function(void) {
  // Create mutex before starting tasks

  // mutex = xSemaphoreCreateMutex();

  // Create the task, storing the handle.  Note that the passed parameter ucParameterToPass
  // must exist for the lifetime of the task, so in this case is declared static.  If it was just an
  // an automatic stack variable it might no longer exist, or at least have been corrupted, by the time
  // the new task attempts to access it.
  /*
  xTaskCreatePinnedToCore(
              vTaskCode_main,
              "vTaskCode_main",
              2048,
              NULL,
              1,
              NULL,
              0);
 */

  xTaskCreatePinnedToCore(vTaskCode_web, "vTaskCode_web", 20000, NULL, 1, NULL,
                          1);  // cpu

  // Use the handle to delete the task.
  /*

     if( xReturned == pdPASS )
     {
       // 태스크가 만들어졌습니다. 태스크의 핸들을 이용하면 태스크를 삭제할 수 있습니다.

      vTaskDelete( xHandle );
     }

 */

  msg_Queue = xQueueCreate(mag_queue_len, sizeof(Autoconnet_Web_setting));

  // RGBW_Queue = xQueueCreate(mag_queue_len, sizeof(RGBW_type) );
  // VEML_7700_Queue = xQueueCreate(mag_queue_len, sizeof(Sensor_type_VEML7700) );
}

void setup() {
  pinMode(OUTPIN_1, OUTPUT);  // wifi_pcb_led_서버상태 // 등록된 센서가 아닐경우에 꺼짐
  pinMode(OUTPIN_2, OUTPUT);  // wifi_pcb_led_보드 동작상태
  pinMode(OUTPIN_3, OUTPUT);  // wifi_pcb_led_wifi 상태 //  wifi 상태 및 망 상태 확인, 정상일때 항상 점등, WIFI AP연결
                              // 에러시 꺼짐, 망접속 에러 시 2번 점멸
  pinMode(OUTPIN_4, OUTPUT);  // wifi_pcb_리셋
  pinMode(OUTPIN_5, OUTPUT);  // wifi_pcb_led_분석모델 or 팜모닝 향 상태
  pinMode(OUTPIN_6,
          OUTPUT);  // sensor node led_1 control // Sensor_node와 통신상태,점등이 안될 시 센서설정이 안된 상태
  pinMode(OUTPIN_7, OUTPUT);  // sensor node led_2 control // wifi 상태 및 망 상태 확인, 정상일때 항상 점등, WIFI
                              // AP연결 에러시 꺼짐, 망접속 에러 시 2번 점멸

  USE_SERIAL.begin(9600);
  USE_SERIAL.setTimeout(500);
  USE_SERIAL2.begin(19200, SERIAL_8N1, RXD2, TXD2);

  chipid = ESP.getEfuseMac();  // The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32));  // print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)chipid);                        // print Low 4bytes.

  sprintf(str, "%08X", htonl((uint32_t)chipid));
  strncpy(esp_chip_addr, str, 8);
  sprintf(str, "%04X", htons((uint16_t)(chipid >> 32)));
  strcat(esp_chip_addr, str);

  Serial.println(esp_chip_addr);

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

  USE_SERIAL.println();                    // 버젼표시
  USE_SERIAL.print("VERSION_PROGRAM : ");  // 버젼표시
  USE_SERIAL.println(VERSION_PROGRAM);     // 버젼표시
  vTaskDelay(2000);                        // basic init time

#if LCD_DEBUG
  I2Cinitialize();
#endif
  // wifi_connect();
  // wifi_smartconfig();

  Basic_Sonser_Get();
  while (!get_addr_flg)
    Basic_Sonser_Get();

  // 1ms timer
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);

  /////////////// wifi_autoconnect Task Watchdog Timer init///////////////
  // esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_init(300, true);  // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);        // add current thread to WDT watch

  wifi_autoconnect();

  // AWS_OTA_UPDATE();
  otadive_update();  //

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

  Portal.load(FPSTR(input_page));
  Portal.load(FPSTR(input_sensor_page));
  Portal.on("/check", checkIPAddress);
  Portal.on("/Select_sensor_check", checkSensor);

  Portal.load(FPSTR(Sensor_cal));
  Server.on("/PH_PV_read", PH_PV_read);
  Server.on("/918_result", PH_918_compleat);
  Server.on("/686_result", PH_686_compleat);
  Server.on("/400_result", PH_400_compleat);

  Server.on("/EC_PV_read", EC_PV_read);
  Server.on("/1413_result", EC_1413_compleat);

  Portal.load(auxPage);
  Server.on("/clock", auxClock);
  Server.on("/status_log", auxStatus);
  Server.on("/status_log_heap", auxStatus_heap);
  Server.on("/status_log_real_time_data", auxStatus_real_time_data);
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

  ///////////////////////////////eeprom/////////////////////////////////
  eeprom_init();
  eeprom_read_data();
  if (serverinfo.NEW_SERVER_NUM == 1)
    digitalWrite(OUTPIN_5, LOW);  //
  else
    digitalWrite(OUTPIN_5, HIGH);

  ///////////////////////////////eeprom/////////////////////////////////

  heap_trace_init_standalone(trace_record, NUM_RECORDS);
}

void loop() {
  Sensor_type* newSensor = (Sensor_type*)malloc(sizeof(Sensor_type));
  Autoconnet_Web_setting* _Auto_setting = (Autoconnet_Web_setting*)malloc(sizeof(Autoconnet_Web_setting));
  linkedList* L = (linkedList*)malloc(sizeof(linkedList));

  memset(newSensor, 0x00, sizeof(Sensor_type));
  memset(_Auto_setting, 0x00, sizeof(Autoconnet_Web_setting));
  memset(L, 0x00, sizeof(linkedList));

  while (1) {
    // if(!start_Get_flag) Httpdatesend_GET_NEW(newSensor); //서버로부터 등록센서정보 받아오기
    if (newSensor->type[0] != NULL)
      start_Get_flag = 1;
    else
      start_Get_flag = 0;

    strcpy(newSensor->type, select_sensor_info.SELECT_TYPE);  //센서 key cp
    esp_task_wdt_reset();
    digitalWrite(OUTPIN_2, HIGH);  // status live LED

    Portal.on("/echo", [](AutoConnectAux& aux, PageArgument& args) {
      AutoConnectText& ac_echo = aux.getElement<AutoConnectText>("echo");
      AutoConnectText& ac_echo_1 = aux.getElement<AutoConnectText>("echo_1");

      ac_echo.value = basic_url;  // web log 보내는값
      ac_echo_1.value = str_buf;  // web log 보내는값

      return String();
    });
    ///////////////2021.6.03 Queue add!!///////////////////////////////////

    if (xQueueReceive(msg_Queue, &(_Auto_setting), 0) == pdTRUE) {
      Serial.println("RECEIVE TASK_1!!!");
      Serial.println(_Auto_setting->ip);
      Serial.println(_Auto_setting->port);
      Serial.println(_Auto_setting->member);
      Serial.println(_Auto_setting->delay_time);
      Serial.println(_Auto_setting->server_num);
    }

    ///////////////2021.6.03///////////////////////////////////

    if (interruptCounter > 0) {
      portENTER_CRITICAL(&timerMux);
      interruptCounter--;
      portEXIT_CRITICAL(&timerMux);

      memset(print_str, 0x00, sizeof(print_str));
      memset(send_data_node, 0x00, sizeof(send_data_node));

      if (serverinfo.NEW_SET_MODE == 1) {
        Basic_Sonser_Get_last(L, newSensor);  // 센서값 기존에 물어 보기 약 1ms 당

        digitalWrite(OUTPIN_2, LOW);  // status live LED

        strcpy(send_data_node_buf, send_data_node);  // web log
        Serial.print("send server count : ");
        Serial.println(data_send_interruptCounter);
      } else {
        digitalWrite(OUTPIN_2, LOW);
      }  // status live LED

      heapUsed = heap_caps_get_free_size(MALLOC_CAP_8BIT);
      sprintf(str_check, "Heap used at end %d, therefore total heap used %d.\n", heapUsed, heapUsedAtStart - heapUsed);
      heapUsedAtStart = heapUsed;
    }

    if (data_send_interruptCounter_flag)  // 10==10000ms
    {
      data_send_interruptCounter_flag = 0;  //초기화
      if (send_crc_check) {
        switch (serverinfo.NEW_SERVER_NUM) {
          case 1:
            if (start_Get_flag)
              Httpdatesend_POST(L, newSensor);
            break;
          case 2:

            if (start_Get_flag)
              Httpdatesend_POST(L, newSensor);
            //  Httpdatesend_GET_UPDATE_REQUEST();
            break;

          case 3:
            if (start_Get_flag)
              AWS_IOT_PUBLISH(L);
            break;

          default: break;
        }

        // if(start_Get_flag) Httpdatesend_POST(L, newSensor);
        // if(start_Get_flag) AWS_IOT_PUBLISH(L);

        start_Get_flag = 0;
        break;

      }  // send_crc_check
      else
        vTaskDelay(1000);  // crc_err retry delay time
    }                      // interrupt Counter

  }  // while

  free(L);
  free(_Auto_setting);
  free(newSensor);

  /*
     heap_trace_stop();
     heap_trace_dump();
 */
}

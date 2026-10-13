
#ifndef GLOBAL_ESP_H
#define GLOBAL_ESP_H

#define END_CODE 'Z'  //'Z'(0x5A) 센서 값 요청 Response의 Data부분이 끝임을 알리기 위한 Data
#define PASSWD_VALUE "1234567a"
#define HOSTNAME "GREENLABS__NODE"
#define NUM_RECORDS 100

#define SERVER_LOCAL 0       // 1 : local server operation & 0 : basic server operation
#define WDT_TIMEOUT 180      ////sec(unit) loop seconds WDT
#define SERIAL_TIME_OUT 500  //

#define SERVER_IP "223.171.52.93:8801"  // 회사 내부 IP
//#define SERVER_IP "192.168.50.100:8801" // local내부 IP
//#define SERVER_IP "192.168.50.101:8801" // 홍성 농장 내부 IP
//#define SERVER_IP "223.171.43.131:8801"  // 내부 테스트 농장 2
//#define SERVER_IP "223.171.52.138:8803"  // 승석 농장 3

//#define FARMLABS_URL "basic.farmlabs.co.kr"
#define FARMLABS_URL "basic-staging.farmmorning.com"
#define FARMLABS_URL_AWS_API_GATEWAY_SENSOR_TYPE \
    "qgyk0mubbb.execute-api.ap-northeast-2.amazonaws.com/default/smartfarm-sensor-type-matcher-handler"
#define X_API_KEY_API_GATEWAY_SENSOR_TYPE "hjdeFcPi6w3niUqaVHSyl4vsY2aQYBra3XyPjtur"

#define FARMLABS_URL_AWS_API_GATEWAY_SENSOR_DATA \
    "qgyk0mubbb.execute-api.ap-northeast-2.amazonaws.com/default/smartfarm-sensor-data-receiver-handler"
#define X_API_KEY_API_GATEWAY_SENSOR_DATA "hjdeFcPi6w3niUqaVHSyl4vsY2aQYBra3XyPjtur"

#define FARMLABS_URL_AWS_API_GATEWAY_SENSOR_UPDATE \
    "qgyk0mubbb.execute-api.ap-northeast-2.amazonaws.com/default/smartfarm-sensor-update-node-handler"
#define X_API_KEY_API_GATEWAY_SENSOR_UPDATE "hjdeFcPi6w3niUqaVHSyl4vsY2aQYBra3XyPjtur"

#define USE_SERIAL Serial
#define USE_SERIAL2 Serial2

#define RXD2 16
#define TXD2 17

#define OUTPIN_1 18
#define OUTPIN_2 19
#define OUTPIN_3 5
#define OUTPIN_4 12
#define OUTPIN_5 21

#define OUTPIN_6 2
#define OUTPIN_7 15

#define ADDR_6713 0x15  // default I2C CO2 slave address
#define LCD_2040A 0x27  // default I2C LCD slave address

typedef struct _sensor_info sensorinfo;
typedef struct _parsing_type parsing;
typedef struct _send_data senddata;

typedef struct NEW_INFO {
    char NEW_IP[15] = {
        0,
    };
    char NEW_RESET_TIME[6] = {
        0,
    };
    int NEW_SERVER_NUM = 0;
    int NEW_SERVER_SEND_DELAY_SEC = 0;
    int NEW_PORT = 0;
    int NEW_SERVER_NUM_TEST = 0;
    uint8_t NEW_SET_MODE = 1;   // reboot 시 항상 리얼모드
    uint8_t NEW_RESET_SET = 1;  //  재시작
} NEW_SERVER_INFO;

typedef struct _NEW_INFO {
    char SELECT_TYPE[50] = {
        0,
    };
    int SELECT_MEMBER = 0;
    int FLOW_MULTIPLIER = 10;
} NEW_SENSOR_INFO;

struct _sensor_info {
    char type[30];
    int member;
};

struct _parsing_type {
    char send_type;
    int send_int;
    float send_float;
};

struct _send_data {
    char send_type;
    char str[10];
};

typedef struct {
    sensorinfo SELECTSENSOR;
    parsing PARSING[30];
    senddata SENDDATA[30];
    int Num;

} Sensor_Node;

///////////////////////////////////////////////////////
typedef struct setting {
    String ip;
    int port;
    int member;
    uint8_t delay_time;
    uint8_t server_num;
} Autoconnet_Web_setting;

typedef struct _node {
    char type[50];
    const char *type_str[10];
    int member;
} Sensor_type;

typedef struct __list {
    struct __node *cur;
    struct __node *head;
    struct __node *tail;
} linkedList;

typedef struct __node {
    const char *str;
    char ch;
    int data;
    uint8_t str_format_t;  // 1== char, 0==str
    struct __node *next;
} node;

///////////////////////////////////////////////////////

const char Sersor_CODE_array[] = {
    'H', 'T', 'C', 'c', 't', 'h', 'W', 'P', 'D', 'S', 'R', 'E', 'm', 'u', 'B', 'w', 'f',
    'U', 'r', 'd', 'N', 'F', 'b', 'Y', 'L', 'M', 'y', 'O', 'i', 'o', 'p', 'g', 'A', 's',
    'e', 'Q', 'l', 'a', 'k', 'G', 'J', 'K', 'V', 'X', 'j', 'v', 'n', 'q', 'I', 'x', '1'
};

const char array_1bytevalue[] = { 'R', 'w', 'N', 'F', 'b', 'M', 's' };
// const char array_1bytevalue[]={"RwNFbMs"};

const char array_2bytevalue[] = { 'H', 'T', 'C', 'c', 't', 'h', 'W', 'P', 'D', 'S', 'E', 'm', 'u', 'B', 'f',
                                  'U', 'r', 'd', 'Y', 'L', 'y', 'O', 'i', 'o', 'p', 'g', 'A', 'e', 'Q', 'l',
                                  'a', 'k', 'G', 'J', 'K', 'V', 'X', 'j', 'v', 'n', 'q', 'I', 'x', '1' };

/*
const char* mqtt_server = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; // Relace with your MQTT END point
const int mqtt_port = 8883;
*/

// int Sensor_parsing(char *temp, char member, int count_val);
int Sensor_parsing(char *temp, char member, int count_val, linkedList *L);

int Sensor_parsing_2BYTE(char *temp, int count_val, linkedList *L);
int Sensor_parsing_1BYTE(char *temp, int count_val, linkedList *L, Sensor_type *newSensor);
void createNode(linkedList *L, const char *str, int tdata, uint8_t format);
void deleteLastNode(linkedList *L);
char *printNodes(linkedList *L);
void _printNodes(linkedList *L);

void Mqtt_loop(void);
void ConnectAWS(void);
void connected_check(void);
void AWS_IOT_publishMessage_SensorNode(char *str);
void AWS_IOT_shadow_update_SensorNode(char *str);
void publishMessage(void);

void eeprom_init(void);
void eeprom_read_data(void);
void AWS_OTA_UPDATE(void);

/*

void connectAWS_AWS_IOT();
void updateShadow (int power);
void updateShadow_AWS();
*/

#endif  // global.h

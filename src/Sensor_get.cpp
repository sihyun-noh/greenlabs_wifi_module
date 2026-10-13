#include "log.h"
#include "syslog.h"
#include <Arduino.h>
#include <global.h>
#include <stdio.h>

static const char *TAG = "[SG]";
Sensor_Node _Sensor;
extern NEW_SENSOR_INFO select_sensor_info;

char ch;
String val;
unsigned int val_dec;
int data_val1, data_val2 = 0;

int Sensor_parsing_2BYTE(char *temp, int count_val, linkedList *L) {
  int Num = 1;

  val = String(temp[count_val]);
  ch = val.charAt(0);
  count_val++;

  val = String(temp[count_val], DEC);
  data_val1 = val.toInt();
  count_val++;

  val = String(temp[count_val], DEC);
  data_val2 = val.toInt();
  count_val++;
  val_dec = (255 * data_val1) + data_val1 + data_val2;

  if (ch == 'f') {
    // Flow scale: 0 sends 0, 10 keeps the raw value, 50 applies x5,
    // and 100 applies x10. Example with scale 10: 1->1, 10->10, 100->100.
    if (select_sensor_info.FLOW_MULTIPLIER == 0)
      val_dec = 0;
    else
      val_dec = (val_dec * select_sensor_info.FLOW_MULTIPLIER) / 10;
  }

  if (ch == 'J' && val_dec > 32768)
    val_dec = val_dec - 65535 - 1;
  if (ch == 'O' && val_dec > 32768)
    val_dec = val_dec - 65535 - 1;
  if (ch == 'l' && val_dec > 32768)
    val_dec = val_dec - 65535 - 1;

  if (ch == 'r' && val_dec > 32768)
    val_dec = val_dec - 65535 - 1;
  if (ch == 'd' && val_dec > 32768)
    val_dec = val_dec - 65535 - 1;

  if (ch == 'Y' && val_dec > 32768)
    val_dec = val_dec - 65535 - 1;
  if (ch == 'L' && val_dec > 32768)
    val_dec = val_dec - 65535 - 1;

  createNode(L, &ch, val_dec, 1);

  _Sensor.PARSING[Num].send_type = ch;
  _Sensor.PARSING[Num].send_int = val_dec;

  SLOGI(TAG, "Code : %c, Value : %d", _Sensor.PARSING[Num].send_type,
        _Sensor.PARSING[Num].send_int);
  // SLOGI(TAG, "Sensor_value : %d", _Sensor.PARSING[Num].send_int);

  return count_val;
}

int Sensor_parsing_1BYTE(char *temp, int count_val, linkedList *L,
                         Sensor_type *newSensor) {
  int Num = 1;

  val = String(temp[count_val]);
  ch = val.charAt(0);
  count_val++;

  val = String(temp[count_val], DEC);
  data_val1 = val.toInt();
  count_val++;

  val_dec = data_val1;

  if (ch == 'b' || ch == 'F' || ch == 'N' || ch == 'w') {
    switch (ch) {
    case 'b':
      if (val_dec / 8) {
        val_dec -= 8;
        createNode(L, "b4", 1, 0);
      } // H (3BIT)
      else {
        createNode(L, "b4", 0, 0);
      }

      if (val_dec / 4) {
        val_dec -= 4;
        createNode(L, "b3", 1, 0);
      } // T (2BIT)
      else {
        createNode(L, "b3", 0, 0);
      }

      if (val_dec / 2) {
        val_dec -= 2;
        createNode(L, "b2", 1, 0);
      } //  SOL (1BIT)
      else {
        createNode(L, "b2", 0, 0);
      }

      if (val_dec / 1) {
        val_dec -= 1;
        createNode(L, "b1", 1, 0);
      } // TEXT (0BIT)
      else {
        createNode(L, "b1", 0, 0);
      }

      break;
    case 'F':
      if (val_dec / 8) {
        val_dec -= 8;
        createNode(L, "F4", 1, 0);
      } // H (3BIT)
      else {
        createNode(L, "F4", 0, 0);
      }

      if (val_dec / 4) {
        val_dec -= 4;
        createNode(L, "F3", 1, 0);
      } // T (2BIT)
      else {
        createNode(L, "F3", 0, 0);
      }

      if (val_dec / 2) {
        val_dec -= 2;
        createNode(L, "F2", 1, 0);
      } // SOL (1BIT)
      else {
        createNode(L, "F2", 0, 0);
      }

      if (val_dec / 1) {
        val_dec -= 1;
        createNode(L, "F1", 1, 0);
      } // TEXT (0BIT)
      else {
        createNode(L, "F1", 0, 0);
      }

      break;
    case 'N':
      if (val_dec / 8) {
        val_dec -= 8;
        createNode(L, "N4", 1, 0);
      } // H (3BIT)
      else {
        createNode(L, "N4", 0, 0);
      }

      if (val_dec / 4) {
        val_dec -= 4;
        createNode(L, "N3", 1, 0);
      } // T (2BIT)
      else {
        createNode(L, "N3", 0, 0);
      }

      if (val_dec / 2) {
        val_dec -= 2;
        createNode(L, "N2", 1, 0);
      } // SOL (1BIT)
      else {
        createNode(L, "N2", 0, 0);
      }

      if (val_dec / 1) {
        val_dec -= 1;
        createNode(L, "N1", 1, 0);
      } // TEXT (0BIT)
      else {
        createNode(L, "N1", 0, 0);
      }
      break;

    case 'w':
      if (val_dec / 4) {
        val_dec -= 4;
        createNode(L, "w3", 1, 0);
      } //  H (3BIT)
      else {
        createNode(L, "w3", 0, 0);
      }

      if (val_dec / 2) {
        val_dec -= 2;
        createNode(L, "w2", 1, 0);
      } // T (2BIT)
      else {
        createNode(L, "w2", 0, 0);
      }

      if (val_dec / 1) {
        val_dec -= 1;
        createNode(L, "w1", 1, 0);
      } // SOL (1BIT)
      else {
        createNode(L, "w1", 0, 0);
      }
      break;

    default:
      break;
    }
  } else
    createNode(L, &ch, val_dec, 1);

  _Sensor.PARSING[Num].send_type = ch;
  _Sensor.PARSING[Num].send_int = val_dec;

  SLOGI(TAG, "Code : %c, Value : %d", _Sensor.PARSING[Num].send_type,
        _Sensor.PARSING[Num].send_int);
  // SLOGI(TAG, "Sensor_value : %d", _Sensor.PARSING[Num].send_int);

  return count_val;
}

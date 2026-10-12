
#ifndef ESP_EEPROM_H
#define ESP_EEPROM_H

#define EEPROM_SIZE 0x0400  // byte

#define ESP_ERR_CHAR_BASE_SERVERNUM 0x0000                               /*!<  */
#define ESP_ERR_CHAR_BASE_RESET_SET (ESP_ERR_CHAR_BASE_SERVERNUM + 0x04) /*!<  */
//#define ESP_ERR_CHAR_NOT_INITIALIZED         (ESP_ERR_CHAR_BASE + 0x01)  /*!<  */
//#define ESP_ERR_CHAR_NOT_FOUND               (ESP_ERR_CHAR_BASE + 0x02)  /*!<  */
//#define ESP_ERR_CHAR_TYPE_MISMATCH           (ESP_ERR_CHAR_BASE + 0x03)  /*!<  */
//#define ESP_ERR_CHAR_READ_ONLY               (ESP_ERR_CHAR_BASE + 0x04)  /*!<  */

#define ESP_ERR_INT_BASE_LOCALPORT 0x0050                               /*!<  */
#define ESP_ERR_INT_BASE_SEND_DELAY (ESP_ERR_INT_BASE_LOCALPORT + 0x04) /*!<  */
#define ESP_ERR_INT_BASE_TEST (ESP_ERR_INT_BASE_LOCALPORT + 0x08)       /*!<  */
//#define ESP_ERR_INT_NOT_FOUND               (ESP_ERR_INT_BASE + 0x02)  /*!<  */
//#define ESP_ERR_INT_TYPE_MISMATCH           (ESP_ERR_INT_BASE + 0x03)  /*!<  */
//#define ESP_ERR_INT_READ_ONLY               (ESP_ERR_INT_BASE + 0x04)  /*!<  */

#define ESP_ERR_STR_BASE_LOCALIP 0x0150 /*!<  */
#define ESP_ERR_STR_BASE_SET_SENSOR 0x0200
#define ESP_ERR_STR_RESET_TIME 0x0260
/*!<  */
//#define ESP_ERR_STR_NOT_INITIALIZED         (ESP_ERR_STR_BASE + 0x01)  /*!<  */
//#define ESP_ERR_STR_NOT_FOUND               (ESP_ERR_STR_BASE + 0x02)  /*!<  */
//#define ESP_ERR_STR_TYPE_MISMATCH           (ESP_ERR_STR_BASE + 0x03)  /*!<  */
//#define ESP_ERR_STR_READ_ONLY               (ESP_ERR_STR_BASE + 0x04)  /*!<  */

#endif  // ESP_EEPROM_H

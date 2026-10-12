#ifndef _SPIFFS_T_H_
#define _SPIFFS_T_H_

void spiffs_init();

void spiffs_format();

void listDir(void);

void readFile(const char *filename);

void writeFile(const char *filename, const char *str);

void appendFile(const char *filename, const char *str); 

void deleteFile(const char *filename); 

#endif /* _SPIFFS_T_H_ */

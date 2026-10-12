/*
 *	Simplic Linked List Structure
 *	  by Yowu (uyu423@gmail.com || http://luckyyowu.tistory.com)
 *	Date : 2016. 04. 15
 *	Development Env.
 *		Ubuntu 15.10 (x64, Linux Kernel 4.2.0)
 *		GNU Compiler Collection 5.2.1
 *		Vi IMproved 7.4, File Encoding UTF-8
 */

#include <ArduinoJson.h>
#include <global.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char print_str[20] = {
  0,
};
char send_data_node[255] = {
  0,
};

void createNode(linkedList *L, const char *str, int tdata, uint8_t format) {
  node *newNode = (node *)malloc(sizeof(node));

  if (!format)
    newNode->str = str;
  else
    newNode->ch = *str;
  newNode->str_format_t = format;
  newNode->data = tdata;

  if (!newNode->str_format_t)
    sprintf(print_str, "\"%s\":\"%d\",", newNode->str, newNode->data);
  else
    sprintf(print_str, "\"%c\":\"%d\",", newNode->ch, newNode->data);

  strcat(send_data_node, print_str);
  free(newNode);
}

/*
void createNode(linkedList *L, const char *str, int tdata, uint8_t format)
{

    node *newNode = (node *)malloc(sizeof(node));

    if(!format) newNode->str = str;
    else newNode->ch = *str;
    newNode->str_format_t = format;
    newNode->data = tdata;

    newNode->next = NULL;
    if(L->head == NULL && L->tail == NULL)
        L->head = L->tail = newNode;
    else {
        L->tail->next = newNode;
        L->tail = newNode;
    }

    L->cur = newNode;

}
*/

void deleteLastNode(linkedList *L) {
  node *p = L->head;
  while (p->next->next != NULL)
    p = p->next;
  p->next = p->next->next;
  L->tail = p;
}

void _printNodes(linkedList *L) {
  node *p = L->head;
  putchar('[');
  while (p != NULL) {
    printf("%s, ", p->str);
    printf("%d, ", p->data);
    p = p->next;
  }
  putchar(']');
  putchar('\n');
}

/*
char *printNodes(linkedList *L ) {

    node *p = L->head;
    char s1[20] ={0,} ;
 //	char str2[255] ={0,} ;

    while(p != NULL) {
            if(!p->str_format_t) sprintf(s1,"\"%s\":\"%d\",", p->str, p->data);
            else sprintf(s1,"\"%c\":\"%d\",", p->ch, p->data);
     //       strcat(str2,s1);
            p = p->next;
        //	Serial.print(" s1 : ");
        //    Serial.println(s1);
    }
    return s1;

}
*/

char *printNodes(linkedList *L) {
  node *p = L->head;
  char *s1 = (char *)malloc(sizeof(char) * 20);
  char *str2 = (char *)malloc(sizeof(char) * 255);

  memset(s1, 0x00, sizeof(char) * 20);
  memset(str2, 0x00, sizeof(char) * 255);

  while (p != NULL) {
    // printf("\"%c\":\"%d\",", p->str,p->data);
    if (!p->str_format_t)
      sprintf(s1, "\"%s\":\"%d\",", p->str, p->data);
    else
      sprintf(s1, "\"%c\":\"%d\",", p->ch, p->data);
    strcat(str2, s1);
    memset(s1, 0x00, sizeof(char) * 20);
    p = p->next;
  }
  // free(s1);
  return str2;
  free(s1);
  free(str2);
}

int linker_list(void) {
  // linkedList pointer define start
  linkedList *L = (linkedList *)malloc(sizeof(linkedList));
  L->cur = NULL;
  L->head = NULL;
  L->tail = NULL;
  // linkedList pointer define end

  // createNode(L, "c", 1, 1, );

  /*
      createNode(L, 'b');
      deleteLastNode(L);
      createNode(L, 4);
      createNode(L, 5);
      createNode(L, 6);
      deleteLastNode(L);
      deleteLastNode(L);
      createNode(L, 7);
  */
  //	printNodes(L);

  return 0;
}

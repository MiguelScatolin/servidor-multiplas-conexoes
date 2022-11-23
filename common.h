#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>

#define BUFSZ 1024
#define NUMERO_MAX_CONEXOES 10

typedef enum {
  REQ_ADD = 1,
  REQ_RM,
  RES_ADD,
  RES_LIST,
  REQ_INF,
  RES_INF,
  ERROR,
  OK
} tipo;

typedef enum {
  EQUIPMENT_NOT_FOUND = 1,
  SOURCE_EQUIPMENT_NOT_FOUND,
  TARGET_EQUIPMENT_NOT_FOUND,
  EQUIPMENT_LIMIT_EXCEEDED
} codigo_erro;

typedef struct message {
  tipo type;
  int sourceId;
  int destinationId;
  int sourceSocket;
  int payload[NUMERO_MAX_CONEXOES];
  float temperature;
} message;

void sendMessage(int s, char *message);
void receiveMessage(int s, char *buf);
char *addressToString(const struct sockaddr *address);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
struct sockaddr_storage parseAddress(char *ipAdress, char *port);
void logexit(char *errorMessage);
bool stringEqual(char *s1, char *s2);
int convertToInt(char *string);
message parseMessage(char s[BUFSZ], int socket);
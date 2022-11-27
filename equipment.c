#include "common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#define MAX_TEMPERATURE 1000

int serverSocket;
int myId = 0;
int equipment[NUMERO_MAX_CONEXOES] = {0};

int getcmd(char *buf, int nbuf)
{
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  return 0;
};

void successfullAdd(message message) {
  int newConnectedId = message.payload[0];
  if(myId == 0) {
    myId = newConnectedId;
    printf("New ID: %02d \n", myId);
  }
  else {
    for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
      if(equipment[i] == 0) {
        equipment[i] = newConnectedId;
        break;
      }
    }
    printf("Equipment %02d added\n", newConnectedId);
  }
}

void printEquipiments() {
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(equipment[i])
      printf("%02d ", equipment[i]);
  }
  printf("\n");
}

void receiveEquipmentList(message message) {
  int equipmentListIndex = 0;
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(message.payload[i] != 0 && message.payload[i] != myId) {
      equipment[equipmentListIndex] = message.payload[i];
      equipmentListIndex++;
    }
  }
  //printEquipiments();
}

void successfulClose(message message) {
  printf("Success\n");
  close(serverSocket);
  exit(EXIT_SUCCESS);
}

void removeEquipment(message message) {
  int removedEquipmentId = message.sourceId;
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(equipment[i] == removedEquipmentId) {
      equipment[i] = 0;
      break;
    }
  }
  printf("Equipment %02d removed\n", removedEquipmentId);
}

float getRandomInformation() {
  return (rand() % MAX_TEMPERATURE + 1) / (float) 100;
}

void respondInformation(message message) {
  printf("requested information\n");
  char respondInformationMessage[50];
  sprintf(respondInformationMessage, "%02d %02d %02d %.2f", RES_INF, message.destinationId, message.sourceId, getRandomInformation());
  sendMessage(serverSocket, respondInformationMessage);
}

void receiveInformation(message message) {
  printf("Value from %02d: %.2f\n", message.sourceId, message.temperature);
}

void runcmd(message message)
{
  switch (message.type) 
  {
    case REQ_RM:
      removeEquipment(message);
      break;
    case RES_ADD:
      successfullAdd(message);
      break;
    case RES_LIST:
      receiveEquipmentList(message);
      break;
    case OK:
      successfulClose(message);
      break;
    case ERROR:
      printError(message.payload[0]);
      break;
    case REQ_INF:
      respondInformation(message);
      break;
    case RES_INF:
      receiveInformation(message);
      break;
    
    default:
      logexit("tipo de mensagem desconhecido");
  }
}

void runTerminalCommand(char *command) {
  const *val = strtok(command, " ");
  char receiveBuf[BUFSZ];

  if(stringEqual(val, "close")) {
    char closeMessage[50];
    sprintf(closeMessage, "%02d %02d", REQ_RM, myId);
    sendMessage(serverSocket, closeMessage);
    receiveMessage(serverSocket, receiveBuf);
    runcmd(parseMessage(receiveBuf, serverSocket));
  }
  else if(stringEqual(val, "list")) {
    printEquipiments();
  }
  else if(stringEqual(val, "request")) {
    strtok(NULL, " ");
    strtok(NULL, " ");
    val = strtok(NULL, " ");
    if(val == NULL) {
      return;
      printf("missing idEQ i");
    }
    int equipmentId = convertToInt(val);
    char requestInformationMessage[50];
    sprintf(requestInformationMessage, "%02d %02d %02d", REQ_INF, myId, equipmentId);
    sendMessage(serverSocket, requestInformationMessage);
  }
}

void * messageHandler() {
  char receiveBuf[BUFSZ];
  while(1) {
    receiveMessage(serverSocket, receiveBuf);
    runcmd(parseMessage(receiveBuf, serverSocket));
  }
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  if (argc < 3)
    logexit("parametros nao informados");

  char *ipAdress = argv[1];
  char *port = argv[2];

  struct sockaddr_storage storage = parseAddress(ipAdress, port);
  struct sockaddr *address = (struct sockaddr *)(&storage);

  serverSocket = socket(storage.ss_family, SOCK_STREAM, 0);
  if(serverSocket == -1)
    logexit("falha ao inicializar socket");

  if(0 != connect(serverSocket, address, sizeof(storage)))
    logexit("falha ao conectar");

  sendMessage(serverSocket, "01");
  char receiveBuf[BUFSZ];

  receiveMessage(serverSocket, receiveBuf);
  runcmd(parseMessage(receiveBuf, serverSocket));

  memset(receiveBuf, 0, BUFSZ);
  receiveMessage(serverSocket, receiveBuf);
  runcmd(parseMessage(receiveBuf, serverSocket));

  pthread_t tid;
  pthread_create(&tid, NULL, messageHandler, NULL);

  char buf[100];
  while(getcmd(buf, sizeof(buf)) >= 0) {
    runTerminalCommand(buf);
  }

  close(serverSocket);
  exit(EXIT_SUCCESS);
}
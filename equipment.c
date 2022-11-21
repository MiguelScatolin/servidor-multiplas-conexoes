#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int serverSocket;
int myId;
int equipments[NUMERO_MAX_CONEXOES] = {0};

int getcmd(char *buf, int nbuf)
{
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  return 0;
};

void successfullAdd(message message) {
  myId = message.payload[0];
  printf("New ID: %02d \n", myId);
}

void printEquipiments() {
  printf("Equipments:");
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(equipments[i])
      printf(" %02d", equipments[i]);
  }
  printf("\n");
}

void receiveEquipmentList(message message) {
  int equipmentListIndex = 0;
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(message.payload[i] != 0) {
      equipments[equipmentListIndex] = message.payload[i];
      equipmentListIndex++;
    }
  }
  printEquipiments();
}

void successfulClose(message message) {
  printf("Success\n");
  close(serverSocket);
  exit(EXIT_SUCCESS);
}

void runcmd(message message)
{
  switch (message.type) 
  {
    case RES_ADD:
      successfullAdd(message);
      break;
    case RES_LIST:
      receiveEquipmentList(message);
      break;
    case OK:
      successfulClose(message);
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

  }
  else if(stringEqual(val, "request")) {
    strtok(NULL, ' ');
    strtok(NULL, ' ');
    val = strtok(NULL, ' ');
    int equipmentId = convertToInt(val);

  }
}

int main(int argc, char *argv[]) {
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

  char buf[100];
  while(getcmd(buf, sizeof(buf)) >= 0) {
    runTerminalCommand(buf);
  }

  close(serverSocket);
  exit(EXIT_SUCCESS);
}
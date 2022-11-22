#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int equipmentPorts[NUMERO_MAX_CONEXOES] = {0};
int currentConnections = 0;
int clientSocket, serverSocket;

void sendMessageToAllEquipments(char *message) {
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(equipmentPorts[i])
      sendMessage(equipmentPorts[i], message);
  }
}

int indexToId(int index) {
  return index + 1;
}

int idToIndex(int id) {
  return id - 1;
}

void terminateConnection() {
  close(clientSocket);
  exit(EXIT_SUCCESS);
}

void installEquipment(message message) {
  if(currentConnections >= NUMERO_MAX_CONEXOES) {
    char errorMessage[50];
    sprintf(errorMessage, "%02d %02d", ERROR, EQUIPMENT_LIMIT_EXCEEDED);
    sendMessage(errorMessage, errorMessage);
    return;
  }

  int newEquipmentId;
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(equipmentPorts[i] == 0) {
      equipmentPorts[i] = message.sourceSocket;
      newEquipmentId = i + 1;
      break;
    }
  }
  
  char equipmentAddedMessage[50];
  sprintf(equipmentAddedMessage, "%02d %02d", RES_ADD, newEquipmentId);
  sendMessageToAllEquipments(equipmentAddedMessage);

  printf("Equipment %02d added\n", newEquipmentId);

  for(int i = 0; i < 100000000; i++);

  char equipmentListMessage[50];
  //todo: fazer isso pegar a lista completa
  sprintf(equipmentListMessage, "%02d %02d", RES_LIST, newEquipmentId);
  sendMessage(clientSocket, equipmentListMessage);
}

void removeEquipment(message message) {
  if(equipmentPorts[idToIndex(message.sourceId)] == 0) {
    char errorMessage[50];
    sprintf(errorMessage, "%02d %02d", ERROR, EQUIPMENT_NOT_FOUND);
    sendMessage(errorMessage, errorMessage);
    return;
  }

  //validacao
  currentConnections--;
  equipmentPorts[idToIndex(message.sourceId)] = 0;

  char okMessage[50];
  sprintf(okMessage, "%02d %02d %02d", OK, message.sourceId, 1);
  sendMessage(clientSocket, okMessage);

  printf("Equipment %02d removed\n", message.sourceId);

  //fechar conexao

  char equipmentRemovedMessage[50];
  sprintf(equipmentRemovedMessage, "%02d %02d", REQ_RM, message.sourceId);
  sendMessageToAllEquipments(equipmentRemovedMessage);
}

void runcmd(message message)
{
  switch (message.type) 
  {
    case REQ_ADD:
      installEquipment(message);
      break;
    case REQ_RM:
      removeEquipment(message);
      break;
    
    default:
      logexit("tipo de mensagem desconhecido");
  }
}

int server_sockaddr_init(const char *portstr, struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_family = AF_INET;
    addr4->sin_addr.s_addr = INADDR_ANY;
    addr4->sin_port = port;
    return 0;
}

int connectToClient(char *portString) {
  struct sockaddr_storage storage;
  if (0 != server_sockaddr_init(portString, &storage))
    logexit("falha ao conectar");

  serverSocket = socket(storage.ss_family, SOCK_STREAM, 0);
  if (serverSocket == -1) {
      logexit("socket");
  }

  int enable = 1;
  if (0 != setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
      logexit("setsockopt");
  }

  struct sockaddr *addr = (struct sockaddr *)(&storage);
  if (0 != bind(serverSocket, addr, sizeof(storage))) {
      logexit("bind");
  }

  if (0 != listen(serverSocket, 10)) {
      logexit("listen");
  }

  char adressString[BUFSZ];
  addrtostr(addr, adressString, BUFSZ);

  struct sockaddr_storage clientStorage;
  struct sockaddr *clientAddress = (struct sockaddr *)(&clientAddress);
  socklen_t caddrlen = sizeof(clientStorage);
  
  int clientSocket = accept(serverSocket, clientAddress, &caddrlen);
  if (clientSocket == -1)
    logexit("accept");
  return clientSocket;
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  if (argc < 2) 
    logexit("parametros nao informados");

  char *port = argv[1];

  clientSocket = connectToClient(port);
  if(clientSocket == -1)
    logexit("accept");

  char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
  while(1) {
    receiveMessage(clientSocket, buf);
    runcmd(parseMessage(buf, clientSocket));
  }

  terminateConnection();
}
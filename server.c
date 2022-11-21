#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int equipmentPorts[NUMERO_MAX_CONEXOES] = {0};
int currentConnections = 0;
int clientSocket, serverSocket;

void terminateConnection() {
  close(clientSocket);
  exit(EXIT_SUCCESS);
}

void installEquipment(cmd comando) {
  if(currentConnections >= NUMERO_MAX_CONEXOES) {
    char errorMessage[50];
    sprintf(errorMessage, "%02d %02d", ERROR, EQUIPMENT_LIMIT_EXCEEDED);
    sendMessage(errorMessage, errorMessage);
  }

  int newEquipmentId;
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(equipmentPorts[i] == 0) {
      equipmentPorts[i] = comando.sourceSocket;
      newEquipmentId = i + 1;
      break;
    }
  }
  
  char equipmentAddedMessage[50];
  sprintf(equipmentAddedMessage, "%02d %02d", RES_ADD, newEquipmentId);
  sendMessage(clientSocket, equipmentAddedMessage);

  printf("Equipment %02d added\n", newEquipmentId);

  for(int i = 0; i < 100000000; i++);

  //todo: fazer isso ser broadcast
  char equipmentListMessage[50];
  //todo: fazer isso pegar a lista completa
  sprintf(equipmentListMessage, "%02d %02d", RES_LIST, newEquipmentId);
  sendMessage(clientSocket, equipmentListMessage);
}

void runcmd(cmd comando)
{
  switch (comando.type) 
  {
    case REQ_ADD:
      installEquipment(comando);
      break;
    
    default:
      logexit("tipo de comando desconhecido");
  }
}

struct sockaddr_storage initializeServerSocket(char *ipVersion, char *portString) {
  if(portString == NULL)
    logexit("port nulo");

  uint16_t port = (uint16_t)atoi(portString);
  if(port == 0)
    logexit("port invalida");
  port = htons(port); //host to network short

  struct sockaddr_storage storage;
  if (strcmp(ipVersion, "v4") == 0) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)(&storage);
    addr4->sin_family = AF_INET;
    addr4->sin_addr.s_addr = INADDR_ANY;
    addr4->sin_port = port;
    return storage;
  } else if(strcmp(ipVersion, "v6") == 0) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)(&storage);
    addr6->sin6_family = AF_INET6;
    addr6->sin6_addr = in6addr_any;
    addr6->sin6_port = port;
    return storage;
  } else
    logexit("nao foi possivel conectar"); 
}

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    } else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    } else {
        return -1;
    }
}

int connectToClient(char *ipVersion, char *portString) {
  struct sockaddr_storage storage;
  if (0 != server_sockaddr_init(ipVersion, portString, &storage))
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
  //printf("bound to %s, waiting connections\n", adressString);

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

  clientSocket = connectToClient("v4", port);
  if(clientSocket == -1)
    logexit("accept");

  char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
  while(1) {
    recv(clientSocket, buf, BUFSZ - 1, 0);
    runcmd(parsecmd(buf, clientSocket));
  }

  terminateConnection();
}
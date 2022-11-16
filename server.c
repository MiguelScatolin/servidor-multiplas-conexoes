#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#define MAX_KBS 1000000
#define NUMERO_MAX_CONEXOES_PENDENTES 10
#define NUMERO_DE_RACKS 4
#define SWITCHES_POR_RACK 3
#define SWITCHES_POR_COMANDO 3

int racks[NUMERO_DE_RACKS][SWITCHES_POR_RACK] = {};
int clientSocket, serverSocket;

typedef enum {
  instalar,
  desinstalar,
  ler,
  listar
} operacao;

typedef struct cmd {
  operacao type;
  int switches[SWITCHES_POR_COMANDO + 1];
  int rack;
} cmd;

void terminateConnection() {
  close(clientSocket);
  exit(EXIT_SUCCESS);
}

operacao getType(char *typeString) {
  if(stringEqual(typeString, "add"))
    return instalar;
  else if(stringEqual(typeString, "rm"))
    return desinstalar;
  else if(stringEqual(typeString, "get"))
    return ler;
  else if(stringEqual(typeString, "ls"))
    return listar;
  else if(stringEqual(typeString, "exit") || stringEqual(typeString, "exit\n"))
    terminateConnection();
  else {
    sendMessage(clientSocket, "invalid command");
    terminateConnection();
  }
}

cmd parsecmd(char s[BUFSZ])
{
  cmd comando;

  int value;
  char *val = strtok(s, " ");
  comando.type = getType(val);
  for(int i = 0; i < SWITCHES_POR_COMANDO + 1; i++)
    comando.switches[i] = 0;

  switch (comando.type) 
  {
    case instalar:
      val = strtok(NULL, " "); 
      if(!stringEqual(val, "sw"))
        logexit("formato incorreto");

      for(int i = 0; i < SWITCHES_POR_COMANDO + 1; i++) {
        val = strtok(NULL, " ");
        
        if(val == NULL)
          logexit("comando incompleto");
        
        if(stringEqual(val, "in")) {
          break;
        }

        if(sscanf(val, "%d", &value) != 1)
          logexit("formato incorreto");
        
        comando.switches[i] = value;
      }

      val = strtok(NULL, " ");
      if(!stringEqual(val, "in") && sscanf(val, "%d", &value) != 1)
        logexit("esperava numero de rack");

      comando.rack = value;
      break;
    case desinstalar:
      val = strtok(NULL, " "); 
      if(!stringEqual(val, "sw"))
        logexit("formato incorreto");

      val = strtok(NULL, " ");
      if(val == NULL)
        logexit("comando incompleto");
        
      if(sscanf(val, "%d", &value) != 1)
        logexit("formato incorreto");
      
      comando.switches[0] = value;

      val = strtok(NULL, " ");
      if(val == NULL || !stringEqual(val, "in"))
        logexit("comando incompleto");
        
      val = strtok(NULL, " ");
      if(sscanf(val, "%d", &value) != 1)
        logexit("esperava numero de rack");

      comando.rack = value;
      break;
    case listar:
      val = strtok(NULL, " ");
      if(sscanf(val, "%d", &value) != 1)
        logexit("esperava numero de rack");

      comando.rack = value;
      break;
    case ler:
      for(int i = 0; i < 3; i++) {
        val = strtok(NULL, " ");
        if(val == NULL)
          logexit("comando incompleto");
        
        if(stringEqual(val, "in"))
          break;
        if(i > 1)
          logexit("formato incorreto");

        if(sscanf(val, "%d", &value) != 1)
          logexit("formato incorreto");
        
        comando.switches[i] = value;
      }

      val = strtok(NULL, " ");
      if(sscanf(val, "%d", &value) != 1)
        logexit("formato incorreto");

      comando.rack = value;
      break;
    
    default:
      logexit("tipo de comando desconhecido");
  }

  return comando;
}

bool hasSwitchTypeUnknownError(cmd comando)
{
  for(int i = 0; i < SWITCHES_POR_COMANDO; i++) 
    if(comando.switches[i] < 0 || comando.switches[i] > 4)
      return true;
  
  return false;
}

int getNumberOfSwitchesInRack(int rack) {
  int numberOfSwitches = 0;

  for(int i = 0; i < SWITCHES_POR_RACK; i++)
    if(racks[rack - 1][i])
      numberOfSwitches++;

  return numberOfSwitches;
}

int getNumberOfSwitchesInCommand(cmd comando) {
  int numberOfSwitches = 0;

  for(int i = 0; i < SWITCHES_POR_COMANDO + 1; i++)
    if(comando.switches[i])
      numberOfSwitches++;

  return numberOfSwitches;
}

bool hasRackLimitExceededError(cmd comando)
{
  int numberOfSwitchesInRack = getNumberOfSwitchesInRack(comando.rack);
  int remainingSpace = SWITCHES_POR_RACK - numberOfSwitchesInRack;
  int switchesInCommand = getNumberOfSwitchesInCommand(comando);
  return (remainingSpace - switchesInCommand) < 0;
}

int switchAlreadyInstalled(cmd comando)
{
  for(int i = 0; i < SWITCHES_POR_COMANDO; i++) {
    if(comando.switches[i]) {
      for(int j = 0; j < SWITCHES_POR_RACK; j++) {
        if(racks[comando.rack - 1][j] == comando.switches[i]) {
          return comando.switches[i];
        }
      }
    }
  }
  return 0;
}

bool rackHasSwitch(int rack, int switchId) {
  for(int i = 0; i < SWITCHES_POR_RACK; i++) {
    if(racks[rack - 1][i] == switchId)
      return true;
  }
  return false;
}

void instalarSwitch(cmd comando) {
  if(hasSwitchTypeUnknownError(comando)) {
    sendMessage(clientSocket, "error switch type unknown");
    return;
  }
  if(comando.rack > NUMERO_DE_RACKS) {
    sendMessage(clientSocket, "error rack doesn't exist");
    return;
  }
  if(hasRackLimitExceededError(comando)) {
    sendMessage(clientSocket, "error rack limit exceeded");
    return;
  }
  int repeatingSwitch = switchAlreadyInstalled(comando);
  if(repeatingSwitch) {
    char errorMessage[50];
    sprintf(errorMessage, "error switch 0%d already installed in 0%d", repeatingSwitch, comando.rack);
    sendMessage(clientSocket, errorMessage);
    return;
  }

  int switchIndex = 0;
  while(racks[comando.rack - 1][switchIndex])
    switchIndex++;
  for(int i = 0; i < SWITCHES_POR_COMANDO; i++) {
    if(comando.switches[i]) {
      racks[comando.rack - 1][switchIndex] = comando.switches[i];
      switchIndex++;
    }
  }

  char successMessage[50];
  int numberOfSwitchesInCommand = getNumberOfSwitchesInCommand(comando);
  if(numberOfSwitchesInCommand == 1)
    sprintf(successMessage, "switch 0%d installed", comando.switches[0]);
  if(numberOfSwitchesInCommand == 2)
    sprintf(successMessage, "switch 0%d 0%d installed", comando.switches[0], comando.switches[1]);
  if(numberOfSwitchesInCommand == 3)
    sprintf(successMessage, "switch 0%d 0%d 0%d installed", comando.switches[0], comando.switches[1], comando.switches[2]);
  sendMessage(clientSocket, successMessage);
}

void desinstalarSwitch(cmd comando) {
  if(comando.rack > NUMERO_DE_RACKS) {
    sendMessage(clientSocket, "error rack doesn't exist");
    return;
  }

  if(!rackHasSwitch(comando.rack, comando.switches[0])) {
    sendMessage(clientSocket, "error switch doesn’t exist");
    return;
  }

  for(int i = 0; i < SWITCHES_POR_RACK; i++) {
    if(racks[comando.rack - 1][i] == comando.switches[0])
     racks[comando.rack - 1][i] = 0;
  }
  char successMessage[50];
  sprintf(successMessage, "switch 0%d removed from 0%d", comando.switches[0], comando.rack);
  sendMessage(clientSocket, successMessage);
}



void listarSwitches(cmd comando) {
  int numberOfSwitchesInRack = getNumberOfSwitchesInRack(comando.rack);
  if(comando.rack > NUMERO_DE_RACKS) {
    sendMessage(clientSocket, "error rack doesn't exist");
    return;
  }
  if(numberOfSwitchesInRack == 0) {
    sendMessage(clientSocket, "empty rack");
    return;
  } 

  char successMessage[50];
  if(numberOfSwitchesInRack == 1)
    sprintf(successMessage, "0%d", racks[comando.rack - 1][0]);
  if(numberOfSwitchesInRack == 2)
    sprintf(successMessage, "0%d 0%d", racks[comando.rack - 1][0], racks[comando.rack - 1][1]);
  if(numberOfSwitchesInRack == 3)
    sprintf(successMessage, "0%d 0%d 0%d", racks[comando.rack - 1][0], racks[comando.rack - 1][1], racks[comando.rack - 1][2]);
  sendMessage(clientSocket, successMessage);
}

int getRandomKbs() {
  return rand() % MAX_KBS;
}

void lerDados(cmd comando) {
  if(comando.rack > NUMERO_DE_RACKS) {
    sendMessage(clientSocket, "error rack doesn't exist");
    return;
  }
  int numberOfSwitches = getNumberOfSwitchesInCommand(comando);
  if(!rackHasSwitch(comando.rack, comando.switches[0])
    || (numberOfSwitches > 1 && !rackHasSwitch(comando.rack, comando.switches[1]))) {
    sendMessage(clientSocket, "error switch doesn’t exist");
    return;
  }
  char successMessage[50];
  if(numberOfSwitches == 1)
    sprintf(successMessage, "%d Kbs", getRandomKbs());
  else
    sprintf(successMessage, "%d Kbs %d Kbs", getRandomKbs(), getRandomKbs());
  sendMessage(clientSocket, successMessage);
}

void runcmd(cmd comando)
{
  switch (comando.type) 
  {
    case instalar:
      instalarSwitch(comando);
      break;
    case desinstalar:
      desinstalarSwitch(comando);
      break;
    case listar:
      listarSwitches(comando);
      break;
    case ler:
      lerDados(comando);
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

void printRacks() {
  printf("Racks: \n");
  for(int i = 0; i < NUMERO_DE_RACKS; i ++) {
    for(int j = 0; j < SWITCHES_POR_RACK; j++)
      printf("%d ", racks[i][j]);
    printf("\n");
  }
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
    runcmd(parsecmd(buf));
  }

  terminateConnection();
}
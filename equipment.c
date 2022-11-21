#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int equipments[NUMERO_MAX_CONEXOES] = {0};

int getcmd(char *buf, int nbuf)
{
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  return 0;
};

void successfullAdd(cmd comando) {
  printf("New ID: %02d \n", comando.payload[0]);
}

void printEquipiments() {
  printf("Equipments:");
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(equipments[i])
      printf(" %02d", equipments[i]);
  }
  printf("\n");
}

void receiveEquipmentList(cmd comando) {
  int equipmentListIndex = 0;
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
    if(comando.payload[i] != 0) {
      equipments[equipmentListIndex] = comando.payload[i];
      equipmentListIndex++;
    }
  }
  printEquipiments();
}

void runcmd(cmd comando)
{
  switch (comando.type) 
  {
    case RES_ADD:
      successfullAdd(comando);
      break;
    case RES_LIST:
      receiveEquipmentList(comando);
      break;
    
    default:
      logexit("tipo de comando desconhecido");
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3)
    logexit("parametros nao informados");

  char *ipAdress = argv[1];
  char *port = argv[2];

  struct sockaddr_storage storage = parseAddress(ipAdress, port);
  struct sockaddr *address = (struct sockaddr *)(&storage);

  int s = socket(storage.ss_family, SOCK_STREAM, 0);
  if(s == -1)
    logexit("falha ao inicializar socket");

  if(0 != connect(s, address, sizeof(storage)))
    logexit("falha ao conectar");

  sendMessage(s, "01");
  char receiveBuf[BUFSZ];

  receiveMessage(s, receiveBuf);
  runcmd(parsecmd(receiveBuf, s));

  receiveMessage(s, receiveBuf);
  runcmd(parsecmd(receiveBuf, s));

  static char buf[100];
	memset(receiveBuf, 0, BUFSZ);
  while(getcmd(buf, sizeof(buf)) >= 0) {
    sendMessage(s, buf);
    receiveMessage(s, receiveBuf);
    printf("%s\n", receiveBuf);
  }

  close(s);

  exit(EXIT_SUCCESS);
}
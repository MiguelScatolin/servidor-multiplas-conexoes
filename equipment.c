#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

int getcmd(char *buf, int nbuf)
{
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  return 0;
};

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

  static char buf[100];
  char receiveBuf[BUFSZ];
	memset(receiveBuf, 0, BUFSZ);
  while(getcmd(buf, sizeof(buf)) >= 0) {
    sendMessage(s, buf);
    if(stringEqual(buf, "exit") || stringEqual(buf, "exit\n"))
      break;
    recv(s, receiveBuf, BUFSZ - 1, 0);
    printf("%s\n", receiveBuf);
  }

  close(s);

  exit(EXIT_SUCCESS);
}
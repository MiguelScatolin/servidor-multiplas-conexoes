#ifndef COMMON_H
#define COMMON_H

#include "common.h"

void logexit(char *errorMessage) {
  if(errno)
    printf("errno: %d", errno);
  perror(errorMessage);
  exit(EXIT_FAILURE);
}

void sendMessage(int s, char *message) {
  printf("message sent: %s\n", message);
  size_t count = send(s, message, strlen(message)+1, 0);
	if (count != strlen(message)+1)
		logexit("send");
}

void receiveMessage(int s, char *buf) {
  recv(s, buf, BUFSZ - 1, 0);
  printf("message received: %s\n", buf);
}

char *addressToString(const struct sockaddr *address) {
  int version;
  char addressString[INET6_ADDRSTRLEN + 1] = "";
  uint16_t port;
  if(address->sa_family == AF_INET) {
    version = 4;
    struct sockaddr_in *addr4 = (struct sockaddr_in *)address;
    if(!inet_ntop(AF_INET, &(addr4->sin_addr), addressString, INET6_ADDRSTRLEN + 1))
      logexit("ntop");
    port = ntohs(addr4->sin_port);
  } else if(address->sa_family == AF_INET6) {
    version = 6;
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)address;
    if(!inet_ntop(AF_INET, &(addr6->sin6_addr), addressString, INET6_ADDRSTRLEN + 1))
      logexit("ntop");
    port = ntohs(addr6->sin6_port);
  } else 
    logexit("familia invalida");

  char *string;
  snprintf(string, 100, "IPv %d %s %d", version, addressString, port);
  return string;
}

struct sockaddr_storage parseAddress(char *ipAdressString, char *portString) {
  if(ipAdressString == NULL)
    logexit("ip address nulo");
  if(portString == NULL)
    logexit("port nulo");

  uint16_t port = (uint16_t)atoi(portString);
  if(port == 0)
    logexit("port invalida");
  port = htons(port); //host to network short

  struct sockaddr_storage storage;
  struct in_addr inAddress4;
  if(inet_pton(AF_INET, ipAdressString, &inAddress4)) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)(&storage);
    addr4->sin_family = AF_INET;
    addr4->sin_port = port;
    addr4->sin_addr = inAddress4;
    return storage;
  }

  struct in6_addr inAddress6;
  if(inet_pton(AF_INET6, ipAdressString, &inAddress6)) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)(&storage);
    addr6->sin6_family = AF_INET6;
    addr6->sin6_port = port;
    memcpy(&(addr6->sin6_addr), &inAddress6, sizeof(inAddress6));
    return storage;
  }

  logexit("nao foi possivel conectar");
};

bool stringEqual(char *s1, char *s2) {
  return strcmp(s1, s2) == 0;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port); // network to host short
    } else if (addr->sa_family == AF_INET6) {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr6->sin6_port); // network to host short
    } else {
        logexit("unknown protocol family.");
    }
    if (str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int convertToInt(char *string) {
  int value;
  if(sscanf(string, "%d", &value) != 1)
    return 0;
  return value;
}

message parseMessage(char s[BUFSZ], int socket)
{
  message comando;

  int value;
  char *val = strtok(s, " ");
  comando.type = convertToInt(val);
  comando.sourceSocket = socket;
  for(int i = 0; i < NUMERO_MAX_CONEXOES; i++)
    comando.payload[i] = 0;

  switch (comando.type) 
  {
    case REQ_ADD:
      break;

    case RES_ADD:
      val = strtok(NULL, " ");
      comando.payload[0] = convertToInt(val);
      break;

    case RES_LIST:
      int payloadIndex = 0;
      for(int i = 0; i < NUMERO_MAX_CONEXOES; i++) {
        val = strtok(NULL, " ");
        if(val == NULL)
          break;

        comando.payload[payloadIndex] = convertToInt(val);
        payloadIndex++;
      }
      break;

    case REQ_RM:
      val = strtok(NULL, " ");
      if(val == NULL)
          logexit("missing idEQ i");
      comando.sourceId = convertToInt(val);
      break;

    case OK:
      val = strtok(NULL, " ");
      if(val == NULL)
          logexit("missing idEQ j");
      comando.destinationId = convertToInt(val);

      val = strtok(NULL, " ");
      if(val == NULL)
          logexit("missing code");
      comando.payload[0] = convertToInt(val);
      break;

    default:
      logexit("tipo de comando desconhecido");
  }

  return comando;
}

#endif
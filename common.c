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
  size_t count = send(s, message, strlen(message)+1, 0);
	if (count != strlen(message)+1)
		logexit("send");
}

char *receiveMessage(int s) {
  char buf[BUFSZ];
  recv(s, buf, BUFSZ, 0);
  return buf;
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

#endif
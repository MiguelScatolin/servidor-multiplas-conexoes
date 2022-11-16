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

void sendMessage(int s, char *message);
char *receiveMessage(int s);
char *addressToString(const struct sockaddr *address);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
struct sockaddr_storage parseAddress(char *ipAdress, char *port);
void logexit(char *errorMessage);
bool stringEqual(char *s1, char *s2);
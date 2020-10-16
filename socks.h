#ifndef ENHANCER_SOCKS_H
#define ENHANCER_SOCKS_H

#include "common.h"

int socks_connect(const char *ProxyURL, const char *DestHost, int DestPort);

#endif


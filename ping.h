#ifndef __PING_H
#define __PING_H

#include <inttypes.h>
#include <utility/w5100.h>
#include <IPAddress.h>

const uint8_t PING_TRY = 3;
const uint32_t PING_TIMEOUT = 1000;

int32_t ping(SOCKET socket, const IPAddress &addr, uint8_t tryCount = PING_TRY, uint32_t timeOut = PING_TIMEOUT);
inline int32_t ping(const IPAddress &addr, uint8_t tryCount = PING_TRY, uint32_t timeOut = PING_TIMEOUT) {
  return ping(MAX_SOCK_NUM, addr, tryCount, timeOut);
}

#endif

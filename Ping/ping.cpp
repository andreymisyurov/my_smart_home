#include "ping.h"

//#define DEBUG

const uint16_t REQ_DATASIZE = 32;
const uint8_t ICMP_ECHOREPLY = 0;
const uint8_t ICMP_ECHOREQ = 8;

struct icmp_t {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint16_t id;
  uint16_t seq;
  uint8_t data[REQ_DATASIZE];
};

#ifdef DEBUG
static void dumpSocket(SOCKET socket, const __FlashStringHelper *title) {
  uint8_t buf[6];

  Serial.print(title);
  Serial.print(F(" SnMR: "));
  Serial.print(W5100.readSnMR(socket), BIN);
  Serial.print(F(", SnIR: "));
  Serial.print(W5100.readSnIR(socket), BIN);
  Serial.print(F(", SnSR: "));
  Serial.print(W5100.readSnSR(socket), HEX);
  Serial.print(F(", SnPORT: "));
  Serial.print(W5100.readSnPORT(socket));
  Serial.print(F(", SnDHAR: "));
  W5100.readSnDHAR(socket, buf);
  for (uint8_t i = 0; i < 6; ++i) {
    if (i)
      Serial.print(':');
    if (buf[i] < 0x10)
      Serial.print('0');
    Serial.print(buf[i], HEX);
  }
  Serial.print(F(", SnDIPR: "));
  W5100.readSnDIPR(socket, buf);
  for (uint8_t i = 0; i < 4; ++i) {
    if (i)
      Serial.print('.');
    Serial.print(buf[i]);
  }
  Serial.print(F(", SnDPORT: "));
  Serial.println(W5100.readSnDPORT(socket));
}
#endif

static void calcCheckSum(icmp_t *icmp) {
  uint16_t size = sizeof(icmp_t);
  uint16_t *w = (uint16_t*)icmp;
  uint32_t sum = 0;

  icmp->checksum = 0;
  while (size > 1) {
    sum += *w++;
    size -= 2;
  }
  if (size)
    sum += *(uint8_t*)w;
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  icmp->checksum = ~sum;
}

static void openSocket(SOCKET socket) {
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  W5100.writeSnMR(socket, SnMR::IPRAW);
  W5100.writeSnPROTO(socket, IPPROTO::ICMP);
  W5100.writeSnPORT(socket, 0);
  W5100.execCmdSn(socket, Sock_OPEN);
  {
    uint8_t _addr[6];

    _addr[0] = _addr[1] = _addr[2] = _addr[3] = _addr[4] = _addr[5] = 0xFF;
    W5100.writeSnDIPR(socket, _addr);
    W5100.writeSnDHAR(socket, _addr);
  }
  W5100.execCmdSn(socket, Sock_SEND);
  W5100.writeSnIR(socket, SnIR::SEND_OK);
#ifdef DEBUG
  dumpSocket(socket, F("OPEN"));
#endif
  SPI.endTransaction();
}

static void closeSocket(SOCKET socket) {
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  W5100.execCmdSn(socket, Sock_CLOSE);
  W5100.writeSnIR(socket, 0xFF);
  SPI.endTransaction();
//  EthernetClass::_server_port[socket] = 0;
}

static void send_data_processing(SOCKET s, const uint8_t *data, uint16_t len) {
  uint16_t ptr = W5100.readSnTX_WR(s);
  uint16_t offset = ptr & W5100.SMASK;
  uint16_t dstAddr = offset + W5100.SBASE(s);

  if (offset + len > W5100.SSIZE) {
    // Wrap around circular buffer
    uint16_t size = W5100.SSIZE - offset;

    W5100.write(dstAddr, data, size);
    W5100.write(W5100.SBASE(s), data + size, len - size);
  } else {
    W5100.write(dstAddr, data, len);
  }
  ptr += len;
  W5100.writeSnTX_WR(s, ptr);
}

static size_t sendToSocket(SOCKET socket, const IPAddress &addr, const uint8_t *buf, size_t size) {
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  W5100.writeSnDPORT(socket, 0);
  {
    uint8_t rawaddr[4];

    *(uint32_t*)rawaddr = (uint32_t)addr;
    W5100.writeSnDIPR(socket, rawaddr);
  }
  send_data_processing(socket, buf, size);
  W5100.execCmdSn(socket, Sock_SEND);
#ifdef DEBUG
  dumpSocket(socket, F("SEND"));
#endif
  while ((W5100.readSnIR(socket) & SnIR::SEND_OK) != SnIR::SEND_OK) {
    if (W5100.readSnIR(socket) & SnIR::TIMEOUT) {
      W5100.writeSnIR(socket, (SnIR::SEND_OK | SnIR::TIMEOUT));
      SPI.endTransaction();
      return 0;
    }
  }
  W5100.writeSnIR(socket, SnIR::SEND_OK);
  SPI.endTransaction();
  return size;
}

static void read_data(SOCKET s, volatile uint16_t src, volatile uint8_t *dst, uint16_t len) {
  uint16_t size;
  uint16_t src_mask;
  uint16_t src_ptr;

  src_mask = src & W5100.SMASK; // RMASK!
  src_ptr = W5100.RBASE(s) + src_mask;

  if ((src_mask + len) > W5100.SSIZE) { // RSIZE!
    size = W5100.SSIZE - src_mask;
    W5100.read(src_ptr, (uint8_t*)dst, size);
    dst += size;
    W5100.read(W5100.RBASE(s), (uint8_t*)dst, len - size);
  } else
    W5100.read(src_ptr, (uint8_t*)dst, len);
}

static size_t recvFromSocket(SOCKET socket, uint8_t *buf, size_t size) {
  uint8_t header[6];
  uint16_t buffer;

  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  buffer = W5100.readSnRX_RD(socket);
  read_data(socket, buffer, header, sizeof(header));
  buffer += sizeof(header);
  uint16_t dataLen = (header[4] << 8) | header[5];
  if (dataLen > size)
    dataLen = size;
  read_data(socket, buffer, buf, dataLen);
  buffer += dataLen;
  W5100.writeSnRX_RD(socket, buffer);
  W5100.execCmdSn(socket, Sock_RECV);
#ifdef DEBUG
  dumpSocket(socket, F("RECV"));
#endif
  SPI.endTransaction();
  return dataLen;
}

static uint16_t getRXReceivedSize(SOCKET s) {
  uint16_t val = 0, val1 = 0;

  do {
    val1 = W5100.readSnRX_RSR(s);
    if (val1 != 0)
      val = W5100.readSnRX_RSR(s);
  } while (val != val1);
  return val;
}

static bool waitForSocket(SOCKET socket, uint32_t timeOut) {
  bool result = true;
  uint32_t time = millis();

  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  while (! getRXReceivedSize(socket)) {
    if (millis() - time > timeOut) {
      result = false;
      break;
    }
  }
#ifdef DEBUG
  dumpSocket(socket, F("RXSIZE"));
#endif
  SPI.endTransaction();
  return result;
}

static SOCKET getFreeSocket() {
  SOCKET s;

  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
  for (s = 0; s < MAX_SOCK_NUM; ++s) {
    uint8_t status = W5100.readSnSR(s);
    if ((status == SnSR::CLOSED) || (status == SnSR::FIN_WAIT) || (status == SnSR::CLOSE_WAIT))
      break;
  }
  SPI.endTransaction();
  return s; // First free socket or MAX_SOCK_NUM
}

int32_t ping(SOCKET socket, const IPAddress &addr, uint8_t tryCount, uint32_t timeOut) {
  int32_t result = -1;
  icmp_t icmp;

  if (socket < MAX_SOCK_NUM)
    closeSocket(socket);
  else {
    socket = getFreeSocket();
    if (socket == MAX_SOCK_NUM)
      return -2;
  }
  openSocket(socket);
  while (tryCount--) {
    icmp.type = ICMP_ECHOREQ;
    icmp.code = 0;
    icmp.id = 0;
    icmp.seq = 0;
    for (uint8_t i = sizeof(uint32_t); i < sizeof(icmp.data); ++i)
      icmp.data[i] = 'A' + i - sizeof(uint32_t);
    *(uint32_t*)icmp.data = millis();
    calcCheckSum(&icmp);
    if (sendToSocket(socket, addr, (uint8_t*)&icmp, sizeof(icmp_t))) {
      if (waitForSocket(socket, timeOut)) {
        if (recvFromSocket(socket, (uint8_t*)&icmp, sizeof(icmp_t)) == sizeof(icmp_t)) {
          result = millis() - *(uint32_t*)icmp.data;
          break;
        }
#ifdef DEBUG
        else
          Serial.println(F("Ping receive error!"));
#endif
      }
#ifdef DEBUG
      else
        Serial.println(F("Ping wait timeout!"));
#endif
    }
#ifdef DEBUG
    else
      Serial.println(F("Ping send error!"));
#endif
  }
  closeSocket(socket);

  return result;
}

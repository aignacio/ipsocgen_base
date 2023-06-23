#include <stdbool.h>
#include <stdint.h>
#include "csr_eth.h"
#include "eth.h"

static volatile uint32_t* const ethINFIFO              = (uint32_t*) ethINFIFO_ADDR;
static volatile uint32_t* const ethOUTFIFO             = (uint32_t*) ethOUTFIFO_ADDR;
static volatile uint32_t* const ethCSR_LOC_MAC_LOW     = (uint32_t*) ethLOC_MAC_LOW;
static volatile uint32_t* const ethCSR_LOC_MAC_HIGH    = (uint32_t*) ethLOC_MAC_HIGH;
static volatile uint32_t* const ethCSR_LOC_IP          = (uint32_t*) ethLOC_IP;
static volatile uint32_t* const ethCSR_GATEWAY_IP      = (uint32_t*) ethGATEWAY_IP;
static volatile uint32_t* const ethCSR_SUBNET_MASK     = (uint32_t*) ethSUBNET_MASK;
static volatile uint32_t* const ethCSR_SEND_MAC_LOW    = (uint32_t*) ethSEND_MAC_LOW;
static volatile uint32_t* const ethCSR_SEND_MAC_HIGH   = (uint32_t*) ethSEND_MAC_HIGH;
static volatile uint32_t* const ethCSR_SEND_IP         = (uint32_t*) ethSEND_IP;
static volatile uint32_t* const ethCSR_SEND_LEN        = (uint32_t*) ethSEND_UDP_LEN;
static volatile uint32_t* const ethCSR_SEND_SRC_PORT   = (uint32_t*) ethSEND_UDP_SRC_PORT;
static volatile uint32_t* const ethCSR_SEND_DST_PORT   = (uint32_t*) ethSEND_UDP_DST_PORT;
static volatile uint32_t* const ethCSR_SEND_PKT        = (uint32_t*) ethSEND_PKT;
static volatile uint32_t* const ethCSR_SEND_FIFO_CLEAR = (uint32_t*) ethSEND_CLEAR;
static volatile uint32_t* const ethCSR_RECV_FIFO_CLEAR = (uint32_t*) ethRECV_CLEAR;
static volatile uint32_t* const ethCSR_SEND_RD_PTR     = (uint32_t*) ethSEND_RD_PTR;
static volatile uint32_t* const ethCSR_SEND_WR_PTR     = (uint32_t*) ethSEND_WR_PTR;
static volatile uint32_t* const ethCSR_RECV_RD_PTR     = (uint32_t*) ethRECV_RD_PTR;
static volatile uint32_t* const ethCSR_RECV_WR_PTR     = (uint32_t*) ethRECV_WR_PTR;
static volatile uint32_t* const ethCSR_UDP_LEN         = (uint32_t*) ethRECV_UDP_LEN;
static volatile uint32_t* const ethCSR_CLEAR_IRQ       = (uint32_t*) ethCLEAR_IRQ;
static volatile uint32_t* const ethCSR_ENABLE_FILTER   = (uint32_t*) ethFILTER_EN;
static volatile uint32_t* const ethCSR_FILTER_PORT     = (uint32_t*) ethFILTER_PORT;
static volatile uint32_t* const ethCSR_FILTER_IP       = (uint32_t*) ethFILTER_IP;

// Function to reverse elements of an array
static void vprvRev (uint8_t *arr, size_t n) {
  uint8_t tmp[n];

  for (size_t i=0;i<n;i++)
    tmp[i] = *(arr+i);

  for (size_t i=0;i<n;i++)
    *(arr+i) = tmp[n-1-i];
}

void vEthSetLocalCfg (EthLocalCfg_t cfg) {
  // Set MAC address
  vprvRev(&cfg.MACAddr.bytes[0], 8);
  /*dbg("\n\r<MACLOW> %d", (cfg.MACAddr.bytes[2]<<16|cfg.MACAddr.bytes[1]<<8|cfg.MACAddr.bytes[0]));*/
  /*dbg("\n\r<MACHIGH> %d", (cfg.MACAddr.bytes[5]<<16|cfg.MACAddr.bytes[4]<<8|cfg.MACAddr.bytes[3]));*/
  *ethCSR_LOC_MAC_LOW  = (cfg.MACAddr.bytes[2]<<16|cfg.MACAddr.bytes[1]<<8|cfg.MACAddr.bytes[0]);
  *ethCSR_LOC_MAC_HIGH = (cfg.MACAddr.bytes[5]<<16|cfg.MACAddr.bytes[4]<<8|cfg.MACAddr.bytes[3]);
  // Set IPv4 address
  vprvRev(&cfg.IPAddr.bytes[0], 4);
  /*dbg("\n\r<IPL> %x", cfg.IPAddr.word);*/
  *ethCSR_LOC_IP = cfg.IPAddr.word;
  // Set IPv4 addr. gateway
  vprvRev(&cfg.IPGateway.bytes[0], 4);
  /*dbg("\n\r<IPG> %x", cfg.IPGateway.word);*/
  *ethCSR_GATEWAY_IP = cfg.IPGateway.word;
  // Set Subnet Mask
  vprvRev(&cfg.SubnetMask.bytes[0], 4);
  /*dbg("\n\r<IPSubnetMask> %x", cfg.SubnetMask.word);*/
  *ethCSR_SUBNET_MASK = cfg.SubnetMask.word;
}

void vEthSetSendCfg (EthCfg_t cfg) {
  // Set destination MAC address
  vprvRev(&cfg.MACAddr.bytes[0], 8);
  *ethCSR_SEND_MAC_LOW  = (cfg.MACAddr.bytes[2]<<16|cfg.MACAddr.bytes[1]<<8|cfg.MACAddr.bytes[0]);
  *ethCSR_SEND_MAC_HIGH = (cfg.MACAddr.bytes[5]<<16|cfg.MACAddr.bytes[4]<<8|cfg.MACAddr.bytes[3]);
  // Set destination IPv4 addr.
  vprvRev(&cfg.IPAddr.bytes[0], 4);
  *ethCSR_SEND_IP = cfg.IPAddr.word;
  // Set destination UDP src port
  *ethCSR_SEND_SRC_PORT = cfg.Src;
  // Set destination UDP dst port
  *ethCSR_SEND_DST_PORT = cfg.Dst;
  // Set destination UDP pkt len
  *ethCSR_SEND_LEN = cfg.Len;
}

void vEthSetSendLenCfg (EthUDPLen_t len) {
  // Set destination UDP pkt len
  *ethCSR_SEND_LEN = len;
}

void vEthSetFilter (EthFilterCfg_t cfg) {
  // Set enable filter
  *ethCSR_ENABLE_FILTER = cfg.Enable;
  // Set UDP port to filter pkts
  *ethCSR_FILTER_PORT = cfg.UDPPort;
  // Set IPv4 address to filter pkts
  vprvRev(&cfg.IPAddr.bytes[0], 4);
  *ethCSR_FILTER_IP = cfg.IPAddr.word;
}

void vEthSendPkt (void) {
  *ethCSR_SEND_PKT = 0x1;
}

void vEthClearOutfifoPtr (void) {
  *ethCSR_SEND_FIFO_CLEAR = 0x1;
}

void vEthClearInfifoPtr (void) {
  *ethCSR_RECV_FIFO_CLEAR = 0x1;
}

void vEthWriteOutfifoWData (uint32_t val) {
  *ethOUTFIFO = val;
}

void vEthWriteOutfifoData (uint8_t *msg, uint16_t len) {
  //*ethOUTFIFO = (uint32_t)msg;

  uint32_t val;

  for (int i=0;i<len;i+=4){
    val = (*(msg+i+3)<<24)|(*(msg+i+2)<<16)|(*(msg+i+1)<<8)|*(msg+i);
    *ethOUTFIFO = val;
  }
}

EthFifoPtr_t xEthOutfifoPtr (void) {
  EthFifoPtr_t fifo;
  fifo.Write = *ethCSR_SEND_WR_PTR;
  fifo.Read = *ethCSR_SEND_RD_PTR;
  return fifo;
}

EthFifoPtr_t xEthInfifoPtr (void) {
  EthFifoPtr_t fifo;
  fifo.Write = *ethCSR_RECV_WR_PTR;
  fifo.Read = *ethCSR_RECV_RD_PTR;
  return fifo;
}

uint32_t ulEthGetRecvLen (void) {
  return *ethCSR_UDP_LEN;
}

uint32_t ulEthGetRecvData (void) {
  uint32_t ulTmp = *ethINFIFO;
  return ulTmp;
}

void vEthClearIRQs (void) {
  *ethCSR_CLEAR_IRQ = 0x1;
}

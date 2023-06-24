#ifndef ETH_H
#define ETH_H

#include <stdio.h>
#include <stdint.h>
#include "soc.h"
#include "csr_eth.h"
#include "uart.h" // Required for dbg crash macro


#define ethINFIFO_ADDR       masterETH_INFIFO_BASE_ADDR
#define ethOUTFIFO_ADDR      masterETH_OUTFIFO_BASE_ADDR
#define ethCSR_ADDR          masterETH_CSR_BASE_ADDR

#define ethLOC_MAC_LOW       (uint32_t*)(ethCSR_ADDR+ETH_CSR_ETH_MAC_LOW_BYTE_OFFSET)
#define ethLOC_MAC_HIGH      (uint32_t*)(ethCSR_ADDR+ETH_CSR_ETH_MAC_HIGH_BYTE_OFFSET)
#define ethLOC_IP            (uint32_t*)(ethCSR_ADDR+ETH_CSR_ETH_IP_BYTE_OFFSET)
#define ethGATEWAY_IP        (uint32_t*)(ethCSR_ADDR+ETH_CSR_GATEWAY_IP_BYTE_OFFSET)
#define ethSUBNET_MASK       (uint32_t*)(ethCSR_ADDR+ETH_CSR_SUBNET_MASK_BYTE_OFFSET)
#define ethSEND_MAC_LOW      (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_MAC_LOW_BYTE_OFFSET)
#define ethSEND_MAC_HIGH     (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_MAC_HIGH_BYTE_OFFSET)
#define ethSEND_IP           (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_IP_BYTE_OFFSET)
#define ethSEND_UDP_LEN      (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_UDP_LENGTH_BYTE_OFFSET)
#define ethSEND_UDP_SRC_PORT (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_SRC_PORT_BYTE_OFFSET)
#define ethSEND_UDP_DST_PORT (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_DST_PORT_BYTE_OFFSET)
#define ethSEND_PKT          (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_PKT_BYTE_OFFSET)
#define ethSEND_CLEAR        (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_FIFO_CLEAR_BYTE_OFFSET)
#define ethRECV_CLEAR        (uint32_t*)(ethCSR_ADDR+ETH_CSR_RECV_FIFO_CLEAR_BYTE_OFFSET)
#define ethSEND_WR_PTR       (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_FIFO_WR_PTR_BYTE_OFFSET)
#define ethSEND_RD_PTR       (uint32_t*)(ethCSR_ADDR+ETH_CSR_SEND_FIFO_RD_PTR_BYTE_OFFSET)
#define ethRECV_WR_PTR       (uint32_t*)(ethCSR_ADDR+ETH_CSR_RECV_FIFO_WR_PTR_BYTE_OFFSET)
#define ethRECV_RD_PTR       (uint32_t*)(ethCSR_ADDR+ETH_CSR_RECV_FIFO_RD_PTR_BYTE_OFFSET)
#define ethRECV_UDP_LEN      (uint32_t*)(ethCSR_ADDR+ETH_CSR_RECV_UDP_LENGTH_BYTE_OFFSET)
#define ethCLEAR_IRQ         (uint32_t*)(ethCSR_ADDR+ETH_CSR_CLEAR_IRQ_BYTE_OFFSET)
#define ethFILTER_EN         (uint32_t*)(ethCSR_ADDR+ETH_CSR_FILTER_EN_BYTE_OFFSET)
#define ethFILTER_PORT       (uint32_t*)(ethCSR_ADDR+ETH_CSR_FILTER_PORT_BYTE_OFFSET)
#define ethFILTER_IP         (uint32_t*)(ethCSR_ADDR+ETH_CSR_FILTER_IP_BYTE_OFFSET)

typedef uint16_t EthUDPPort_t;
typedef uint16_t EthUDPLen_t;

typedef union {
  uint8_t         bytes[4];
  uint32_t        word;
} EthSubNetMask_t;

typedef union {
  uint8_t         bytes[4];
  uint32_t        word;
} EthIPAddrV4_t;

typedef struct {
  uint32_t        Read;
  uint32_t        Write;
} EthFifoPtr_t;

typedef union {
  uint8_t         bytes[8];
  uint64_t        val;
} EthMACAddr_t;

typedef struct {
  EthMACAddr_t    MACAddr;
  EthIPAddrV4_t   IPAddr;
  EthIPAddrV4_t   IPGateway;
  EthSubNetMask_t SubnetMask;
} EthLocalCfg_t;

typedef struct {
  EthMACAddr_t    MACAddr;
  EthIPAddrV4_t   IPAddr;
  EthUDPPort_t    Src;
  EthUDPPort_t    Dst;
  EthUDPLen_t     Len;
} EthCfg_t;

typedef struct {
  uint8_t         Enable;
  EthUDPPort_t    UDPPort;
  EthIPAddrV4_t   IPAddr;
} EthFilterCfg_t;

// Prototypes
void vEthSetLocalCfg (EthLocalCfg_t cfg);
void vEthSetSendCfg (EthCfg_t cfg);
void vEthSetSendLenCfg (EthUDPLen_t len);
void vEthSetFilter (EthFilterCfg_t cfg);
void vEthSendPkt (void);
void vEthClearOutfifoPtr (void);
void vEthClearInfifoPtr (void);
void vEthWriteOutfifoWData (uint32_t val);
void vEthWriteOutfifoData (uint8_t *msg, uint16_t len);
EthFifoPtr_t xEthOutfifoPtr (void);
EthFifoPtr_t xEthInfifoPtr (void);
uint32_t ulEthGetRecvLen (void);
uint32_t ulEthGetRecvData (void);
void vEthClearIRQs (void);

#endif

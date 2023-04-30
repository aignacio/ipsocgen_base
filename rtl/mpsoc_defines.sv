/**
 * File              : mpsoc_defines.sv
 * License           : MIT license <Check LICENSE>
 * Author            : IPSoCGen
 * Date              : 30/04/2023 14:21:55
 * Description       : Verilog defines configuration file
 * -------------------------------------------
 * -- Design AUTO-GENERATED using IPSoC Gen --
 * -------------------------------------------
 **/
/* verilator lint_off REDEFMACRO */

// RaveNoC CFG
`define FLIT_BUFF             	2
`define FLIT_DATA_WIDTH       	32
`define H_PRIORITY            	ZeroHighPrior
`define ROUTING_ALG           	XYAlg
`define MAX_SZ_PKT            	256
`define NOC_CFG_SZ_ROWS       	3
`define NOC_CFG_SZ_COLS       	3
`define RAVENOC_BASE_ADDR     	'h64000
`define N_VIRT_CHN            	1
`define RD_AXI_BFF(x)         	x<=2?2:4
`define NUM_TILES             	9

// AXI Configuration
`define AXI_WR_BFF_BASE_ADDR  	`RAVENOC_BASE_ADDR+'h1000
`define AXI_RD_BFF_BASE_ADDR  	`RAVENOC_BASE_ADDR+'h2000
`define AXI_CSR_BASE_ADDR     	`RAVENOC_BASE_ADDR+'h3000
`define AXI_ADDR_WIDTH          32
`define AXI_DATA_WIDTH          32
`define AXI_ALEN_WIDTH          8
`define AXI_ASIZE_WIDTH         3
`define AXI_MAX_OUTSTD_RD       2
`define AXI_MAX_OUTSTD_WR       2
`define AXI_USER_RESP_WIDTH     1
`define AXI_USER_REQ_WIDTH      1
`define AXI_USER_DATA_WIDTH     1
`define AXI_TXN_ID_WIDTH        8

// DMA HW Config.
`define DMA_MAX_BURST_EN        0
`define DMA_EN_UNALIGNED        0
`define DMA_MAX_BEAT_BURST      4

/* verilator lint_on REDEFMACRO */
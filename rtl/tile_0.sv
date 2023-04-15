/**
 * File              : tile_0.sv
 * License           : MIT license <Check LICENSE>
 * Author            : IPSoCGen
 * Date              : 15/04/2023 12:16:24
 * Description       : MPSoC tile no 0
 * -------------------------------------------
 * -- Design AUTO-GENERATED using IPSoC Gen --
 * -------------------------------------------
 **/
module tile_0
  import amba_axi_pkg::*;
  import ravenoc_pkg::*;
  import eth_pkg::*;
(
  input		logic	clk_in,
  input		logic	rst_in,
  input		s_irq_ni_t	irq_ravenoc,
  input		s_axi_miso_t	noc_axi_miso_i,
  input		logic	uart_rx_i,
  input		logic	bootloader_i,
  input		logic	phy_rx_clk,
  input		logic [3:0]	phy_rxd,
  input		logic	phy_rx_ctl,
  input		logic	phy_int_n,
  input		logic	phy_pme_n,
  input		logic	clk_in_100MHz,
  output	s_axi_mosi_t	noc_axi_mosi_o,
  output	logic	uart_tx_o,
  output	logic	phy_tx_clk,
  output	logic [3:0]	phy_txd,
  output	logic	phy_tx_ctl,
  output	logic	phy_reset_n
);

  logic clk_int;
  logic rst_int;
  logic [31:0] rst_addr;
  logic irq_uart_rx;
  logic irq_mtimer;
  logic dma_done;
  logic dma_error;
  logic irq_ctrl_ext;
  logic eth_pkt_recv_irq;
  logic eth_pkt_sent_irq;
  logic eth_pkt_recv_full_irq;
  s_rgmii_tx_t phy_tx;
  s_rgmii_rx_t phy_rx;


  assign clk_int = clk_in;

  
// Master ID  Description
// 0          NoX CPU - Instr. IF
// 1          NoX CPU - LSU IF
// 2          DMA Engine

// Slave ID  Base Addr  End Addr  Size (KiB)  Description
// 0         0x0        0x7fff    32          RaveNoC Slave IF
// 1         0x8000     0xffff    32          Instruction RAM
// 2         0x10000    0x13fff   16          Data RAM
// 3         0x18000    0x1ffff   32          Boot ROM image
// 4         0x20000    0x21fff   8           UART Serial IP
// 5         0x22000    0x23fff   8           Machine Timer mtimer
// 6         0x24000    0x25fff   8           DMA Engine Control CSRs
// 7         0x26000    0x27fff   8           IRQ Controller
// 8         0x28000    0x29fff   8           Reset Controller
// 9         0x2a000    0x2afff   4           Ethernet Slave RGMII
// 10        0x2b000    0x2bfff   4           Ethernet InFIFO IF
// 11        0x2c000    0x2cfff   4           Ethernet OutFIFO IF

  s_axi_mosi_t  [2:0] masters_axi_mosi;
  s_axi_miso_t  [2:0] masters_axi_miso;
  s_axi_mosi_t  [11:0] slaves_axi_mosi;
  s_axi_miso_t  [11:0] slaves_axi_miso;

  //
  // AXI4 Crossbar
  //
  axi_crossbar_wrapper #(
    .ADDR_WIDTH       (32),
    .DATA_WIDTH       (32),
    .N_MASTERS        (3),
    .N_SLAVES         (12),
    .AXI_TID_WIDTH    (8),
    .M_BASE_ADDR      ({32'h2c000,
                        32'h2b000,
                        32'h2a000,
                        32'h28000,
                        32'h26000,
                        32'h24000,
                        32'h22000,
                        32'h20000,
                        32'h18000,
                        32'h10000,
                        32'h8000,
                        32'h0}),
    .M_ADDR_WIDTH     ({32'd12,
                        32'd12,
                        32'd12,
                        32'd13,
                        32'd13,
                        32'd13,
                        32'd13,
                        32'd13,
                        32'd15,
                        32'd14,
                        32'd15,
                        32'd15})
  ) u_axi4_crossbar (
    .clk              (clk_int),
    .rst              (rst_int),
    .*
  );

  //
  // RISC-V RV32I - NoX CPU
  //
  nox_wrapper u_nox (
    .clk              (clk_int),
    .rst              (rst_int),
    .irq_i            ({irq_mtimer,irq_uart_rx,irq_ctrl_ext}),
    .start_fetch_i    (1'b1),
    .start_addr_i     (rst_addr),
    .instr_axi_mosi_o (masters_axi_mosi[0]),
    .instr_axi_miso_i (masters_axi_miso[0]),
    .lsu_axi_mosi_o   (masters_axi_mosi[1]),
    .lsu_axi_miso_i   (masters_axi_miso[1])
  );

  //
  // RaveNoC Slave IF
  //
  assign noc_axi_mosi_o = slaves_axi_mosi[0];
  assign slaves_axi_miso[0] = noc_axi_miso_i;

  //
  // Instruction RAM
  //
  axi_mem_wrapper #(
    .MEM_KB           (30),
    .ID_WIDTH         (8)
  ) u_iram (
    .clk              (clk_int),
    .rst              (rst_int),
    .axi_mosi         (slaves_axi_mosi[1]),
    .axi_miso         (slaves_axi_miso[1])
  );

  // synthesis translate_off
  function automatic void writeWordRAM__iram(addr_val, word_val);
    /*verilator public*/
    logic [31:0] addr_val;
    logic [31:0] word_val;
    u_iram.mem_loading[addr_val] = word_val;
  endfunction
  // synthesis translate_on

  //
  // Data RAM
  //
  axi_mem_wrapper #(
    .MEM_KB           (16),
    .ID_WIDTH         (8)
  ) u_dram (
    .clk              (clk_int),
    .rst              (rst_int),
    .axi_mosi         (slaves_axi_mosi[2]),
    .axi_miso         (slaves_axi_miso[2])
  );

  // synthesis translate_off
  function automatic void writeWordRAM__dram(addr_val, word_val);
    /*verilator public*/
    logic [31:0] addr_val;
    logic [31:0] word_val;
    u_dram.mem_loading[addr_val] = word_val;
  endfunction
  // synthesis translate_on

  //
  // Boot ROM image
  //
  axi_rom_wrapper u_boot_rom (
    .clk              (clk_int),
    .rst              (rst_int),
    .axi_mosi         (slaves_axi_mosi[3]),
    .axi_miso         (slaves_axi_miso[3])
  );

  //
  // UART Serial IP
  //
  axi_uart_wrapper u_uart (
    .clk              (clk_int),
    .rst              (rst_int),
    .axi_mosi         (slaves_axi_mosi[4]),
    .axi_miso         (slaves_axi_miso[4]),
    .uart_tx_o        (uart_tx_o),
    .uart_rx_i        (uart_rx_i),
    .uart_rx_irq_o    (irq_uart_rx)
  );

  //
  // Machine Timer mtimer
  //
  axi_timer #(
    .BASE_ADDR        ('h22000)
  ) u_timer (
    .clk              (clk_int),
    .rst              (rst_int),
    .axi_mosi         (slaves_axi_mosi[5]),
    .axi_miso         (slaves_axi_miso[5]),
    .timer_irq_o      (irq_mtimer)
  );

  s_axil_mosi_t axil_mosi_dma_0;
  s_axil_miso_t axil_miso_dma_0;

  axil_to_axi u_axil_to_axi_dma_0 (
    .axi_mosi_i       (slaves_axi_mosi[6]),
    .axi_miso_o       (slaves_axi_miso[6]),
    .axil_mosi_o      (axil_mosi_dma_0),
    .axil_miso_i      (axil_miso_dma_0)
  );

  //
  // DMA Engine Control CSRs
  //
  dma_axi_wrapper #(
    .DMA_ID_VAL       (2)
  ) u_dma_0 (
    .clk              (clk_int),
    .rst              (rst_int),
    // CSR DMA I/F
    .dma_csr_mosi_i   (axil_mosi_dma_0),
    .dma_csr_miso_o   (axil_miso_dma_0),
    // Master DMA I/F
    .dma_m_mosi_o     (masters_axi_mosi[2]),
    .dma_m_miso_i     (masters_axi_miso[2]),
    // Triggers - IRQs
    .dma_done_o       (dma_done),
    .dma_error_o      (dma_error)
  );

  logic [31:0] irq_vector_mapping;

  assign irq_vector_mapping = '0; // Default assignment
  assign irq_vector_mapping[0] = irq_ravenoc.irq_trig;
  assign irq_vector_mapping[1] = dma_error;
  assign irq_vector_mapping[2] = dma_done;
  assign irq_vector_mapping[3] = 1'b0;
  assign irq_vector_mapping[4] = 1'b0;
  assign irq_vector_mapping[5] = eth_pkt_recv_irq;
  assign irq_vector_mapping[6] = eth_pkt_sent_irq;
  assign irq_vector_mapping[7] = eth_pkt_recv_full_irq;

  //
  // IRQ Controller
  //
  axi_irq_ctrl #(
    .BASE_ADDR        ('h26000),
    .TYPE_OF_IRQ      ('hffffffff)
  ) u_irq_ctrl (
    .clk              (clk_int),
    .rst              (rst_int),
    .irq_i            (irq_vector_mapping),
    .irq_summary_o    (irq_ctrl_ext),
    .axi_mosi         (slaves_axi_mosi[7]),
    .axi_miso         (slaves_axi_miso[7])
  );

  //
  // Reset Controller
  //
  axi_rst_ctrl #(
    .RESET_VECTOR_ADDR ('h18000),
    .BASE_ADDR         ('h28000),
    .RESET_PULSE_WIDTH (4)
  ) u_rst_ctrl  (
    .clk               (clk_int),
    .rst               (rst_in),
    .bootloader_i      (bootloader_i),
    .axi_mosi          (slaves_axi_mosi[8]),
    .axi_miso          (slaves_axi_miso[8]),
    .rst_addr_o        (rst_addr),
    .rst_o             (rst_int)
  );

  // synthesis translate_off
  function automatic void writeRstAddr__rst_ctrl(rst_addr);
    /*verilator public*/
    logic [31:0] rst_addr;
    u_rst_ctrl.rst_loading = rst_addr;
  endfunction
  // synthesis translate_on

  s_axil_mosi_t axil_mosi_eth_eth;
  s_axil_miso_t axil_miso_eth_eth;

  axil_to_axi u_axil_to_axi_eth_eth (
    .axi_mosi_i       (slaves_axi_mosi[9]),
    .axi_miso_o       (slaves_axi_miso[9]),
    .axil_mosi_o      (axil_mosi_eth_eth),
    .axil_miso_i      (axil_miso_eth_eth)
  );

  // Ethernet: 1000BASE-T RGMII
  assign phy_rx.phy_rx_clk = phy_rx_clk;
  assign phy_rx.phy_rxd    = phy_rxd;
  assign phy_rx.phy_rx_ctl = phy_rx_ctl;
  assign phy_rx.phy_int_n  = phy_int_n;
  assign phy_rx.phy_pme_n  = phy_pme_n;

  assign phy_tx_clk   = phy_tx.phy_tx_clk;
  assign phy_txd      = phy_tx.phy_txd;
  assign phy_tx_ctl   = phy_tx.phy_tx_ctl;
  assign phy_reset_n  = phy_tx.phy_reset_n;

  //
  // Ethernet Slave RGMII
  //
  ethernet_wrapper u_eth (
    .clk_src            (clk_in_100MHz),
    .clk_axi            (clk_int), // Clk of the AXI bus
    .rst_axi            (rst_int), // Active-High
    // Ethernet Control and Status Registers
    .eth_csr_mosi_i     (axil_mosi_eth_eth),
    .eth_csr_miso_o     (axil_miso_eth_eth),
    // Ethernet inFIFO I/F
    .eth_infifo_mosi_i  (slaves_axi_mosi[10]),
    .eth_infifo_miso_o  (slaves_axi_miso[10]),
    // Ethernet outFIFO I/F
    .eth_outfifo_mosi_i (slaves_axi_mosi[11]),
    .eth_outfifo_miso_o (slaves_axi_miso[11]),
    // Ethernet: 1000BASE-T RGMII
    .phy_tx             (phy_tx),
    .phy_rx             (phy_rx),
    // IRQs
    .pkt_recv_o         (eth_pkt_recv_irq),
    .pkt_sent_o         (eth_pkt_sent_irq),
    .pkt_recv_full_o    (eth_pkt_recv_full_irq)
  );

endmodule
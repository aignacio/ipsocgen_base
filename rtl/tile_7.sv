/**
 * File              : tile_7.sv
 * License           : MIT license <Check LICENSE>
 * Author            : IPSoCGen
 * Date              : 14/04/2023 23:25:26
 * Description       : MPSoC tile no 7
 * -------------------------------------------
 * -- Design AUTO-GENERATED using IPSoC Gen --
 * -------------------------------------------
 **/
module tile_7
  import amba_axi_pkg::*;
  import ravenoc_pkg::*;
  import eth_pkg::*;
(
  input		logic	clk_in,
  input		logic	rst_in,
  input		s_irq_ni_t	irq_ravenoc,
  input		s_axi_miso_t	noc_axi_miso_i,
  input		logic	arty_a7_uart_rx,
  input		logic	bootloader_i,
  output	s_axi_mosi_t	noc_axi_mosi_o,
  output	logic	arty_a7_uart_tx
);

  logic clk_int;
  logic rst_int;
  logic [31:0] rst_addr;
  logic irq_uart_rx;
  logic irq_mtimer;
  logic dma_done;
  logic dma_error;
  logic irq_ctrl_ext;


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
// 5         0x22000    0x23fff   8           Machine Timer
// 6         0x24000    0x25fff   8           DMA Engine Control CSRs
// 7         0x26000    0x27fff   8           IRQ Controller
// 8         0x28000    0x29fff   8           Reset Controller

  s_axi_mosi_t  [2:0] masters_axi_mosi;
  s_axi_miso_t  [2:0] masters_axi_miso;
  s_axi_mosi_t  [8:0] slaves_axi_mosi;
  s_axi_miso_t  [8:0] slaves_axi_miso;

  //
  // AXI4 Crossbar
  //
  axi_crossbar_wrapper #(
    .ADDR_WIDTH       (32),
    .DATA_WIDTH       (32),
    .N_MASTERS        (3),
    .N_SLAVES         (9),
    .AXI_TID_WIDTH    (8),
    .M_BASE_ADDR      ({32'h28000,
                        32'h26000,
                        32'h24000,
                        32'h22000,
                        32'h20000,
                        32'h18000,
                        32'h10000,
                        32'h8000,
                        32'h0}),
    .M_ADDR_WIDTH     ({32'd13,
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
    .MEM_KB           (20),
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
    .uart_tx_o        (arty_a7_uart_tx),
    .uart_rx_i        (arty_a7_uart_rx),
    .uart_rx_irq_o    (irq_uart_rx)
  );

  //
  // Machine Timer
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

endmodule
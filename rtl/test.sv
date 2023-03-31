/**
 * File              : test.sv
 * License           : MIT license <Check LICENSE>
 * Author            : IPSoCGen
 * Date              : 26/03/2023 17:20:07
 * Description       : Description of the MP/SoC to be generated
 * --------------------------------------------
 * ---- Do not edit, design auto-generated ----
 * --------------------------------------------
 **/

module test
(
  input		logic	arty_a7_100MHz,
  input		logic	arty_a7_sw_0,
  input		logic	arty_a7_rst_n,
  input		logic	arty_a7_uart_rx,
  input		logic	arty_a7_sw_1,
  output	logic	arty_a7_uart_tx
);

  logic [31:0] rst_addr;
  logic irq_uart_rx;
  logic arty_a7_uart_tx; // IO Pin
  logic arty_a7_uart_rx; // IO Pin
  logic irq_mtimer;
  logic dma_done;
  logic dma_error;
  logic irq_ctrl_ext;
  logic arty_a7_sw_1; // IO Pin

  //
  // Clk/PLL signals
  //
  logic clk_in_buff;
  logic clk_buff_out;
  logic clk_feedback_in;
  logic clk_feedback_out;
  logic pll_locked;
  logic rst_pll;
  
  assign rst_pll = ~arty_a7_sw_0;
  
  //
  // PLLE2_BASE: Base Phase Locked Loop (PLL)
  //             7 Series
  PLLE2_ADV#(
    .BANDWIDTH           ("OPTIMIZED"),
    .COMPENSATION        ("ZHOLD"),
    .STARTUP_WAIT        ("FALSE"),
    .DIVCLK_DIVIDE       (2),
    .CLKFBOUT_MULT       (17),
    .CLKFBOUT_PHASE      (0.000),
    .CLKOUT0_DIVIDE      (17),
    .CLKOUT0_PHASE       (0.000),
    .CLKOUT0_DUTY_CYCLE  (0.500),
    .CLKIN1_PERIOD       (10)
  ) u_clk_pll (
    .CLKFBOUT            (clk_feedback_in),
    .CLKOUT0             (clk_buff_out),
     // Input clock control
    .CLKFBIN             (clk_feedback_out),
    .CLKIN1              (clk_in_buff),
    .CLKIN2              (1'b0),
     // Tied to always select the primary input clock
    .CLKINSEL            (1'b1),
    // Ports for dynamic reconfiguration
    .DADDR               (7'h0),
    .DCLK                (1'b0),
    .DEN                 (1'b0),
    .DI                  (16'h0),
    .DWE                 (1'b0),
    // Other control and status signals
    .LOCKED              (pll_locked),
    .PWRDWN              (1'b0),
    .RST                 (rst_pll)
  );

  IBUF clk_in_ibufg(
    .I (arty_a7_100MHz),
    .O (clk_in_buff)
  );

  BUFG clk_feedback_buf(
    .I (clk_feedback_in),
    .O (clk_feedback_out)
  );

  BUFG clk_out_buf(
    .I (clk_buff_out),
    .O (clk_50MHz)
  );

  
// Master ID  Description
// 0          NoX CPU - Instr. I/F
// 1          NoX CPU - LSU I/F
// 2          DMA Engine
// 3          Custom Master ACC.

// Slave ID  Base Addr  End Addr  Size (KiB)  Description
// 0         0x0        0xffff    64          Instruction RAM
// 1         0x10000    0x17fff   32          Data RAM
// 2         0x18000    0x1ffff   32          Boot ROM image
// 3         0x20000    0x21fff   8           UART Serial IP
// 4         0x22000    0x23fff   8           Machine Timer (mtimer)
// 5         0x24000    0x25fff   8           DMA Engine Control CSRs
// 6         0x26000    0x27fff   8           IRQ Controller
// 7         0x28000    0x29fff   8           Reset Controller
// 8         0x2a000    0x2bfff   8           My custom accelerator

  s_axi_mosi_t  [3:0] masters_axi_mosi;
  s_axi_miso_t  [3:0] masters_axi_miso;
  s_axi_mosi_t  [8:0] slaves_axi_mosi;
  s_axi_miso_t  [8:0] slaves_axi_miso;

  //
  // AXI4 Crossbar
  //
  axi_crossbar_wrapper #(
    .ADDR_WIDTH       (32),
    .DATA_WIDTH       (32),
    .N_MASTERS        (4),
    .N_SLAVES         (9),
    .AXI_TID_WIDTH    (8),
    .M_BASE_ADDR      ({32'h2a000,
                        32'h28000,
                        32'h26000,
                        32'h24000,
                        32'h22000,
                        32'h20000,
                        32'h18000,
                        32'h10000,
                        32'h0}),
    .M_ADDR_WIDTH     ({32'h13,
                        32'h13,
                        32'h13,
                        32'h13,
                        32'h13,
                        32'h13,
                        32'h15,
                        32'h15,
                        32'h16})
  ) u_axi4_crossbar (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .*
  );

  //
  // RISC-V RV32I - NoX CPU
  //
  nox_wrapper u_nox (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .irq_i            ({irq_mtimer,irq_uart_rx,irq_ctrl_ext}),
    .start_fetch_i    (1'b1),
    .start_addr_i     (rst_addr),
    .instr_axi_mosi_o (masters_axi_mosi[0]),
    .instr_axi_miso_i (masters_axi_miso[0]),
    .lsu_axi_mosi_o   (masters_axi_mosi[1]),
    .lsu_axi_miso_i   (masters_axi_miso[1])
  );

  //
  // Custom Master ACC.
  //
  axi_custom_master_acc u_custom_master (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .axi_mosi         (masters_axi_mosi[3]),
    .axi_miso         (masters_axi_miso[3])
  );

  //
  // Instruction RAM
  //
  axi_mem_wrapper #(
    .MEM_KB           (64),
    .ID_WIDTH         (8)
  ) u_iram (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .axi_mosi         (slaves_axi_mosi[0]),
    .axi_miso         (slaves_axi_miso[0])
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
    .MEM_KB           (32),
    .ID_WIDTH         (8)
  ) u_dram (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .axi_mosi         (slaves_axi_mosi[1]),
    .axi_miso         (slaves_axi_miso[1])
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
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .axi_mosi         (slaves_axi_mosi[2]),
    .axi_miso         (slaves_axi_miso[2])
  );

  //
  // UART Serial IP
  //
  axi_uart_wrapper u_uart (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .axi_mosi         (slaves_axi_mosi[3]),
    .axi_miso         (slaves_axi_miso[3]),
    .uart_tx_o        (arty_a7_uart_tx),
    .uart_rx_i        (arty_a7_uart_rx),
    .uart_rx_irq_o    (irq_uart_rx)
  );

  //
  // Machine Timer (mtimer)
  //
  axi_timer #(
    .BASE_ADDR        ('h22000)
  ) u_timer (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .axi_mosi         (slaves_axi_mosi[4]),
    .axi_miso         (slaves_axi_miso[4]),
    .timer_irq_o      (irq_mtimer)
  );

  s_axil_mosi_t axil_mosi_dma_0;
  s_axil_miso_t axil_miso_dma_0;

  axil_to_axi u_axil_to_axi_dma_0 (
    .axi_mosi_i       (slaves_axi_mosi[5]),
    .axi_miso_o       (slaves_axi_miso[5]),
    .axil_mosi_o      (axil_mosi_dma_0),
    .axil_miso_i      (axil_miso_dma_0)
  );

  //
  // DMA Engine Control CSRs
  //
  dma_axi_wrapper #(
    .DMA_ID_VAL       (2)
  ) u_dma_0 (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
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

  logic irq_vector_mapping [31:0];

  assign irq_vector_mapping = '0; // Default assignment
  assign irq_vector_mapping[0] = dma_error;
  assign irq_vector_mapping[1] = dma_done;

  //
  // IRQ Controller
  //
  axi_irq_ctrl #(
    .BASE_ADDR        ('h26000),
    .TYPE_OF_IRQ      ('hffffffff)
  ) u_irq_ctrl (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .irq_i            (irq_vector_mapping),
    .irq_summary_o    (irq_ctrl_ext),
    .axi_mosi         (slaves_axi_mosi[6]),
    .axi_miso         (slaves_axi_miso[6])
  );

  //
  // Reset Controller
  //
  axi_rst_ctrl #(
    .RESET_VECTOR_ADDR ('h0),
    .BASE_ADDR         ('h28000),
    .RESET_PULSE_WIDTH (4)
  ) u_rst_ctrl  (
    .clk               (clk_50MHz),
    .rst               (~arty_a7_rst_n),
    .bootloader_i      (arty_a7_sw_1),
    .axi_mosi          (slaves_axi_mosi[7]),
    .axi_miso          (slaves_axi_miso[7]),
    .rst_addr_o        (rst_addr),
    .rst_o             (rst_int_soc)
  );

  // synthesis translate_off
  function automatic void writeRstAddr__rst_ctrl(rst_addr);
    /*verilator public*/
    logic [31:0] rst_addr;
    u_rst_ctrl.rst_loading = rst_addr;
  endfunction
  // synthesis translate_on

  //
  // My custom accelerator
  //
  axi_custom_slave_acc #(
    .BASE_ADDR        ('h2a000)
  ) u_acc_custom (
    .clk              (clk_50MHz),
    .rst              (rst_int_soc),
    .axi_mosi         (slaves_axi_mosi[8]),
    .axi_miso         (slaves_axi_miso[8])
  );

endmodule
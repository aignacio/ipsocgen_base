/**
 * File              : mpsoc.sv
 * License           : MIT license <Check LICENSE>
 * Author            : IPSoCGen
 * Date              : 15/04/2023 12:16:24
 * Description       : Description of the MP/SoC to be generated
 * -------------------------------------------
 * -- Design AUTO-GENERATED using IPSoC Gen --
 * -------------------------------------------
 **/
module mpsoc
  import amba_axi_pkg::*;
  import ravenoc_pkg::*;
  import eth_pkg::*;
(
  input		            arty_a7_100MHz,
  input		            arty_a7_pll_rst,
  input		            arty_a7_rst,
  input               uart_switch_i, // 0 - Slave tiles, 1 - Master tile
  input               bootloader_i,  // Active high
  output  logic       uart_tx_o,
  input               uart_rx_i,
  // Ethernet: 1000BASE-T RGMII
  input               phy_rx_clk,
  input   [3:0]       phy_rxd,
  input               phy_rx_ctl,
  output  logic       phy_tx_clk,
  output  logic [3:0] phy_txd,
  output  logic       phy_tx_ctl,
  output  logic       phy_reset_n,
  input               phy_int_n,
  input               phy_pme_n

);

  logic clk_50MHz;
  s_axi_mosi_t [`NUM_TILES-1:0] slaves_axi_mosi;
  s_axi_mosi_t [`NUM_TILES-1:0] ravenoc_mosi;
  s_axi_miso_t [`NUM_TILES-1:0] slaves_axi_miso;
  s_axi_miso_t [`NUM_TILES-1:0] ravenoc_miso;
  s_irq_ni_t   [`NUM_TILES-1:0] irqs_ravenoc;
  logic rst_int_mpsoc;

  //
  // Clk/PLL signals
  //
  logic clk_in_buff;
  logic clk_buff_out;
  logic clk_feedback_in;
  logic clk_feedback_out;
  logic pll_locked;
  logic rst_pll;
  
  assign rst_pll = arty_a7_pll_rst;
  

`ifdef SIMULATION
  assign clk_50MHz = arty_a7_100MHz;
`else
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
`endif
  
  logic [`NUM_TILES-1:0] uart_tx;
  logic [`NUM_TILES-1:0] uart_rx;
  logic                  master_bootloader;
  logic                  slave_bootloader;

  logic                  uart_switch_int;
  logic                  bootloader_int;

  assign  uart_switch_int = uart_switch_i;
  assign  bootloader_int  = bootloader_i;
  
  assign rst_int_mpsoc = arty_a7_rst;
  
  always_comb begin
    master_bootloader = 1'b0;
    slave_bootloader = 1'b0;

    if (uart_switch_int) begin
      master_bootloader = bootloader_int;
      uart_tx_o = uart_tx[0]; // Connect Master Tile to the main UART
    end
    else begin
      slave_bootloader = bootloader_int;
      uart_tx_o = uart_tx[1]; // Connect Slave Tile #0 to the main UART
    end

    for (int i=0;i<`NUM_TILES;i++) begin
      uart_rx[i] = uart_rx_i;
    end
  end


  /* verilator lint_off PINMISSING */
  // tile_0
  tile_0 u_tile_0 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[0]),
    .noc_axi_mosi_o   (slaves_axi_mosi[0]),
    .irq_ravenoc      (irqs_ravenoc[0]),
    .clk_in_100MHz    (arty_a7_100MHz),
    .uart_tx_o        (uart_tx[0]),
    .uart_rx_i        (uart_rx[0]),
    .bootloader_i     (master_bootloader),
    .phy_rx_clk       (phy_rx_clk),
    .phy_rxd          (phy_rxd),
    .phy_rx_ctl       (phy_rx_ctl),
    .phy_int_n        (phy_int_n),
    .phy_pme_n        (phy_pme_n),
    .phy_tx_clk       (phy_tx_clk),
    .phy_txd          (phy_txd),
    .phy_tx_ctl       (phy_tx_ctrl),
    .phy_reset_n      (phy_reset_n)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_1
  tile_1 u_tile_1 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[1]),
    .noc_axi_mosi_o   (slaves_axi_mosi[1]),
    .irq_ravenoc      (irqs_ravenoc[1]),
    .uart_tx_o        (uart_tx[1]),
    .uart_rx_i        (uart_rx[1]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_2
  tile_2 u_tile_2 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[2]),
    .noc_axi_mosi_o   (slaves_axi_mosi[2]),
    .irq_ravenoc      (irqs_ravenoc[2]),
    .uart_tx_o        (uart_tx[2]),
    .uart_rx_i        (uart_rx[2]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_3
  tile_3 u_tile_3 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[3]),
    .noc_axi_mosi_o   (slaves_axi_mosi[3]),
    .irq_ravenoc      (irqs_ravenoc[3]),
    .uart_tx_o        (uart_tx[3]),
    .uart_rx_i        (uart_rx[3]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_4
  tile_4 u_tile_4 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[4]),
    .noc_axi_mosi_o   (slaves_axi_mosi[4]),
    .irq_ravenoc      (irqs_ravenoc[4]),
    .uart_tx_o        (uart_tx[4]),
    .uart_rx_i        (uart_rx[4]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_5
  tile_5 u_tile_5 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[5]),
    .noc_axi_mosi_o   (slaves_axi_mosi[5]),
    .irq_ravenoc      (irqs_ravenoc[5]),
    .uart_tx_o        (uart_tx[5]),
    .uart_rx_i        (uart_rx[5]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_6
  tile_6 u_tile_6 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[6]),
    .noc_axi_mosi_o   (slaves_axi_mosi[6]),
    .irq_ravenoc      (irqs_ravenoc[6]),
    .uart_tx_o        (uart_tx[6]),
    .uart_rx_i        (uart_rx[6]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_7
  tile_7 u_tile_7 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[7]),
    .noc_axi_mosi_o   (slaves_axi_mosi[7]),
    .irq_ravenoc      (irqs_ravenoc[7]),
    .uart_tx_o        (uart_tx[7]),
    .uart_rx_i        (uart_rx[7]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */
  /* verilator lint_off PINMISSING */
  // tile_8
  tile_8 u_tile_8 (
    // Mandatory IOs
    .clk_in           (clk_50MHz),
    .rst_in           (rst_int_mpsoc),
    .noc_axi_miso_i   (slaves_axi_miso[8]),
    .noc_axi_mosi_o   (slaves_axi_mosi[8]),
    .irq_ravenoc      (irqs_ravenoc[8]),
    .uart_tx_o        (uart_tx[8]),
    .uart_rx_i        (uart_rx[8]),
    .bootloader_i     (slave_bootloader)
  );
  /* verilator lint_on PINMISSING */

  always_comb begin
    ravenoc_mosi    = slaves_axi_mosi;
    slaves_axi_miso = ravenoc_miso;

    // NoC only accepts INCR type of burst
    for (int i=0;i<`NUM_TILES;i++) begin
      ravenoc_mosi[i].arburst = AXI_INCR;
      ravenoc_mosi[i].awburst = AXI_INCR;
    end
  end

  ravenoc #(
    .AXI_CDC_REQ      ('0)
  ) u_ravenoc (
    .clk_axi          ({ `NUM_TILES { clk_50MHz }}),
    .clk_noc          (clk_50MHz),
    .arst_axi         ({ `NUM_TILES { rst_int_mpsoc }}),
    .arst_noc         (rst_int_mpsoc),
    // NI interfaces
    .axi_mosi_if      (ravenoc_mosi),
    .axi_miso_if      (ravenoc_miso),
    // IRQs
    .irqs             (irqs_ravenoc),
    // Used only in tb to bypass cdc module
    .bypass_cdc       ('0)
  );

endmodule

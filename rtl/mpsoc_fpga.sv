/**
 * File              : mpsoc.sv
 * License           : MIT license <Check LICENSE>
 * Author            : Anderson Ignacio da Silva (aignacio) <anderson@aignacio.com>
 * Date              : 12.03.2022
 * Last Modified Date: 04.05.2022
 */
module mpsoc
  import amba_axi_pkg::*;
  import amba_ahb_pkg::*;
  import ravenoc_pkg::*;
  import eth_pkg::*;
(
  input               clk_in,
  input               rst_clk,       // Active high
  input               rst_cpu,       // Active high
  input               uart_switch_i, // 0 - Slave tiles, 1 - Master tile
  input               bootloader_i,  // Active high
  output  logic       uart_tx_o,
  input               uart_rx_i,
  // Ethernet: 1000BASE-T RGMII
  input               phy_rx_clk,
  input   [3:0]       phy_rxd,
  input               phy_rx_ctl,
  output              phy_tx_clk,
  output  [3:0]       phy_txd,
  output              phy_tx_ctl,
  output              phy_reset_n,
  input               phy_int_n,
  input               phy_pme_n
);
  s_axi_mosi_t  [`NUM_TILES-1:0] slaves_axi_mosi, ravenoc_mosi;
  s_axi_miso_t  [`NUM_TILES-1:0] slaves_axi_miso, ravenoc_miso;
  s_irq_ni_t    [`NUM_TILES-1:0] irqs_ravenoc;

  logic                  start_fetch;
  logic                  clk;
  logic                  rst;
  logic [`NUM_TILES-1:0] uart_tx;
  logic [`NUM_TILES-1:0] uart_rx;
  logic                  master_bootloader;
  logic                  slave_bootloader;

  logic                  rst_clk_int;
  logic                  uart_switch_int;
  logic                  bootloader_int;

`ifdef SIMULATION
  assign  clk         = clk_in;
  assign  start_fetch = 'b1;
  assign  rst         = rst_cpu;

  assign  uart_switch_int = uart_switch_i;
  assign  bootloader_int  = bootloader_i;
`else
  assign  rst             = rst_cpu;
  assign  rst_clk_int     = rst_clk;
  assign  uart_switch_int = uart_switch_i;
  assign  bootloader_int  = bootloader_i;

  logic   clk_feedback_in;
  logic   clk_feedback_out;
  logic   clk_buff_in;

  IBUF clk_in_ibufg(
    .I (clk_in),
    .O (clk_in_buff)
  );

  BUFG clk_feedback_buf(
    .I (clk_feedback_in),
    .O (clk_feedback_out)
  );

  BUFG clk_out_buf(
    .I (clk_buff_in),
    .O (clk)
  );

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
    .CLKIN1_PERIOD       (10.000)
  ) plle2_adv_100_to_50MHz_inst (
    .CLKFBOUT            (clk_feedback_in),
    .CLKOUT0             (clk_buff_in),
    .CLKOUT1             (),
    .CLKOUT2             (),
    .CLKOUT3             (),
    .CLKOUT4             (),
    .CLKOUT5             (),
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
    .DO                  (),
    .DRDY                (),
    .DWE                 (1'b0),
    // Other control and status signals
    .LOCKED              (),
    .PWRDWN              (1'b0),
    .RST                 (rst_clk)
  );
`endif

  tile_0 u_tile_0 (
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[0]),
    .noc_axi_miso_i (slaves_axi_miso[0]),
    .uart_rx_i      (uart_rx[0]),
    .bootloader_i   (master_bootloader),
    .phy_rx_clk     (phy_rx_clk),
    .phy_rxd        (phy_rxd),
    .phy_rx_ctl     (phy_rx_ctl),
    .phy_int_n      (phy_int_n),
    .phy_pme_n      (phy_pme_n),
    .clk_in_100MHz  (clk_in),
    .noc_axi_mosi_o (slaves_axi_mosi[0]),
    .uart_tx_o      (uart_tx[0]),
    .phy_tx_clk     (phy_tx_clk),
    .phy_txd        (phy_txd),
    .phy_tx_ctl     (phy_tx_ctl),
    .phy_reset_n    (phy_reset_n)
  ); 

  tile_1 u_tile_1(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[1]),
    .noc_axi_miso_i (slaves_axi_miso[1]),
    .uart_rx_i      (uart_rx[1]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[1]),
    .uart_tx_o      (uart_tx[1])
  );

  tile_2 u_tile_2(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[2]),
    .noc_axi_miso_i (slaves_axi_miso[2]),
    .uart_rx_i      (uart_rx[2]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[2]),
    .uart_tx_o      (uart_tx[2])
  );

  tile_3 u_tile_3(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[3]),
    .noc_axi_miso_i (slaves_axi_miso[3]),
    .uart_rx_i      (uart_rx[3]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[3]),
    .uart_tx_o      (uart_tx[3])
  );

  tile_4 u_tile_4(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[4]),
    .noc_axi_miso_i (slaves_axi_miso[4]),
    .uart_rx_i      (uart_rx[4]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[4]),
    .uart_tx_o      (uart_tx[4])
  );

  tile_5 u_tile_5(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[5]),
    .noc_axi_miso_i (slaves_axi_miso[5]),
    .uart_rx_i      (uart_rx[5]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[5]),
    .uart_tx_o      (uart_tx[5])
  );

  tile_6 u_tile_6(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[6]),
    .noc_axi_miso_i (slaves_axi_miso[6]),
    .uart_rx_i      (uart_rx[6]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[6]),
    .uart_tx_o      (uart_tx[6])
  );

  tile_7 u_tile_7(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[7]),
    .noc_axi_miso_i (slaves_axi_miso[7]),
    .uart_rx_i      (uart_rx[7]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[7]),
    .uart_tx_o      (uart_tx[7])
  );

  tile_8 u_tile_8(
    .clk_in         (clk),
    .rst_in         (rst),
    .irq_ravenoc    (irqs_ravenoc[8]),
    .noc_axi_miso_i (slaves_axi_miso[8]),
    .uart_rx_i      (uart_rx[8]),
    .bootloader_i   (slave_bootloader),
    .noc_axi_mosi_o (slaves_axi_mosi[8]),
    .uart_tx_o      (uart_tx[8])
  );

  always_comb begin
    ravenoc_mosi    = slaves_axi_mosi;
    slaves_axi_miso = ravenoc_miso;

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

    // NoC only accepts INCR type of burst
    for (int i=0;i<`NUM_TILES;i++) begin
      ravenoc_mosi[i].arburst = AXI_INCR;
      ravenoc_mosi[i].awburst = AXI_INCR;
      uart_rx[i] = uart_rx_i;
    end
  end

  ravenoc #(
    .AXI_CDC_REQ      ('0)
  ) u_noc (
    .clk_axi          ({`NUM_TILES{clk}}),
    .clk_noc          (clk),
    .arst_axi         ({`NUM_TILES{rst}}),
    .arst_noc         (rst),
    // NI interfaces
    .axi_mosi_if      (ravenoc_mosi),
    .axi_miso_if      (ravenoc_miso),
    // IRQs
    .irqs             (irqs_ravenoc),
    // Used only in tb to bypass cdc module
    .bypass_cdc       ('0)
  );
endmodule

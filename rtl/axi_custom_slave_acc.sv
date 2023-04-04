/**
 * File              : axi_custom_slave_acc.sv
 * License           : MIT license <Check LICENSE>
 * Author            : Anderson Ignacio da Silva (aignacio) <anderson@aignacio.com>
 * Date              : 02.04.2023
 * Last Modified Date: 02.04.2023
 */
module axi_custom_slave_acc
  import amba_axi_pkg::*;
#(
  parameter int BASE_ADDR = 'h0
)(
  input                 clk,
  input                 rst,
  input   s_axi_mosi_t  axi_mosi,
  output  s_axi_miso_t  axi_miso
);
  assign axi_miso = s_axi_miso_t'('0);
endmodule

/**
 * File              : axi_custom_master_acc.sv
 * License           : MIT license <Check LICENSE>
 * Author            : Anderson Ignacio da Silva (aignacio) <anderson@aignacio.com>
 * Date              : 02.04.2023
 * Last Modified Date: 02.04.2023
 */
module axi_custom_master_acc
  import amba_axi_pkg::*;
(
  input                 clk,
  input                 rst,
  output  s_axi_mosi_t  axi_mosi,
  input   s_axi_miso_t  axi_miso
);
  assign axi_mosi = s_axi_mosi_t'('0);
endmodule

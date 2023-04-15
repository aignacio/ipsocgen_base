## Configuration options, can be used for all designs
set_property CONFIG_VOLTAGE 3.3 [current_design]
set_property CFGBVS VCCO [current_design]

## Clock Signal
set_property -dict { PACKAGE_PIN R4   IOSTANDARD LVCMOS33 } [get_ports { arty_a7_100MHz }]; #IO_L13P_T2_MRCC_34 Sch=sysclk
create_clock -add -name sys_clk_pin -period 10.00 -waveform {0 5} [get_ports arty_a7_100MHz]

#set_property -dict { PACKAGE_PIN G4  IOSTANDARD LVCMOS15 } [get_ports { arty_a7_rst }]; #IO_L12N_T1_MRCC_35 Sch=cpu_resetn
set_property -dict { PACKAGE_PIN C22 IOSTANDARD LVCMOS12 } [get_ports { arty_a7_rst }]; #IO_L20P_T3_16 Sch=btnl
set_property -dict { PACKAGE_PIN B22 IOSTANDARD LVCMOS12 } [get_ports { arty_a7_pll_rst }]; #IO_L20N_T3_16 Sch=btnc

set_property -dict { PACKAGE_PIN AA19  IOSTANDARD LVCMOS33 } [get_ports { uart_tx_o }]; #IO_L15P_T2_DQS_RDWR_B_14 Sch=uart_rx_out
set_property -dict { PACKAGE_PIN V18   IOSTANDARD LVCMOS33 } [get_ports { uart_rx_i }]; #IO_L14P_T2_SRCC_14 Sch=uart_tx_in

set_property -dict { PACKAGE_PIN D22 IOSTANDARD LVCMOS12 } [get_ports { bootloader_i }]; #IO_L22N_T3_16 Sch=btnd

set_property -dict { PACKAGE_PIN E22  IOSTANDARD LVCMOS12 } [get_ports { uart_switch_i }]; #IO_L22P_T3_16 Sch=sw[0]

# Gigabit Ethernet RGMII PHY
set_property -dict {LOC V13 IOSTANDARD LVCMOS25} [get_ports phy_rx_clk]
set_property -dict {LOC AB16 IOSTANDARD LVCMOS25} [get_ports {phy_rxd[0]}]
set_property -dict {LOC AA15 IOSTANDARD LVCMOS25} [get_ports {phy_rxd[1]}]
set_property -dict {LOC AB15 IOSTANDARD LVCMOS25} [get_ports {phy_rxd[2]}]
set_property -dict {LOC AB11 IOSTANDARD LVCMOS25} [get_ports {phy_rxd[3]}]
set_property -dict {LOC W10 IOSTANDARD LVCMOS25} [get_ports phy_rx_ctl]
set_property -dict {LOC AA14 IOSTANDARD LVCMOS25 SLEW FAST DRIVE 16} [get_ports phy_tx_clk]
set_property -dict {LOC Y12 IOSTANDARD LVCMOS25 SLEW FAST DRIVE 16} [get_ports {phy_txd[0]}]
set_property -dict {LOC W12 IOSTANDARD LVCMOS25 SLEW FAST DRIVE 16} [get_ports {phy_txd[1]}]
set_property -dict {LOC W11 IOSTANDARD LVCMOS25 SLEW FAST DRIVE 16} [get_ports {phy_txd[2]}]
set_property -dict {LOC Y11 IOSTANDARD LVCMOS25 SLEW FAST DRIVE 16} [get_ports {phy_txd[3]}]
set_property -dict {LOC V10 IOSTANDARD LVCMOS25 SLEW FAST DRIVE 16} [get_ports phy_tx_ctl]
set_property -dict {LOC U7 IOSTANDARD LVCMOS33 SLEW SLOW DRIVE 12} [get_ports phy_reset_n]
set_property -dict {LOC Y14 IOSTANDARD LVCMOS25} [get_ports phy_int_n]
set_property -dict {LOC W14 IOSTANDARD LVCMOS25} [get_ports phy_pme_n]

create_clock -period 8.000 -name phy_rx_clk [get_ports phy_rx_clk]

set_false_path -to [get_ports {phy_reset_n}]
set_output_delay 0 [get_ports {phy_reset_n}]
set_false_path -from [get_ports {phy_int_n phy_pme_n}]
set_input_delay 0 [get_ports {phy_int_n phy_pme_n}]

set_property IDELAY_VALUE 0 [get_cells {u_tile_0/u_eth/phy_rx_ctl_idelay u_tile_0/u_eth/phy_rxd_idelay_*}]

set_clock_groups -asynchronous -group [get_clocks -include_generated_clocks arty_a7_100MHz]
set_clock_groups -asynchronous -group [get_clocks -of_objects [get_pins u_clk_pll/CLKOUT0]] -group [get_clocks -of_objects [get_pins {u_tile_0/u_eth/u_clk_mgmt_eth/clk_mmcm_inst/CLKOUT0}]]
#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_pins u_clk_mgmt/plle2_adv_100_to_50MHz_inst/CLKOUT0]] -group [get_clocks phy_rx_clk]
#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_pins {gen_tiles[0].master.u_master_tile/u_ethernet/u_clk_mgmt_eth/clk_mmcm_inst/CLKOUT0}]] -group [get_clocks phy_rx_clk]
#set_clock_groups -asynchronous -group [get_clocks -of_objects [get_pins {gen_tiles[0].master.u_master_tile/u_ethernet/u_clk_mgmt_eth/clk_mmcm_inst/CLKOUT0}]] -group [get_clocks -of_objects [get_pins {gen_tiles[0].master.u_master_tile/u_ethernet/u_clk_mgmt_eth/clk_mmcm_inst/CLKOUT1}]]

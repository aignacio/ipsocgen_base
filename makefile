PARALLEL_B		:=	24
GTKWAVE_PRE		:=	/Applications/gtkwave.app/Contents/Resources/bin/
# Design files
_SRC_VERILOG 	+=	rtl/mpsoc_defines.sv
_SRC_VERILOG	+=	ips/bus_arch_sv_pkg/amba_ahb_pkg.sv
_SRC_VERILOG	+=	ips/bus_arch_sv_pkg/amba_axi_pkg.sv
_SRC_VERILOG	+=	ips/nox/rtl/inc/nox_pkg.svh
_SRC_VERILOG 	+=	ips/nox/rtl/inc/core_bus_pkg.svh
_SRC_VERILOG 	+=	ips/nox/rtl/inc/riscv_pkg.svh
_SRC_VERILOG 	+=	ips/nox/rtl/inc/nox_utils_pkg.sv
_SRC_VERILOG 	+=	ips/ethernet_axi/rtl/inc/eth_pkg.sv
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_defines.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_structs.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_axi_fnc.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_pkg.sv
_SRC_VERILOG  +=  ips/ravenoc/src/ravenoc.sv
_SRC_VERILOG  +=  ips/ravenoc/src/ravenoc_wrapper.sv
_SRC_VERILOG  +=  ips/ravenoc/src/ni/axi_slave_if.sv
_SRC_VERILOG  +=  ips/ravenoc/src/ni/cdc_pkt.sv
_SRC_VERILOG  +=  ips/ravenoc/src/ni/axi_csr.sv
_SRC_VERILOG  +=  ips/ravenoc/src/ni/pkt_proc.sv
_SRC_VERILOG  +=  ips/ravenoc/src/ni/router_wrapper.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/rr_arbiter.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/input_router.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/input_datapath.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/fifo.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/vc_buffer.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/output_module.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/router_ravenoc.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/input_module.sv
_SRC_VERILOG  +=  ips/ravenoc/src/router/router_if.sv
_SRC_VERILOG 	+=	$(shell find rtl/													 -type f -iname *.sv)
_SRC_VERILOG 	+=	$(shell find ips/nox/rtl/									 -type f -iname *.sv)
_SRC_VERILOG 	+=	$(shell find ips/verilog-axi-aignacio/rtl	 -type f -iname *.v)
_SRC_VERILOG 	+=	$(shell find ips/soc_components 					 -type f -iname *.sv)
_SRC_VERILOG 	+=	ips/axi_dma/rtl/inc/dma_pkg.svh
_SRC_VERILOG 	+=	ips/axi_dma/rtl/inc/dma_utils_pkg.sv
_SRC_VERILOG 	+=	ips/axi_dma/csr_out/csr_dma.v
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/rtl							 -type f -iname *.sv)
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/rggen-verilog-rtl -type f -iname *.v)
_SRC_VERILOG 	+=	$(shell find sw/bootloader								 -type f -iname *.sv)
_SRC_VERILOG 	+=	ips/wbuart32/rtl/axiluart.v
_SRC_VERILOG 	+=	ips/wbuart32/rtl/rxuart.v
_SRC_VERILOG 	+=	ips/wbuart32/rtl/skidbuffer.v
_SRC_VERILOG 	+=	ips/wbuart32/rtl/txuart.v
_SRC_VERILOG 	+=	ips/wbuart32/rtl/ufifo.v
# Ethernet
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/rtl -type f -iname *.sv)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/rtl -type f -iname *.v)
_SRC_VERILOG 	+=	ips/ethernet_axi/csr_out/eth_csr.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_mac_mii_fifo.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_mac_mii.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_axis_rx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_axis_tx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/udp_complete.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ip_arb_mux.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ip_complete.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/udp.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/udp_ip_rx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/udp_ip_tx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/udp_checksum_gen.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/lib/axis/rtl/axis_async_fifo_adapter.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/lib/axis/rtl/axis_fifo.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/lib/axis/rtl/axis_async_fifo.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_arb_mux.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/mii_phy_if.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/axis_gmii_rx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/axis_gmii_tx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_mac_1g.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ip.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ip_eth_tx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ip_eth_rx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/arp.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/arp_cache.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/arp_eth_rx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/arp_eth_tx.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ssio_sdr_in.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/lfsr.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_mac_1g_rgmii_fifo.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_mac_1g_rgmii.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/rgmii_phy_if.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ssio_ddr_in.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/iddr.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/oddr.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_mac_1g_gmii_fifo.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/eth_mac_1g_gmii.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/gmii_phy_if.v
_SRC_VERILOG	+=  ips/ethernet_axi/verilog-ethernet/rtl/ssio_sdr_out.v
#_SRC_VERILOG 	+=	sw/nootloader/output/boot_rom.sv
SRC_VERILOG 	:=	$(_SRC_VERILOG)

# Design include files
_INCS_VLOG		+=	ips/bus_arch_sv_pkg
_INCS_VLOG		+=	ips/nox/rtl/inc
_INCS_VLOG		+=	ips/ravenoc/src/include
_INCS_VLOG		+=	ips/axi_dma/rtl/inc
_INCS_VLOG		+=	ips/axi_dma/rggen-verilog-rtl/
_INCS_VLOG		+=	ips/ethernet_axi/rggen-verilog-rtl
_INCS_VLOG		+=	ips/ethernet_axi/rtl/inc
_INCS_VLOG		+=	rtl
INCS_VLOG			:=	$(addprefix -I,$(_INCS_VLOG))

# Parameters of simulation
MASTER_IRAM_KB_SIZE	?=	30
MASTER_DRAM_KB_SIZE	?=	16
MASTER_IRAM_ADDR		?=	0x00040000
MASTER_DRAM_ADDR		?=	0x00020000

SLAVE_IRAM_KB_SIZE	?=	20
SLAVE_DRAM_KB_SIZE	?=	16
SLAVE_IRAM_ADDR			?=	0x00040000
SLAVE_DRAM_ADDR			?=	0x00020000

WAVEFORM_USE	?=	1 # Use 0 to not generate waves [compliance test]

# Verilator info
VERILATOR_TB	:=	tb
WAVEFORM_FST	:=	mpsoc_waves.fst
OUT_VERILATOR	:=	output_verilator
ROOT_MOD_VERI	:=	mpsoc
VERILATOR_EXE	:=	$(OUT_VERILATOR)/$(ROOT_MOD_VERI)
VERI_EXE_SOC	:=	$(OUT_VERILATOR)/$(ROOT_MOD_SOC)

# Testbench files
SRC_CPP				:=	$(wildcard $(VERILATOR_TB)/cpp/testbench.cpp)
_INC_CPPS			:=	../tb/cpp/elfio
_INC_CPPS			+=	../tb/cpp/inc
INCS_CPP			:=	$(addprefix -I,$(_INC_CPPS))

# Verilog Macros
#_MACROS_VLOG	?=	MASTER_IRAM_KB_SIZE=$(IRAM_KB_SIZE)
#_MACROS_VLOG	+=	MASTER_DRAM_KB_SIZE=$(MASTER_DRAM_KB_SIZE)
#_MACROS_VLOG	?=	SLAVE_IRAM_KB_SIZE=$(SLAVE_IRAM_KB_SIZE)
#_MACROS_VLOG	+=	SLAVE_DRAM_KB_SIZE=$(SLAVE_DRAM_KB_SIZE)
_MACROS_VLOG	+=  RGGEN_NAIVE_MUX_IMPLEMENTATION
_MACROS_VLOG	+=	SIMULATION
MACROS_VLOG		?=	$(addprefix +define+,$(_MACROS_VLOG))

# Be sure to set up correctly the number of
# resources like memory/cpu for docker to run
# in case you don't, docker will killed
# the container and you'll not be able to build
# the executable nox_sim
RUN_CMD				:=	docker run --rm --name ship_mpsoc_eth	\
									-v $(abspath .):/mpsoc -w							\
									/mpsoc aignacio/mpsoc

#RUN_SW				:=	sw/slave_tile/output/slave_tile.elf

CPPFLAGS_VERI	:=	"$(INCS_CPP) -O0 -g3 -Wall												\
									-DMASTER_IRAM_KB_SIZE=\"$(MASTER_IRAM_KB_SIZE)\"	\
									-DMASTER_DRAM_KB_SIZE=\"$(MASTER_DRAM_KB_SIZE)\"	\
									-DSLAVE_IRAM_KB_SIZE=\"$(SLAVE_IRAM_KB_SIZE)\"		\
									-DSLAVE_DRAM_KB_SIZE=\"$(SLAVE_DRAM_KB_SIZE)\"		\
									-DMASTER_IRAM_ADDR=\"$(MASTER_IRAM_ADDR)\"				\
									-DMASTER_DRAM_ADDR=\"$(MASTER_DRAM_ADDR)\"				\
									-DSLAVE_IRAM_ADDR=\"$(SLAVE_IRAM_ADDR)\"					\
									-DSLAVE_DRAM_ADDR=\"$(SLAVE_DRAM_ADDR)\"					\
									-DWAVEFORM_USE=\"$(WAVEFORM_USE)\"								\
									-DWAVEFORM_FST=\"$(WAVEFORM_FST)\""
									#-Wunknown-warning-option"

VERIL_ARGS		:=	-j $(PARALLEL_B)							\
									--timescale 1ns/1ps						\
									-CFLAGS $(CPPFLAGS_VERI) 			\
									--top-module $(ROOT_MOD_VERI)	\
									--Mdir $(OUT_VERILATOR)				\
									--threads $(PARALLEL_B)				\
									-f verilator.flags			  		\
									$(INCS_VLOG)									\
									$(MACROS_VLOG)							 	\
									$(SRC_VERILOG) 								\
									$(SRC_CPP) 										\
									-o 														\
									$(ROOT_MOD_VERI)

.PHONY: verilator clean help
help:
	@echo "Targets:"
	@echo "lint	- calls verilator sv linting"
	@echo "all	- build design and sim through verilator"
	@echo "run	- run sim with sw"
	@echo "wave	- calls gtkwave for nox_sim sims"
	@echo "fpga	- Build/Program FPGA"

wave: $(WAVEFORM_FST)
	$(GTKWAVE_PRE)gtkwave $(WAVEFORM_FST) pixel.gtkw

lint: $(SRC_VERILOG) $(SRC_CPP) $(TB_VERILATOR)
	$(RUN_CMD) verilator --lint-only $(VERIL_ARGS)

clean:
	$(RUN_CMD) rm -rf $(OUT_VERILATOR)

all: clean sw/bootloader/output/boot_rom.sv $(VERILATOR_EXE)
	@echo "\n"
	@echo "Design build done, run as follow:"
	@echo "$(VERILATOR_EXE) -h"
	@echo "\n"

sw/bootloader/output/boot_rom.sv:
	make -C sw/bootloader all

run_test:
	$(RUN_CMD) ./$(VERILATOR_EXE) -s 1000000 -em master_tile.elf -es slave_tile.elf

run_bootloader:
	$(RUN_CMD) ./$(VERILATOR_EXE) -s 1000000

$(VERILATOR_EXE): $(OUT_VERILATOR)/V$(ROOT_MOD_VERI).mk
	$(RUN_CMD) make -C $(OUT_VERILATOR)	\
		VM_PARALLEL_BUILDS=1							\
		-f V$(ROOT_MOD_VERI).mk

#$(RUN_SW):
	#make -C sw/test_noc all

#run: $(RUN_SW)
#	$(RUN_CMD) ./$(VERILATOR_EXE) -s 1000000 -e $<

$(OUT_VERILATOR)/V$(ROOT_MOD_VERI).mk: $(SRC_VERILOG) $(SRC_CPP) $(TB_VERILATOR)
	$(RUN_CMD) verilator $(VERIL_ARGS)

fpga_kintex:
	fusesoc run --target=k7_synth --run core:mpsoc:v1.0.0

fpga_arty:
	fusesoc run --target=a7_synth --run core:mpsoc:v1.0.0

fpga_nexys:
	fusesoc run --target=nv_synth --run core:mpsoc:v1.0.0

add_lib:
	fusesoc library add core:mpsoc:v1.0.0 .

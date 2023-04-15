PARALLEL_B		:=	4
GTKWAVE_PRE		:=	/Applications/gtkwave.app/Contents/Resources/bin/
# Add your project name here - Be careful to not add spaces after the name
PROJECT_NAME  :=	mpsoc
WAVEFORM_USE	:=	1

# Common IP Design files
# 1) Packages
_SRC_VERILOG	 =	rtl/$(PROJECT_NAME)_defines.sv
_SRC_VERILOG	+=	ips/bus_arch_sv_pkg/amba_ahb_pkg.sv
_SRC_VERILOG	+=	ips/bus_arch_sv_pkg/amba_axi_pkg.sv
_SRC_VERILOG 	+=	ips/nox/rtl/inc/core_bus_pkg.svh
_SRC_VERILOG 	+=	ips/nox/rtl/inc/nox_utils_pkg.sv
_SRC_VERILOG	+=	ips/nox/rtl/inc/nox_pkg.svh
_SRC_VERILOG 	+=	ips/nox/rtl/inc/riscv_pkg.svh
_SRC_VERILOG 	+=	ips/ethernet_axi/rtl/inc/eth_pkg.sv
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_defines.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_structs.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_axi_fnc.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_pkg.sv
_SRC_VERILOG 	+=	ips/axi_dma/rtl/inc/dma_pkg.svh
_SRC_VERILOG 	+=	ips/axi_dma/rtl/inc/dma_utils_pkg.sv
# 2) RaveNoC files
_SRC_VERILOG 	+=	$(shell find ips/ravenoc/src							 -maxdepth 1 -type f -iname *.sv)
_SRC_VERILOG 	+=	$(shell find ips/ravenoc/src/ni						 -maxdepth 1 -type f -iname *.sv ! -name 'async_gp_fifo.sv')
_SRC_VERILOG 	+=	$(shell find ips/ravenoc/src/router				 -maxdepth 1 -type f -iname *.sv)
# 3) Nox files                                                         
_SRC_VERILOG 	+=	$(shell find ips/nox/rtl									 -maxdepth 1 -type f -iname *.sv)
# 4) Verilog AXI modified                                              
_SRC_VERILOG 	+=	$(shell find ips/verilog-axi-aignacio/rtl	 -maxdepth 1 -type f -iname *.v)
# 5) Different SoC components                                          
_SRC_VERILOG 	+=	$(shell find ips/soc_components 					 -maxdepth 1 -type f -iname *.sv)
# 6) AXI DMA
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/csr_out 					 -maxdepth 1 -type f -iname *.v)
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/rggen-verilog-rtl -maxdepth 1 -type f -iname *.v)
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/rtl							 -maxdepth 1 -type f -iname *.sv)
# 7) Boot ROM
_SRC_VERILOG 	+=	sw/bootloader/output/boot_rom.sv
# 8) AXI UART
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/axiluart.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/rxuart.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/skidbuffer.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/txuart.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/ufifo.v

# 9) Ethernet AXI
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/csr_out	-maxdepth 1 -type f -iname *.v)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/rtl 		-maxdepth 1 -type f -iname *.sv)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/rtl 		-maxdepth 1 -type f -iname *.v)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/verilog-ethernet/lib/axis/rtl	 -maxdepth 1 -type f -iname *.v ! -name 'priority_encoder.v' ! -name 'arbiter.v')
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/verilog-ethernet/rtl					 -maxdepth 1 -type f -iname *.v)
# 10) RTL files from user
_SRC_VERILOG 	+=	$(shell find rtl/	-type f -iname *.sv)
# Merge all files into a single variable
SRC_VERILOG 	:=	$(_SRC_VERILOG)

# Include folders from different design design files
_INCS_VLOG		 =	ips/bus_arch_sv_pkg
_INCS_VLOG		+=	ips/nox/rtl/inc
_INCS_VLOG		+=	ips/ravenoc/src/include
_INCS_VLOG		+=	ips/axi_dma/rtl/inc
_INCS_VLOG		+=	ips/axi_dma/rggen-verilog-rtl/
_INCS_VLOG		+=	ips/ethernet_axi/rggen-verilog-rtl
_INCS_VLOG		+=	ips/ethernet_axi/rtl/inc
_INCS_VLOG		+=	rtl
INCS_VLOG			:=	$(addprefix -I,$(_INCS_VLOG))

# Design parameters
MASTER_IRAM_KB_SIZE	?=	30
MASTER_DRAM_KB_SIZE	?=	16
MASTER_IRAM_ADDR		?=	0x00040000
MASTER_DRAM_ADDR		?=	0x00020000

SLAVE_IRAM_KB_SIZE	?=	20
SLAVE_DRAM_KB_SIZE	?=	16
SLAVE_IRAM_ADDR			?=	0x00040000
SLAVE_DRAM_ADDR			?=	0x00020000

# Verilator info
VERILATOR_TB	:=	tb
WAVEFORM_FST	:=	waves.fst
OUT_VERILATOR	:=	output_verilator
ROOT_MOD_VERI	:=	$(PROJECT_NAME)
VERILATOR_EXE	:=	$(OUT_VERILATOR)/$(ROOT_MOD_VERI)

# Testbench files
SRC_CPP				:=	$(wildcard $(VERILATOR_TB)/cpp/testbench.cpp)
_INC_CPPS			:=	../tb/cpp/elfio
_INC_CPPS			+=	../tb/cpp/inc
INCS_CPP			:=	$(addprefix -I,$(_INC_CPPS))

# Verilog Macros
_MACROS_VLOG	+=  RGGEN_NAIVE_MUX_IMPLEMENTATION
_MACROS_VLOG	+=	SIMULATION
MACROS_VLOG		?=	$(addprefix +define+,$(_MACROS_VLOG))

# Be sure to set up correctly the number of
# resources like memory/cpu for docker to run
# in case you don't, docker will killed
# the container and you'll not be able to build
# the executable nox_sim
RUN_CMD				:=	docker run --rm --name ship_soc	\
									-v $(abspath .):/soc -w					\
									/soc aignacio/mpsoc

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

wave: $(WAVEFORM_FST)
	$(GTKWAVE_PRE)gtkwave $(WAVEFORM_FST) pixel.gtkw

lint: $(SRC_VERILOG) $(SRC_CPP) $(TB_VERILATOR)
	$(RUN_CMD) verilator --lint-only $(VERIL_ARGS)

clean:
	$(RUN_CMD) rm -rf $(OUT_VERILATOR)

$(VERILATOR_EXE): $(OUT_VERILATOR)/V$(ROOT_MOD_VERI).mk
	$(RUN_CMD) make -C $(OUT_VERILATOR)	\
		VM_PARALLEL_BUILDS=1							\
		-f V$(ROOT_MOD_VERI).mk

$(OUT_VERILATOR)/V$(ROOT_MOD_VERI).mk: $(SRC_VERILOG) $(SRC_CPP) $(TB_VERILATOR)
	$(RUN_CMD) verilator $(VERIL_ARGS)

sw/bootloader/output/boot_rom.sv:
	make -C sw/bootloader all

sw/hello_world/output/hello_world.elf:
	make -C sw/hello_world all

all: clean sw/bootloader/output/boot_rom.sv $(VERILATOR_EXE)
	@echo "\n"
	@echo "Design build done, run as follow:"
	@echo "$(VERILATOR_EXE) -h"
	@echo "\n"

run: $(VERILATOR_EXE) 
	$(RUN_CMD) ./$(VERILATOR_EXE) -s 1000000 -em sw/master/build/master_tile.elf -es sw/slave/build/slave_tile.elf

fpga_nexys:
	fusesoc run --target=nv_synth --run core:mpsoc:v1.0.0

add_lib:
	fusesoc library add core:mpsoc:v1.0.0 .

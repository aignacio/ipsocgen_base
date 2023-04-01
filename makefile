PARALLEL_B		:=	4
GTKWAVE_PRE		:=	/Applications/gtkwave.app/Contents/Resources/bin/
PROJECT_NAME  :=	test # Add your project name here
WAVEFORM_USE	:=	1

# Common IP Design files
# 1) Packages
_SRC_VERILOG	 =	ips/bus_arch_sv_pkg/amba_ahb_pkg.sv
_SRC_VERILOG	+=	ips/bus_arch_sv_pkg/amba_axi_pkg.sv
_SRC_VERILOG 	+=	ips/nox/rtl/inc/core_bus_pkg.svh
_SRC_VERILOG 	+=	ips/nox/rtl/inc/nox_utils_pkg.sv
_SRC_VERILOG	+=	ips/nox/rtl/inc/nox_pkg.svh
_SRC_VERILOG 	+=	ips/nox/rtl/inc/riscv_pkg.svh
_SRC_VERILOG 	+=	ips/ethernet_axi/rtl/inc/eth_pkg.svh
_SRC_VERILOG 	+=	ips/ethernet_axi/rtl/inc/utils_pkg.sv
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_defines.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_structs.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_axi_fnc.svh
_SRC_VERILOG 	+=	ips/ravenoc/src/include/ravenoc_pkg.sv
_SRC_VERILOG 	+=	ips/axi_dma/rtl/inc/dma_pkg.svh
_SRC_VERILOG 	+=	ips/axi_dma/rtl/inc/dma_utils_pkg.sv
# 2) RaveNoC files
_SRC_VERILOG 	+=	$(shell find ips/ravenoc/src																-type f -iname *.sv -maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/ravenoc/src/ni															-type f -iname *.sv ! -name 'async_gp_fifo.sv' -maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/ravenoc/src/router													-type f -iname *.sv -maxdepth 1)
# 3) Nox files
_SRC_VERILOG 	+=	$(shell find ips/nox/rtl																		-type f -iname *.sv -maxdepth 1)
# 4) Verilog AXI modified
_SRC_VERILOG 	+=	$(shell find ips/verilog-axi-aignacio/rtl	 									-type f -iname *.v  -maxdepth 1)
# 5) Different SoC components
_SRC_VERILOG 	+=	$(shell find ips/soc_components 					 									-type f -iname *.sv -maxdepth 1)
# 6) AXI DMA
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/csr_out 					 									-type f -iname *.v  -maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/rggen-verilog-rtl 									-type f -iname *.v	-maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/axi_dma/rtl							 									-type f -iname *.sv -maxdepth 1)
# 7) Boot ROM
_SRC_VERILOG 	+=	sw/bootloader/output/boot_rom.sv
# 8) AXI UART
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/axiluart.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/rxuart.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/skidbuffer.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/txuart.v
_SRC_VERILOG 	+=	ips/soc_components/wbuart32/rtl/ufifo.v

# 9) Ethernet AXI
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/csr_out												-type f -iname *.v  -maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/rtl 													-type f -iname *.sv -maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/rtl 													-type f -iname *.v  -maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/verilog-ethernet/lib/axis/rtl	-type f -iname *.v  ! -name 'priority_encoder.v' ! -name 'arbiter.v' -maxdepth 1)
_SRC_VERILOG 	+=	$(shell find ips/ethernet_axi/verilog-ethernet/rtl					-type f -iname *.v  -maxdepth 1)
# 10) RTL files from user
_SRC_VERILOG 	+=	$(shell find rtl/													 -type f -iname *.sv)
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

# Verilator info
VERILATOR_TB	:=	tb
WAVEFORM_FST	:=	waves.fst
OUT_VERILATOR	:=	output_verilator
ROOT_MOD_VERI	:=	$(PROJECT_NAME)
VERILATOR_EXE	:=	$(OUT_VERILATOR)/$(ROOT_MOD_VERI)
VERI_EXE_SOC	:=	$(OUT_VERILATOR)/$(ROOT_MOD_SOC)

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

CPPFLAGS_VERI	:=	"$(INCS_CPP) -O0 -g3 -Wall					\
									-DWAVEFORM_USE=\"$(WAVEFORM_USE)\"	\
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

all: clean sw/bootloader/output/boot_rom.sv $(VERILATOR_EXE)
	@echo "\n"
	@echo "Design build done, run as follow:"
	@echo "$(VERILATOR_EXE) -h"
	@echo "\n"

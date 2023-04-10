CONTAINER	=	docker run --rm --name ship_sw \
						-v $(abspath .):/mpsoc_sw -w	 \
						/mpsoc_sw aignacio/mpsoc_sw

CONTAINER_IT	=	docker run -it --rm --name ship_sw \
								-v $(abspath .):/mpsoc_sw -w			 \
								/mpsoc_sw aignacio/mpsoc_sw


# Gather all the .h/.c files of this project
FILES			= $(shell find . \( -path ./libs/FreeRTOS-Kernel -o -path ./build \) \
						-prune -o -iname "*.c" -o -iname "*.h" -print)

start:
	@echo $(CONTAINER_IT)

pgm_master:
	python3 scripts/bootloader_elf.py --elf build/master_tile.elf --device /dev/ttyUSB0

pgm_slave:
	python3 scripts/bootloader_elf.py --elf build/slave_tile.elf --device /dev/ttyUSB0

run_cstyle:
	$(CONTAINER) cstyle --config .cstyle $(FILES)


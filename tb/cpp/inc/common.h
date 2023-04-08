#include <iostream>
#include <string>
#include <stdint.h>

using namespace std;

typedef enum {
  MASTER,
  SLAVE
} type_t;

typedef struct{
  uint32_t iram_addr;
  uint32_t iram_kb_size;
  uint32_t dram_addr;
  uint32_t dram_kb_size;
  type_t   type;
} s_tile_t;

typedef struct{
  int sim_cycles;
  int waves_dump;
  unsigned long waves_timestamp;
  string waves_path;
  string elf_path;
} s_sim_setup_t;

void show_usage(void) {
  cerr  << "Usage: "
        << "\t-h,--help\tShow this help message\n"
        << "\t-s,--sim\tSimulation cycles\n"
        << "\t-e,--elf\tELF file to be loaded in the SoC\n"
        << "\t-w,--waves_start\tClk cycles to start dumping waves\n"
        << std::endl;
}

void show_summary(s_sim_setup_t *setup) {
  cout  << "=================================================" << std::endl;
  cout  << "Running simulation: \n"
        << "\n\tClock cycles: \t"    << setup->sim_cycles
        << "\n\tWaves enable: \t"    << setup->waves_dump
        << "\n\tEn. waves at: \t"    << setup->waves_timestamp
        << "\n\tWaves path:   \t"    << setup->waves_path
        << "\n\tELF file:     \t"    << setup->elf_path
        << std::endl;
  cout  << "=================================================" << std::endl;
}

void parse_input (int argc, char** argv, s_sim_setup_t *setup){
  if (argc == 1) {
    show_usage();
    exit(EXIT_FAILURE);
  }

  for (int i=1; i < argc; ++i) {
    string arg = argv[i];
    if ((arg == "-h") || (arg == "--help")) {
      show_usage();
      exit(EXIT_SUCCESS);
    } else if ((arg == "-w") || (arg == "--waves_start")) {
      setup->waves_timestamp = atoi(argv[i+1]);
    } else if ((arg == "-s") || (arg == "--sim")) {
      setup->sim_cycles = atoi(argv[i+1]);
    } else if ((arg == "-e") || (arg == "--elf")) {
      setup->elf_path = argv[i+1];
    }
  }
  show_summary(setup);
}



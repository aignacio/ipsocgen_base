#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <signal.h>
#include <cstdlib>
#include <elfio/elfio.hpp>
#include <iomanip>
#include <ctime>
#include <queue>

#include "inc/common.h"
#include "verilated.h"
#include "verilated_vcd_c.h"
#include "verilated_fst_c.h"
#include "Vmpsoc.h"
#include "Vmpsoc__Syms.h"

#define STRINGIZE(x) #x
#define STRINGIZE_VALUE_OF(x) STRINGIZE(x)

using namespace std;
unsigned long tick_counter;
fstream master_tile_log;
fstream slave_tile_log;

template<class module> class testbench {
  VerilatedFstC *trace = new VerilatedFstC;
  bool getDataNextCycle;
  unsigned long start_dumping;

  public:
    module *core = new module;

    testbench() {
      Verilated::traceEverOn(true);
      tick_counter = 0l;
    }

    ~testbench(void) {
      delete core;
      core = NULL;
    }

    virtual void reset(int rst_cyc) {
      //core->uart_rx_i       = 0;
      //core->bootloader_i    = 0;
      core->arty_a7_rst_n    = 1;
      //core->uart_switch_i   = 0;
      //core->phy_rx_clk      = 0;
      //core->phy_rxd         = 0;
      //core->phy_rx_ctl      = 0;
      //core->phy_int_n       = 0;
      //core->phy_pme_n       = 0;

      for (int i=0;i<rst_cyc;i++) {
        core->arty_a7_rst_n = 0;
        this->tick();
      }
      core->arty_a7_rst_n = 1;
      this->tick();
    }

    virtual	void init_dump_setpoint(unsigned long val) {
      start_dumping = val;
    }

    virtual	void opentrace(const char *name) {
      core->trace(trace, 99);
      trace->open(name);
    }

    virtual void close(void) {
      if (trace) {
        trace->close();
        trace = NULL;
      }
    }

    virtual void tick(void) {
      char master, slave;
      //core->uart_rx_i = rand()%10;
      if (core->mpsoc->u_tile_0->u_rst_ctrl->printfbufferReq()) {
        master = core->mpsoc->u_tile_0->u_rst_ctrl->getbufferReq();
        master_tile_log << master;
      }

      if (core->mpsoc->u_tile_1->u_rst_ctrl->printfbufferReq()) {
        slave = core->mpsoc->u_tile_1->u_rst_ctrl->getbufferReq();
        slave_tile_log << slave;
      }

      core->arty_a7_100MHz = 0;
      core->eval();
      tick_counter++;
      if (tick_counter>(start_dumping*2))
        if(trace) trace->dump(tick_counter);

      core->arty_a7_100MHz = 1;
      core->eval();
      tick_counter++;
      if (tick_counter>(start_dumping*2))
        if(trace) trace->dump(tick_counter);
    }

    virtual bool done(void) {
      return (Verilated::gotFinish());
    }
};

bool loadELF(testbench<Vmpsoc> *sim, string program_path, s_tile_t tile, const bool en_print){
  ELFIO::elfio program;

  program.load(program_path);

  if (program.get_class() != ELFIO::ELFCLASS32 ||
    program.get_machine() != 0xf3){
    cout << "\n[ERROR] Error loading ELF file, headers does not match with ELFCLASS32/RISC-V!" << std::endl;
    return 1;
  }

  ELFIO::Elf_Half seg_num = program.segments.size();

  if (en_print){
    cout << "\n[ELF Loader]"  << std::endl;
    cout << "Program path: "  << program_path << std::endl;
    cout << "Number of segments (program headers): " << seg_num << std::endl;
  }

  for (uint8_t i = 0; i<seg_num; i++){
    const ELFIO::segment *p_seg = program.segments[i];
    const ELFIO::Elf64_Addr lma_addr = (uint32_t)p_seg->get_physical_address();
    const ELFIO::Elf64_Addr vma_addr = (uint32_t)p_seg->get_virtual_address();
    const uint32_t mem_size = (uint32_t)p_seg->get_memory_size();
    const uint32_t file_size = (uint32_t)p_seg->get_file_size();

    if (en_print){
      cout << "Segment [" << (uint32_t)i << "] - LMA [" << std::hex << lma_addr << "] VMA [" << std::hex << vma_addr << "]" << std::endl;
      cout << "File size [" << file_size << "] - Memory size [" << std::dec << mem_size << " ~ " << std::dec << mem_size/1024 << " KB]" << std::endl;
    }

    if ((lma_addr >= tile.iram_addr && lma_addr < (tile.iram_addr+(tile.iram_kb_size*1024))) && (file_size > 0x00)){
      int init_addr = (lma_addr-tile.iram_addr);

      if (file_size >= (tile.iram_kb_size*1024)){
        cout << "[ELF Loader] IRAM ERROR:" << std::endl;
        cout << "ELF program: \t" << mem_size/1024 << " KB" << std::endl;
        cout << "Verilator model memory size: \t" << tile.iram_kb_size << " KB" << std::endl;
        cout << "ELF File too big for emulated memory!" << std::endl;
        return 1;
      }
      // IRAM Address
      for (uint32_t p = 0; p < file_size; p+=4){
        uint32_t word_line = ((uint8_t)p_seg->get_data()[p+3]<<24)+((uint8_t)p_seg->get_data()[p+2]<<16)+
                             ((uint8_t)p_seg->get_data()[p+1]<<8)+(uint8_t)p_seg->get_data()[p];
        // If the whole word is zeroed, we don't write as it might overlap other regions
        if (!(word_line == 0x00)) {
          if (tile.type == MASTER){
            sim->core->mpsoc->u_tile_0->writeWordRAM___05Firam((p+init_addr)/4,word_line);
          }
          else{
            sim->core->mpsoc->u_tile_1->writeWordRAM___05Firam((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_2->writeWordRAM___05Firam((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_3->writeWordRAM___05Firam((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_4->writeWordRAM___05Firam((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_5->writeWordRAM___05Firam((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_6->writeWordRAM___05Firam((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_7->writeWordRAM___05Firam((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_8->writeWordRAM___05Firam((p+init_addr)/4,word_line);
          }
        }
      }
    }
    else if ((lma_addr >= tile.dram_addr && lma_addr < (tile.dram_addr+(tile.dram_kb_size*1024))) && (file_size > 0x00)) {
      int init_addr = (lma_addr-tile.dram_addr);

      if (file_size >= (tile.dram_kb_size*1024)){
        cout << "[ELF Loader] DRAM ERROR:" << std::endl;
        cout << "ELF program: \t" << mem_size/1024 << " KB" << std::endl;
        cout << "Verilator model memory size: \t" << tile.dram_kb_size << " KB" << std::endl;
        cout << "ELF File too big for emulated memory!" << std::endl;
        return 1;
      }
      // DRAM Address
      for (uint32_t p = 0; p < file_size; p+=4){
        uint32_t word_line = ((uint8_t)p_seg->get_data()[p+3]<<24)+((uint8_t)p_seg->get_data()[p+2]<<16)+
                             ((uint8_t)p_seg->get_data()[p+1]<<8)+(uint8_t)p_seg->get_data()[p];
        // If the whole word is zeroed, we don't write as it might overlap other regions
        if (!(word_line == 0x00)) {
          if (tile.type == MASTER){
            sim->core->mpsoc->u_tile_0->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
          }
          else {
            sim->core->mpsoc->u_tile_1->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_2->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_3->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_4->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_5->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_6->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_7->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
            sim->core->mpsoc->u_tile_8->writeWordRAM___05Fdram((p+init_addr)/4,word_line);
          }
        }
      }
    }
  }
  if (tile.type == MASTER){
    sim->core->mpsoc->u_tile_0->writeRstAddr___05Frst_ctrl(program.get_entry());
  }
  else{
    sim->core->mpsoc->u_tile_1->writeRstAddr___05Frst_ctrl(program.get_entry());
    sim->core->mpsoc->u_tile_2->writeRstAddr___05Frst_ctrl(program.get_entry());
    sim->core->mpsoc->u_tile_3->writeRstAddr___05Frst_ctrl(program.get_entry());
    sim->core->mpsoc->u_tile_4->writeRstAddr___05Frst_ctrl(program.get_entry());
    sim->core->mpsoc->u_tile_5->writeRstAddr___05Frst_ctrl(program.get_entry());
    sim->core->mpsoc->u_tile_6->writeRstAddr___05Frst_ctrl(program.get_entry());
    sim->core->mpsoc->u_tile_7->writeRstAddr___05Frst_ctrl(program.get_entry());
    sim->core->mpsoc->u_tile_8->writeRstAddr___05Frst_ctrl(program.get_entry());
  }
  cout << std::endl;
  return 0;
}

int main(int argc, char** argv, char** env){
  Verilated::commandArgs(argc, argv);
  auto *dut = new testbench<Vmpsoc>;
  s_tile_t tile;

  s_sim_setup_t setup = {
    .sim_cycles = 1000,
    .waves_dump = WAVEFORM_USE,
    .waves_timestamp = 0,
    .waves_path = STRINGIZE_VALUE_OF(WAVEFORM_FST),
    .elf_master_path = "",
    .elf_slave_path = ""
  };

  cout << "[MPSoC]" << std::endl;

  cout << "Master Tile cfg:" << std::endl;
  cout << "[IRAM] " << STRINGIZE_VALUE_OF(MASTER_IRAM_KB_SIZE) << "KB" << std::endl;
  cout << "[DRAM] " << STRINGIZE_VALUE_OF(MASTER_DRAM_KB_SIZE) << "KB" << std::endl;

  cout << "Slave Tile cfg:" << std::endl;
  cout << "[IRAM] " << STRINGIZE_VALUE_OF(SLAVE_IRAM_KB_SIZE) << "KB" << std::endl;
  cout << "[DRAM] " << STRINGIZE_VALUE_OF(SLAVE_DRAM_KB_SIZE) << "KB" << std::endl;

  parse_input(argc, argv, &setup);

  int sim_cycles_timeout = setup.sim_cycles;

  if (WAVEFORM_USE)
    dut->opentrace(STRINGIZE_VALUE_OF(WAVEFORM_FST));

  dut->init_dump_setpoint(setup.waves_timestamp);

  if (!setup.elf_master_path.empty() && !setup.elf_slave_path.empty()){
    tile.iram_addr    = MASTER_IRAM_ADDR;
    tile.iram_kb_size = MASTER_IRAM_KB_SIZE;
    tile.dram_addr    = MASTER_DRAM_ADDR;
    tile.dram_kb_size = MASTER_DRAM_KB_SIZE;
    tile.type         = MASTER;

    if (loadELF(dut, setup.elf_master_path, tile, true)) {
      cout << "\nError while processing Master ELF file!" << std::endl;
      exit(EXIT_FAILURE);
    }

    tile.iram_addr    = SLAVE_IRAM_ADDR;
    tile.iram_kb_size = SLAVE_IRAM_KB_SIZE;
    tile.dram_addr    = SLAVE_DRAM_ADDR;
    tile.dram_kb_size = SLAVE_DRAM_KB_SIZE;
    tile.type         = SLAVE;

    if (loadELF(dut, setup.elf_slave_path, tile, true)) {
      cout << "\nError while processing Slave ELF file!" << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  master_tile_log.open("master_tile_log.txt", ios::out);
  slave_tile_log.open("slave_tile_log.txt", ios::out);

  if (!master_tile_log || !slave_tile_log) {
    cout << "Log files not created!" << std::endl;
    exit(EXIT_FAILURE);
  }

  dut->reset(2);
  while(!Verilated::gotFinish() && setup.sim_cycles--) {
    dut->tick();
  }

  master_tile_log.close();
  slave_tile_log.close();

  cout << "\n[SIM Summary]" << std::endl;
  cout << "Clk cycles elapsed\t= " << (sim_cycles_timeout-(setup.sim_cycles+1)) << std::endl;
  cout << "Remaining clk cycles\t= " << setup.sim_cycles+1 << std::endl;
  dut->close();
  exit(EXIT_SUCCESS);
}

double sc_time_stamp (){
    return tick_counter;
}
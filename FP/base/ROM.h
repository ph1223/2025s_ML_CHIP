#ifndef ROM_H
#define ROM_H

#include "systemc.h"
#include <iostream>
#include <fstream>
#include "DRAM_PARAM.h"

using namespace std;

SC_MODULE( ROM ) {
    sc_in  < bool >  clk;
    sc_in  < bool >  rst;
    
    // AXI Read Address Channel
    sc_in <int> S_ARID;
    sc_in <sc_lv<32> > S_ARADDR;
    sc_in <sc_lv<8> > S_ARLEN; // Burst length in beats
    sc_in <sc_lv<3> > S_ARSIZE; // 000: BYTE, 001: HWORD, 010: WORD, 011: DWORD, 100: QWORD
    sc_in <sc_lv<2> > S_ARBURST; // 00: FIXED, 01: INCR, 10: WRAP
    sc_in <bool> S_ARVALID;
    sc_out <bool> S_ARREADY;

    // AXI Read Data Channel
    sc_out <int> S_RID;
    sc_out <sc_lv<32> > S_RDATA;
    sc_out <sc_lv<2> > S_RRESP; // 00: OKAY, 01: EXOKAY, 10: SLVERR, 11: DECERR
    sc_out <bool> S_RLAST;
    sc_out <bool> S_RVALID;
    sc_in <bool> S_RREADY;

    void run();
    // vvv Please don't remove these two variables vvv
    string DATA_PATH ;
    string IMAGE_FILE_NAME;     
    // ^^^ Please don't remove these two variables ^^^

    vector<uint32_t> DRAM;

    void load_file_to_dram(const std::string& filename, size_t start_addr) {
        std::ifstream fin(filename.c_str());
        if (!fin.is_open()) {
            std::cerr << "Cannot open " << filename << std::endl;
            return;
        }

        float val;
        size_t addr = start_addr;

        while (fin >> val) {
            if (addr >= DRAM.size()) {
                DRAM.resize(addr + 1); // 自動擴展
            }

            DRAM[addr] = *reinterpret_cast<uint32_t*>(&val);
            ++addr;
        }

        std::cout << "Loaded " << (addr - start_addr) << " data into DRAM at address " << start_addr << std::endl;
    }

    void preload_dram() { 
        DRAM.resize(0x03A69F28 + 1024);
        load_file_to_dram(DATA_PATH + IMAGE_FILE_NAME, IMG_ADDRESS);
        
        load_file_to_dram(DATA_PATH + "conv1_weight.txt", CONV1_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv1_bias.txt",   CONV1_BIAS_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv2_weight.txt", CONV2_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv2_bias.txt",   CONV2_BIAS_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv3_weight.txt", CONV3_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv3_bias.txt",   CONV3_BIAS_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv4_weight.txt", CONV4_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv4_bias.txt",   CONV4_BIAS_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv5_weight.txt", CONV5_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "conv5_bias.txt",   CONV5_BIAS_ADDRESS);
        load_file_to_dram(DATA_PATH + "fc6_weight.txt",   FC6_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "fc6_bias.txt",     FC6_BIAS_ADDRESS);
        load_file_to_dram(DATA_PATH + "fc7_weight.txt",   FC7_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "fc7_bias.txt",     FC7_BIAS_ADDRESS);
        load_file_to_dram(DATA_PATH + "fc8_weight.txt",   FC8_WEIGHT_ADDRESS);
        load_file_to_dram(DATA_PATH + "fc8_bias.txt",     FC8_BIAS_ADDRESS);
    }



    SC_CTOR( ROM )
    {
        DATA_PATH = "./data/";      // Please change this to your own data path

        const char* env_file = getenv("IMAGE_FILE_NAME");
        //IMAGE_FILE_NAME = "cat.txt"; // You can change this to test another image file
        if (env_file != NULL) {
            IMAGE_FILE_NAME = env_file;
        } else {
            IMAGE_FILE_NAME = "cat.txt"; // default fallback
        }

        SC_THREAD( run );
        sensitive << clk.pos() << rst.neg();

        
    }
};

#endif
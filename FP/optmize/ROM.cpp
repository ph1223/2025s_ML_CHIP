#include "ROM.h"
#include <sstream>

void ROM::run()
{
    int state;

    int address;
    int length;
    int size;
    int transmission_count;

    float value;
    bool is_reading_data;

    preload_dram();

    while(rst.read()){
        
        wait();
    }
        
    while(true)
    {
        if(rst.read() == true) {
            S_ARREADY.write(false);

            S_RID.write(0);
            S_RRESP.write(0);
            S_RDATA.write(0);
            S_RVALID.write(false);
            S_RLAST.write(false);

            is_reading_data = false;
            transmission_count = 0;
            size = 0;
        }
        else{
            if(!is_reading_data){
                // prepare to read address
                S_ARREADY.write(true);
                
                if(S_ARVALID.read() == true){
                    address = S_ARADDR.read().to_uint();
                    length = S_ARLEN.read().to_uint() + 1;
                    size = S_ARSIZE.read().to_uint();

                    S_RID.write(S_ARID.read());

                    // Check if the address is within the DRAM size
                    if (address + length > DRAM.size()) {
                        std::cerr << "Error: Address out of bounds. Address: " << address << ", Length: " << length << ", DRAM size: " << DRAM.size() << std::endl;
                        sc_stop();
                    }

                    is_reading_data = true;
                    S_ARREADY.write(false);
                }
            }
            else{
                if(S_ARVALID.read() == true){
                    std::cerr << "Error: ARVALID is still high when reading data." << std::endl;
                    sc_stop();
                }
                else{
                    if(size == 2){
                        if(S_RREADY.read() == true){
                            // Read data from DRAM
                            sc_lv<128> data;
                            S_RVALID.write(true);
                            data.range(31,0) = DRAM[address + transmission_count];
                            data.range(127,32) = 0; 
                            S_RDATA.write(data);

                            // S_RDATA.range(31,0).write(DRAM[address + transmission_count]);
                            // S_RDATA.range(127,32).write(0);

                            transmission_count++;

                            if(address + transmission_count >= address + length){
                                S_RLAST.write(true); // Set LAST if this is the last data

                                wait();
                                is_reading_data = false;
                                S_RVALID.write(false);
                                S_RLAST.write(false);
                                transmission_count = 0; // Reset transmission count for the next read
                            }
                            else{
                                S_RLAST.write(false);
                            }
                        }
                    }
                    else if(size == 4){
                        if(S_RREADY.read() == true){
                            // Read data from DRAM
                            sc_lv<128> data;

                            S_RVALID.write(true);
                            data.range(31,0) = (DRAM[address + transmission_count]);
                            data.range(63,32) = (DRAM[address + transmission_count + 1]);
                            data.range(95,64) = (DRAM[address + transmission_count + 2]);
                            data.range(127,96) = (DRAM[address + transmission_count + 3]);
                            S_RDATA.write(data);

                            transmission_count+= 4;

                            if(address + transmission_count >= address + (length * 4)){
                                S_RLAST.write(true); // Set LAST if this is the last data

                                wait();
                                is_reading_data = false;
                                S_RVALID.write(false);
                                S_RLAST.write(false);
                                transmission_count = 0; // Reset transmission count for the next read
                            }
                            else{
                                S_RLAST.write(false);
                            }
                        }
                    }
                }
            }


        }
        

        wait();
    }
}
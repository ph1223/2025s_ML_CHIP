#include <systemc.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Alexnet.h"

using namespace std;
using namespace sc_core;

int sc_main(int argc, char* argv[]) {
    sc_clock clk("clk", 1, SC_NS);
    sc_buffer<bool> start;
    sc_buffer<bool> done;

    if(argc != 2) {
        cout << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    string input_file = argv[1];

    Alexnet alexnet("alexnet", input_file); 
    
    alexnet.clk(clk);
    alexnet.start(start);
    alexnet.done(done);


    start.write(1);
    sc_start( 2, SC_NS );
    start.write(0);
    

    sc_start( 500, SC_NS );


    return 0;
}
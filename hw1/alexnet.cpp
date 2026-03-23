#include <systemc.h>
#include <iostream>
#include <fstream>
#include <string>
#include "Alexnet.h"

using namespace std;
using namespace sc_core;

int sc_main(int argc, char* argv[]) {
    sc_clock clk("clk", 1, SC_NS);
    sc_signal<bool> reset;

    if(argc != 2) {
        cout << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    string input_file = argv[1];

    Alexnet alexnet("alexnet", input_file); 

    sc_start( 500, SC_NS );


    return 0;
}

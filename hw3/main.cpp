#include "systemc.h"
#include "clockreset.h"
#include "core.h"
#include "pe.h"
#include "router.h"
#include "PARAM.h"
#include <sstream>
#include <iostream>

#define SIM_TIME 35000 

using namespace std;

int sc_main(int argc, char* argv[])
{
    // =======================
    //   signals declaration
    // =======================
    sc_signal < bool > clk;
    sc_signal < bool > rst;

    sc_signal <sc_lv<34> > core_router_flit[4][4][2];
    sc_signal <bool> core_router_req[4][4][2];
    sc_signal <bool> core_router_ack[4][4][2];

    sc_signal <sc_lv<34> > router_router_flit_h[4][4][2];
    sc_signal <bool> router_router_req_h[4][4][2];
    sc_signal <bool> router_router_ack_h[4][4][2];

    sc_signal <sc_lv<34> > router_router_flit_v[4][4][2];
    sc_signal <bool> router_router_req_v[4][4][2];
    sc_signal <bool> router_router_ack_v[4][4][2];

    sc_trace_file *tf = sc_create_vcd_trace_file("wave");
    sc_trace(tf, clk, "clk");
    sc_trace(tf, rst, "rst");


    // =======================
    //   modules declaration
    // =======================
    Clock m_clock("m_clock", 10);
    Reset m_reset("m_reset", 15);

    Core *m_core[4][4];
    Router *m_router[4][4];

    for (int i = 0; i < 4; i++){
        for (int j = 0; j < 4; j++){
            int id = i * 4 + j;
            // Router(sc_module_name name, int id, sc_trace_file *tf = nullptr) : sc_module(name), id(id)
            std::stringstream ss_core, ss_router;

            ss_core << "m_core_" << id;
            m_core[i][j] = new Core(ss_core.str().c_str(), id, tf);

            ss_router << "m_router_" << id;
            m_router[i][j] = new Router(ss_router.str().c_str(), id, tf);
        }
    }
    

    // =======================
    //   modules connection
    // =======================
    m_clock( clk );
    m_reset( rst );

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            // Connect rst and clk
            m_core[i][j]->clk(clk);
            m_core[i][j]->rst(rst);
            m_router[i][j]->clk(clk);
            m_router[i][j]->rst(rst);

            // Connect Core & routers
            m_core[i][j]->flit_rx(core_router_flit[i][j][0]);
            m_core[i][j]->req_rx(core_router_req[i][j][0]);
            m_core[i][j]->ack_rx(core_router_ack[i][j][0]);

            m_core[i][j]->flit_tx(core_router_flit[i][j][1]);
            m_core[i][j]->req_tx(core_router_req[i][j][1]);
            m_core[i][j]->ack_tx(core_router_ack[i][j][1]);

            m_router[i][j]->out_flit[CORE](core_router_flit[i][j][0]);
            m_router[i][j]->out_req[CORE](core_router_req[i][j][0]);
            m_router[i][j]->in_ack[CORE](core_router_ack[i][j][0]);

            m_router[i][j]->in_flit[CORE](core_router_flit[i][j][1]);
            m_router[i][j]->in_req[CORE](core_router_req[i][j][1]);
            m_router[i][j]->out_ack[CORE](core_router_ack[i][j][1]);

            // Connect Routers Horizontally
            m_router[i][j]->out_flit[EAST](router_router_flit_h[i][j][0]);
            m_router[i][j]->out_req[EAST](router_router_req_h[i][j][0]);
            m_router[i][j]->in_ack[EAST](router_router_ack_h[i][j][0]);

            m_router[i][j]->out_flit[WEST](router_router_flit_h[i][j][1]);
            m_router[i][j]->out_req[WEST](router_router_req_h[i][j][1]);
            m_router[i][j]->in_ack[WEST](router_router_ack_h[i][j][1]);

            m_router[i][j]->in_flit[EAST](router_router_flit_h[i][(j + 1) % 4][1]);
            m_router[i][j]->in_req[EAST](router_router_req_h[i][(j + 1) % 4][1]);
            m_router[i][j]->out_ack[EAST](router_router_ack_h[i][(j + 1) % 4][1]);

            m_router[i][j]->in_flit[WEST](router_router_flit_h[i][(j + 3) % 4][0]);
            m_router[i][j]->in_req[WEST](router_router_req_h[i][(j + 3) % 4][0]);
            m_router[i][j]->out_ack[WEST](router_router_ack_h[i][(j + 3) % 4][0]);

            // Connect Routers  Vertically
            m_router[i][j]->out_flit[SOUTH](router_router_flit_v[i][j][0]);
            m_router[i][j]->out_req[SOUTH](router_router_req_v[i][j][0]);
            m_router[i][j]->in_ack[SOUTH](router_router_ack_v[i][j][0]);

            m_router[i][j]->out_flit[NORTH](router_router_flit_v[i][j][1]);
            m_router[i][j]->out_req[NORTH](router_router_req_v[i][j][1]);
            m_router[i][j]->in_ack[NORTH](router_router_ack_v[i][j][1]);

            m_router[i][j]->in_flit[SOUTH](router_router_flit_v[(i + 1) % 4][j][1]);
            m_router[i][j]->in_req[SOUTH](router_router_req_v[(i + 1) % 4][j][1]);
            m_router[i][j]->out_ack[SOUTH](router_router_ack_v[(i + 1) % 4][j][1]);

            m_router[i][j]->in_flit[NORTH](router_router_flit_v[(i + 3) % 4][j][0]);
            m_router[i][j]->in_req[NORTH](router_router_req_v[(i + 3) % 4][j][0]);
            m_router[i][j]->out_ack[NORTH](router_router_ack_v[(i + 3) % 4][j][0]);
        }
    }
    
    sc_start(SIM_TIME, SC_NS);
    sc_close_vcd_trace_file(tf);

    return 0;
}
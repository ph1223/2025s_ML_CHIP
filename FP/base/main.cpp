#include "systemc.h"
#include "clockreset.h"
#include "PARAM.h"
#include "core.h"
#include "router.h"
#include "controller.h"
#include "ROM.h"

using namespace std;
#define SIM_TIME 50000000

int sc_main(int argc, char* argv[])
{
    // =======================
    //   signals declaration
    // =======================
    sc_signal < bool > clk;
    sc_signal < bool > rst;

    // AXI signals
    sc_signal <int> ARID;
    sc_signal <sc_lv<32> > ARADDR;
    sc_signal <sc_lv<8> > ARLEN; // Burst length in beats
    sc_signal <sc_lv<3> > ARSIZE; // 000: BYTE, 001: HWORD, 010: WORD, 011: DWORD, 100: QWORD
    sc_signal <sc_lv<2> > ARBURST; // 00: FIXED, 01: INCR, 10: WRAP
    sc_signal <bool> ARVALID;
    sc_signal <bool> ARREADY;

    sc_signal <int> RID;
    sc_signal <sc_lv<32> > RDATA;
    sc_signal <sc_lv<2> > RRESP; // 00: OKAY, 01: EXOKAY, 10: SLVERR, 11: DECERR
    sc_signal <bool> RLAST;
    sc_signal <bool> RVALID;
    sc_signal <bool> RREADY;

    sc_signal <sc_lv<34> > core_router_flit[4][4][2];
    sc_signal <bool> core_router_req[4][4][2];
    sc_signal <bool> core_router_ack[4][4][2];

    sc_signal <sc_lv<34> > router_router_flit_h[4][4][2];
    sc_signal <bool> router_router_req_h[4][4][2];
    sc_signal <bool> router_router_ack_h[4][4][2];

    sc_signal <sc_lv<34> > router_router_flit_v[4][4][2];
    sc_signal <bool> router_router_req_v[4][4][2];
    sc_signal <bool> router_router_ack_v[4][4][2];

    sc_signal<sc_lv<34> > dummy_flit[2];
    sc_signal<bool> dummy_req[2];
    sc_signal<bool> dummy_ack[2];

    //sc_trace_file *tf = sc_create_vcd_trace_file("wave");
    sc_trace_file *tf = NULL;
    sc_trace(tf, clk, "clk");
    sc_trace(tf, rst, "rst");


    // =======================
    //   modules declaration
    // =======================
    Clock m_clock("m_clock", 10);
    Reset m_reset("m_reset", 15);

    Core *m_core[4][4];
    Router *m_router[4][4];
    Controller *m_controller;
    ROM *m_ROM;

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

    std::stringstream ss_controller, ss_rom;

    ss_controller << "m_controller";
    m_controller = new Controller(ss_controller.str().c_str(), tf);
    ss_rom << "m_ROM";
    m_ROM = new ROM(ss_rom.str().c_str());

    

    // =======================
    //   modules connection
    // =======================
    m_clock( clk );
    m_reset( rst );

    // AXI signals connection
    m_ROM->clk(clk);
    m_ROM->rst(rst);

    m_ROM->S_ARID(ARID);
    m_ROM->S_ARADDR(ARADDR);
    m_ROM->S_ARLEN(ARLEN);
    m_ROM->S_ARSIZE(ARSIZE);
    m_ROM->S_ARBURST(ARBURST);
    m_ROM->S_ARVALID(ARVALID);
    m_ROM->S_ARREADY(ARREADY);

    m_ROM->S_RID(RID);
    m_ROM->S_RDATA(RDATA);
    m_ROM->S_RRESP(RRESP);
    m_ROM->S_RLAST(RLAST);
    m_ROM->S_RVALID(RVALID);
    m_ROM->S_RREADY(RREADY);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            // Connect rst and clk
            m_core[i][j]->clk(clk);
            m_core[i][j]->rst_n(rst);
            m_router[i][j]->clk(clk);
            m_router[i][j]->rst(rst);

            if (i == 0 && j == 0)
            {
                // Core0 is reserved for Controller
                // Connect m_controller to Core
                m_controller->rst(rst);
                m_controller->clk(clk);

                
                // AXI
                m_controller->M_ARID(ARID);
                m_controller->M_ARADDR(ARADDR);
                m_controller->M_ARLEN(ARLEN);
                m_controller->M_ARSIZE(ARSIZE);
                m_controller->M_ARBURST(ARBURST);
                m_controller->M_ARVALID(ARVALID);
                m_controller->M_ARREADY(ARREADY);

                m_controller->M_RID(RID);
                m_controller->M_RDATA(RDATA);
                m_controller->M_RRESP(RRESP);
                m_controller->M_RLAST(RLAST);
                m_controller->M_RVALID(RVALID);
                m_controller->M_RREADY(RREADY);

                // From router0
                m_controller->flit_rx(core_router_flit[0][0][0]);
                m_controller->req_rx(core_router_req[0][0][0]);
                m_controller->ack_rx(core_router_ack[0][0][0]);

                // To router0
                m_controller->flit_tx(core_router_flit[0][0][1]);
                m_controller->req_tx(core_router_req[0][0][1]);
                m_controller->ack_tx(core_router_ack[0][0][1]);

                m_router[0][0]->out_flit[CORE](core_router_flit[i][j][0]);
                m_router[0][0]->out_req[CORE](core_router_req[i][j][0]);
                m_router[0][0]->in_ack[CORE](core_router_ack[i][j][0]);

                m_router[0][0]->in_flit[CORE](core_router_flit[i][j][1]);
                m_router[0][0]->in_req[CORE](core_router_req[i][j][1]);
                m_router[0][0]->out_ack[CORE](core_router_ack[i][j][1]);

                m_core[0][0]->flit_rx(dummy_flit[0]);
                m_core[0][0]->req_rx(dummy_req[0]);
                m_core[0][0]->ack_rx(dummy_ack[0]);

                m_core[0][0]->flit_tx(dummy_flit[1]);
                m_core[0][0]->req_tx(dummy_req[1]);
                m_core[0][0]->ack_tx(dummy_ack[1]);
            }
            else{
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

                
            }
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

    

     // set simulation end time
    sc_start(SIM_TIME, SC_MS);

    sc_close_vcd_trace_file(tf);
    return 0;
}
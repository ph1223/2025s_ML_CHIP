#ifndef ROUTER_H
#define ROUTER_H

#include "systemc.h"
#include "PARAM.h"
#include <sstream>

using namespace std;

SC_MODULE( Router ) {
    sc_in  < bool >  rst;
    sc_in  < bool >  clk;

    sc_out < sc_lv<130> >  out_flit[5];
    sc_out < bool >  out_req[5];
    sc_in  < bool >  in_ack[5];

    sc_in  < sc_lv<130> >  in_flit[5];
    sc_in  < bool >  in_req[5];
    sc_out < bool >  out_ack[5];

    SC_HAS_PROCESS(Router);

    int id;
    sc_trace_file *tf;

    sc_signal<bool> out_buf_busy[5];
    sc_signal<bool> in_buf_busy[5];
    sc_lv<130> out_buf[5];
    sc_lv<130> in_buf[5];
    int flit_destination[5]; // Used to store the  direction of the input buf go to for each input ports
    int flit_sources[5];    // used to store the source of output buf come from for each output ports



    void route_flits() {
        while (true) {
            if(rst.read() == true) {
                for (int i = 0; i < 5; i++) {
                    out_req[i].write(false);
                    out_ack[i].write(false);
                    out_flit[i].write(0);

                    out_buf_busy[i].write(false);
                    in_buf_busy[i].write(false);
                    flit_destination[i] = 0;
                    flit_sources[i] = 0;
                    out_buf[i] = 0;
                    in_buf[i] = 0;
                } 
            } else {
                for(int out_port_dir = 0; out_port_dir < 5; out_port_dir++) {
                    // if output port is busy, something to transfer
                    if (out_buf_busy[out_port_dir] == true)
                    {  
                        // output send request
                        //out_req[out_port_dir].write(true);
  
                        int current_flit_source = flit_sources[out_port_dir];
                        //out_flit[out_port_dir].write(out_buf[out_port_dir]);
 
                        // display flit sources for current router and its corresponding output port
                        //  router id info
                        //  cout << "==================Router id: " << id << endl;
                        //  cout << "Flit sources for current router: " << current_flit_source << " Output port: " << out_port_dir << endl;

                        // buffer holds tail and handshaked with output buffer
                        if ((out_buf[out_port_dir][128] == 1) && in_ack[out_port_dir].read() == true)
                        {
                            // sends data out from the buffer
                            // release the output buffer
                            out_buf_busy[out_port_dir] = false;
                            out_req[out_port_dir].write(false);
                            out_flit[out_port_dir].write(0); 
                            out_ack[current_flit_source].write(false);
                        }
                        else if (in_ack[out_port_dir].read() == true)   
                        {
  
                            // send the flit to the output port, note must delay out_ack by 1 cycle
                            out_buf[out_port_dir] = in_flit[current_flit_source].read();
                            out_flit[out_port_dir].write(out_buf[out_port_dir]);
                            out_ack[current_flit_source].write(true);

                            //cout << "Router " << id << " sent flit " << out_flit << " to port " << out_port_dir << endl;
                            // print current flit source and output dir
                            //cout << "Current flit source: " << current_flit_source << " Output port dir: " << out_port_dir << endl;
                        }
                        else{ 
                            out_ack[current_flit_source].write(false);
                            // out_req[out_port_dir].write(false);
                        }
                    }
                    else // find new packet
                    {
                        for (int in_buf_dir = 0; in_buf_dir < 5; in_buf_dir++) 
                        {   
                            //cout << "Check router " << id << " in_req " << in_req[in_buf_dir].read()  << endl;
                            // check if there is packet to read
                            if (in_req[in_buf_dir].read() == true)
                            {
                                //sc_lv<34> in_buf_flit = in_flit[in_buf_dir].read();

                                // if in_buf_flit is head
                                if (in_flit[in_buf_dir].read()[129] == 1)
                                {
                                    // Extract information
                                    int dst_id = in_flit[in_buf_dir].read().range(123, 120).to_uint(); 

                                    int cur_x = this->id % 4;
                                    int cur_y = this->id / 4;

                                    int dst_x = dst_id % 4;
                                    int dst_y = dst_id / 4;

                                    int d_x = std::abs(cur_x - dst_x);
                                    int d_y = std::abs(cur_y - dst_y);

                                    int cur_dir;

                                    // finding the direction
                                    if (cur_x == dst_x && cur_y == dst_y)
                                    {
                                        // destination reached
                                        cur_dir = CORE;
                                    }
                                    else if (d_x != 0) // check if reached
                                    {
                                        // determine going east or west
                                        int cost_east = std::abs((4 + cur_x + 1) % 4 - dst_x);
                                        int cost_west = std::abs((4 + cur_x - 1) % 4 - dst_x);

                                        if (cost_east <= cost_west)
                                        {
                                            cur_dir = EAST;
                                        }
                                        else
                                        {
                                            cur_dir = WEST;
                                        }
                                    }
                                    else
                                    {
                                        // destermine going north or south
                                        int cost_north = std::abs((4 + cur_y - 1) % 4 - dst_y);
                                        int cost_south = std::abs((4 + cur_y + 1) % 4 - dst_y);

                                        if (cost_north <= cost_south)
                                        {
                                            cur_dir = NORTH;
                                        }
                                        else
                                        {
                                            cur_dir = SOUTH;
                                        }
                                    }

                                    // cout << "Router " << id << " current dir " << cur_dir  << endl;

                                    // see if the selected port header matches the port direction
                                    if (cur_dir == out_port_dir && out_buf_busy[out_port_dir] == false)
                                    {
                                        // this shall be happened in the next cycle!
                                        out_buf_busy[out_port_dir] = true;
                                        flit_sources[out_port_dir] = in_buf_dir;
                                        flit_destination[in_buf_dir] = out_port_dir;

                                        
                                        // send the head flit
                                        out_buf[out_port_dir] = in_flit[in_buf_dir].read();
                                        out_req[out_port_dir].write(true);
                                        out_flit[out_port_dir].write(out_buf[out_port_dir]);
                                        //out_ack[in_buf_dir].write(true);

                                        //cout << "Router " << id << " sent flit " << out_buf[out_port_dir] << " to port " << out_port_dir << endl; 
                                        break;
                                    }
                                     
                                }
                            }
                        }
                    }
                }
            }
            wait();
        }
    }
        
    Router(sc_module_name name, int id, sc_trace_file *tf = NULL) : sc_module(name), id(id), tf(tf){
        SC_THREAD(route_flits);
        sensitive << clk.pos();
        dont_initialize(); 

        for (int i = 0; i < 5; i++)
        {
            std::stringstream ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7, ss8;

            ss0 << "R_" << id << ".in_flit_" << i;
            sc_trace(tf, in_flit[i], ss0.str());

            ss1 << "R_" << id << ".in_req_" << i;
            sc_trace(tf, in_req[i], ss1.str());

            ss2 << "R_" << id << ".in_ack_" << i;
            sc_trace(tf, in_ack[i], ss2.str());

            ss3 << "R_" << id << ".out_flit_" << i;
            sc_trace(tf, out_flit[i], ss3.str());

            ss4 << "R_" << id << ".out_req_" << i;
            sc_trace(tf, out_req[i], ss4.str());

            ss5 << "R_" << id << ".out_ack_" << i;
            sc_trace(tf, out_ack[i], ss5.str());

            ss6 << "R_" << id << ".out_busy_" << i;
            sc_trace(tf, out_buf_busy[i], ss6.str());

            ss7 << "R_" << id << ".in_buf_busy_" << i;
            sc_trace(tf, in_buf_busy[i], ss7.str());

            ss8 << "R_" << id << ".out_buf_" << i;
            sc_trace(tf, out_buf[i], ss8.str());
        }
    }
    ~Router() {
        if (tf) {
            sc_close_vcd_trace_file(tf);
        }
    }
};

#endif
#ifndef CORE_H
#define CORE_H

#include "systemc.h"
#include "pe.h"
#include <string>
#include <queue>
#include <sstream>

using namespace std;
sc_lv<32> float_to_sc_lv32(float value);
float sc_lv32_to_float(const sc_lv<32> lv);

SC_MODULE( Core ) {
    sc_in  < bool >  rst;
    sc_in  < bool >  clk;
    // receive
    sc_in  < sc_lv<34> > flit_rx;	// The input channel
    sc_in  < bool > req_rx;	        // The request associated with the input channel
    sc_out < bool > ack_rx;	        // The outgoing ack signal associated with the input channel
    // transmit
    sc_out < sc_lv<34> > flit_tx;	// The output channel
    sc_out < bool > req_tx;	        // The request associated with the output channel
    sc_in  < bool > ack_tx;	        // The outgoing ack signal associated with the output channel

    PE pe;

    SC_HAS_PROCESS(Core);

    int id;
    sc_trace_file *tf;

    Packet *packet_tx, *packet_rx;

    void send_packet(){
        int send_count; 
        int num_packets;
        queue<float> data;
        int src_id, dest_id;
        bool done_flag;

        while(true){
            if(rst.read() == false){ 
                req_tx.write(false);
                flit_tx.write(0);

                send_count = 0;
                packet_tx = NULL;
                done_flag = false;
            }
            else if(packet_tx == NULL){
                packet_tx = pe.get_packet();
                // need checking for null ptr to perform operation on packet_tx since it is a pointer
                if (packet_tx != NULL)
                {   
                    vector<float> datas_f = packet_tx->datas;
                    // copy datas_f into data_q
                    for (int i = 0; i < datas_f.size(); i++)
                    {
                        data.push(datas_f[i]);
                    }
                    num_packets = data.size() + 1; // 1 for additional header
                    src_id = packet_tx->source_id;
                    dest_id = packet_tx->dest_id;
                    done_flag = false;
                    //cout << "Core_" << id << " get packet" << "src_id:" << src_id << ", dest_id:" << dest_id << ", num_packets:" << num_packets << endl;
                } 
                else
                {      
                    vector<float> datas_f;
                    num_packets = 0;
                    src_id = 0;
                    dest_id = 0;
                    req_tx.write(false);
                    flit_tx.write(0);
                    send_count = 0;
                    done_flag = false;
                    packet_tx = NULL;
                }
            }
            else if(send_count < num_packets){
                req_tx.write(true);
   
                if(send_count == 0){
                    sc_lv<34> header;
                    header.range(33,32) = "10";
                    header.range(31,28) = src_id;
                    header.range(27,24) = dest_id;
                    header.range(23,0) = 0;
                    flit_tx.write(header);
                    send_count++;
                }
                else if(ack_tx.read() == true){
                    if(send_count == num_packets - 1){
                        float data_val;
                        data_val = data.front(); 
                        data.pop();
                        sc_lv<34> data_flit;
                        data_flit.range(33,32) = "01";
                        data_flit.range(31,0) = float_to_sc_lv32(data_val);
                        flit_tx.write(data_flit);
                        send_count++;
                    }
                    else{
                        float data_val;
                        data_val = data.front();
                        data.pop();
                        sc_lv<34> data_flit;
                        data_flit.range(33,32) = "00";
                        data_flit.range(31,0) = float_to_sc_lv32(data_val);
                        flit_tx.write(data_flit);
                        send_count++;
                    }
                }

            } 
            else if(ack_tx.read() == true){
                req_tx.write(false);
                flit_tx.write(0);
                //done_flag = true;
                packet_tx = NULL;
                send_count = 0;
                src_id = 0;
                dest_id = 0;
                num_packets = 0;
            }
            wait(); 
        }
    }

    void receive_packet(){
        while(true){
            sc_lv<34> data_flit = flit_rx.read();

            if(rst.read() == false){ 
                ack_rx.write(false);
            }
            // else if (ack_rx.read() == true)
            // { 
            //     ack_rx.write(false);
            // }
            else if(req_rx.read() == true){
                ack_rx.write(true); 
            }
            else{
                ack_rx.write(false); 
            }

            if(req_rx.read() == true ){ // && ack_rx.read() == true
                if(data_flit.range(33,32) == "10"){     // header

                    packet_rx = new Packet;

                    packet_rx->source_id = sc_dt::sc_lv<4>(data_flit.range(31,28)).to_uint();
                    packet_rx->dest_id = sc_dt::sc_lv<4>(data_flit.range(27,24)).to_uint();

                }
                else if(data_flit.range(33,32) == "01"){    // tail
                    float data_val = sc_lv32_to_float(data_flit.range(31,0));
                    packet_rx->datas.push_back(data_val);

                    pe.check_packet(packet_rx);
                }
                else{   // body
                    float data_val = sc_lv32_to_float(data_flit.range(31,0));
                    packet_rx->datas.push_back(data_val);
                }
            }

            wait();
        }
    }

    

    Core(sc_module_name name, int id, sc_trace_file *tf = NULL) : sc_module(name), id(id), tf(tf) {
        pe.init(id);

        SC_THREAD(send_packet);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(receive_packet);
        sensitive << clk.pos();
        dont_initialize();

        std::stringstream ss;

        ss << "core_" << id << ".clk";
        sc_trace(tf, clk, ss.str());

        ss.str(""); ss.clear();  // 清空內容與錯誤旗標
        ss << "core_" << id << ".rst";
        sc_trace(tf, rst, ss.str());

        ss.str(""); ss.clear();
        ss << "core_" << id << ".flit_rx";
        sc_trace(tf, flit_rx, ss.str());

        ss.str(""); ss.clear();
        ss << "core_" << id << ".req_rx";
        sc_trace(tf, req_rx, ss.str());

        ss.str(""); ss.clear();
        ss << "core_" << id << ".ack_rx";
        sc_trace(tf, ack_rx, ss.str());

        ss.str(""); ss.clear();
        ss << "core_" << id << ".flit_tx";
        sc_trace(tf, flit_tx, ss.str());

        ss.str(""); ss.clear();
        ss << "core_" << id << ".req_tx";
        sc_trace(tf, req_tx, ss.str());

        ss.str(""); ss.clear();
        ss << "core_" << id << ".ack_tx";
        sc_trace(tf, ack_tx, ss.str());

    }

    ~Core() {
        if (tf) {
            sc_close_vcd_trace_file(tf);
        }
    }

};

sc_lv<32> float_to_sc_lv32(float value) {
    uint32_t as_int;
    memcpy(&as_int, &value, sizeof(value));
    sc_lv<32> result(as_int);
    return result;
}

float sc_lv32_to_float(const sc_lv<32> lv) {
    uint32_t int_val = lv.to_uint();
    float f;
    memcpy(&f, &int_val, sizeof(int_val));
    return f;
}

#endif
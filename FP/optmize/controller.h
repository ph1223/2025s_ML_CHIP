#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "pe.h"
#include <queue>
#include "systemc.h"
#include "Utils.h"
#include "DRAM_PARAM.h"
using namespace std;

SC_MODULE( Controller ) {
    sc_in  < bool >  rst;
    sc_in  < bool >  clk;
    
    // AXI signals
    sc_out < int > M_ARID; // AXI Read Address ID
    sc_out < sc_lv<32> > M_ARADDR; // AXI Read Address
    sc_out < sc_lv<8> > M_ARLEN; // Burst length in beats
    sc_out < sc_lv<3> > M_ARSIZE; // 000: BYTE, 001: HWORD, 010: WORD 32 bits , 011: DWORD, 100: QWORD 128 bits
    sc_out < sc_lv<2> > M_ARBURST; // 00: FIXED, 01: INCR, 10: WRAP
    sc_out < bool > M_ARVALID; // AXI Read Address Valid
    sc_in  < bool > M_ARREADY; // AXI Read Address Ready

    sc_in  < int > M_RID; // AXI Read Data ID
    sc_in  < sc_lv<128> > M_RDATA; // AXI Read Data
    sc_in  < sc_lv<2> > M_RRESP; // AXI Read Response
    sc_in  < bool > M_RLAST; // AXI Read Last
    sc_in  < bool > M_RVALID; // AXI Read Data Valid
    sc_out < bool > M_RREADY; // AXI Read Data Ready

    
    // to router0
    sc_out < sc_lv<130> > flit_tx;
    sc_out < bool > req_tx;
    sc_in  < bool > ack_tx;

    // from router0
    sc_in  < sc_lv<130> > flit_rx;
    sc_in  < bool > req_rx;
    sc_out < bool > ack_rx;

    // Trace file
    sc_trace_file *tf;


    int pe_cnt_tx, state;
    int layer_id_cnt;
    int dst_id, src_id;

    Packet *packet_rx;
    vector<float> result_q;

    bool tail_received;

    string file_dir;

    int received_data_cnt;
    int expected_data_cnt;
    int received_weight_cnt;
    int received_bias_cnt;
    int expected_weight_cnt;
    int expected_bias_cnt;

    int data_address;

    bool is_reading_data;

    int cycle_cnt;

    void run(){
        while(true){
            if (rst.read()){

                // AXI
                M_ARID.write(0);
                M_ARADDR.write(0);
                M_ARLEN.write(0);
                M_ARSIZE.write(4);
                M_ARBURST.write(1);
                M_ARVALID.write(false);

                M_RREADY.write(0);

            
                flit_tx.write(0);
                req_tx.write(0);
            
                ack_rx.write(0);

                tail_received = false;
                file_dir = "./data/";
            
                // Initial Idle state
                state = 0;
                layer_id_cnt = 0;
                dst_id = 0;
                src_id = 0;
                received_data_cnt = 0;
                expected_data_cnt = 0;
                received_weight_cnt = 0;
                received_bias_cnt = 0;
                expected_weight_cnt = 0;
                expected_bias_cnt = 0;
                data_address = 0;
                is_reading_data = false;
            }
            else{
                switch(state){
                    case 0: {
                        queue<sc_lv<32> > weight_q;
                        queue<sc_lv<32> > bias_q;
                        queue<sc_lv<32> > img_q;

                        int num_of_data = 0;
                        int first_data_receive = 0;

                        // receive the image data
                        if(layer_id_cnt == 0){
                            expected_data_cnt = 150528/4; // 3 * 224 * 224

                            while(received_data_cnt != expected_data_cnt){
                                if(!is_reading_data){
                                    M_ARADDR.write(data_address);
                                    M_ARLEN = (expected_data_cnt - received_data_cnt) / 256 ? 255 : (expected_data_cnt - received_data_cnt) - 1;
                                    M_ARVALID.write(true);

                                    if(M_ARREADY.read() == true && M_ARVALID.read() == true){
                                        is_reading_data = true;
                                        M_ARVALID.write(false);
                                    }
                                }
                                else{   // reading data
                                    M_RREADY.write(true);
                                    if(M_ARSIZE.read() == 2){ // WORD
                                        if(M_RVALID.read() == true){
                                            img_q.push(M_RDATA.read());
                                            received_data_cnt++;
                                            data_address++;
                                            num_of_data++;
                                            // cout << "Received data: " << received_data_cnt << " / " << expected_data_cnt << endl;
                                            // cout << "Data : " << sc_lv32_to_float(img_q.back()) << endl;
                                        }
                                    }
                                    else if(M_ARSIZE.read() == 4){ // DWORD
                                        if(M_RVALID.read() == true){
                                            img_q.push(M_RDATA.read().range(31,0));
                                            img_q.push(M_RDATA.read().range(63,32));   
                                            img_q.push(M_RDATA.read().range(95,64));
                                            img_q.push(M_RDATA.read().range(127,96));
                                            received_data_cnt ++;
                                            data_address += 4;
                                            num_of_data += 4;
                                            // cout << "Received data: " << received_data_cnt << " / " << expected_data_cnt << endl;
                                            // cout << "Data : " << sc_lv32_to_float(img_q.back()) << endl;
                                        }
                                    }

                                    if(M_RLAST.read() == true){
                                        is_reading_data = false;
                                        M_RREADY.write(false);
                                    }
                                }

                                wait();
                            }
                            

                            cout << "Received Imgs with number of " << num_of_data << " data" << endl;
                            layer_id_cnt++;
                        }

                        switch (layer_id_cnt){
                            case (1):
                                expected_weight_cnt = 23232/4; // 64 * 3 * 11 * 11
                                expected_bias_cnt = 64/4; // 64
                                break;
                            case (2):
                                expected_weight_cnt = 307200/4; // 192 * 64 * 5 * 5
                                expected_bias_cnt = 192/4; // 192
                                break;
                            case (3):
                                expected_weight_cnt = 663552/4; // 384 * 192 * 3 * 3
                                expected_bias_cnt = 384/4; // 384
                                break;
                            case (4):
                                expected_weight_cnt = 884736/4; // 256 * 384 * 3 * 3
                                expected_bias_cnt = 256/4; // 256
                                break;
                            case (5):
                                expected_weight_cnt = 589824/4; // 256 * 256 * 3 * 3
                                expected_bias_cnt = 256/4; // 256
                                break;
                            case (6):
                                expected_weight_cnt = 37748736/4; // 4096 * 9216
                                expected_bias_cnt = 4096/4; // 4096
                                break;
                            case (7):
                                expected_weight_cnt = 16777216/4; // 4096 * 4096
                                expected_bias_cnt = 4096/4; // 4096
                                break;
                            case (8):
                                expected_weight_cnt = 4096000/4; // 1000 * 4096
                                expected_bias_cnt = 1000/4; // 1000
                                break;
                            default:
                                cout << "Invalid layer_id_cnt: " << layer_id_cnt << endl;
                                sc_stop();
                                break;
                        }

                        num_of_data = 0;

                        while(received_weight_cnt != expected_weight_cnt){
                            if(!is_reading_data){
                                M_ARADDR.write(data_address);
                                M_ARLEN = (expected_weight_cnt - received_weight_cnt) / 256 ? 255 : (expected_weight_cnt - received_weight_cnt) - 1;
                                M_ARVALID.write(true);

                                //cout <<"Request address: " << hex << data_address << endl;

                                if(M_ARREADY.read() == true && M_ARVALID.read() == true){
                                    is_reading_data = true;
                                    M_ARVALID.write(false);
                                }
                            }
                            else{   // reading data
                                M_RREADY.write(true);

                                if(M_ARSIZE.read() == 2){ // WORD
                                    if(M_RVALID.read() == true){
                                        weight_q.push(M_RDATA.read());
                                        received_weight_cnt++;
                                        data_address++;
                                        num_of_data++;
                                        // cout << "Received data: " << received_weight_cnt << " / " << expected_weight_cnt << endl;
                                        // cout << "Data : " << sc_lv32_to_float(weight_q.back()) << endl;
                                        // if(num_of_data % 100000 == 0){
                                        //     cout << "Received weight with number of " << num_of_data << " data" << " in layer "<< layer_id_cnt << endl;
                                        // }
                                    }
                                }
                                else if(M_ARSIZE.read() == 4){ // DWORD
                                    if(M_RVALID.read() == true){
                                        weight_q.push(M_RDATA.read().range(31,0));
                                        weight_q.push(M_RDATA.read().range(63,32));
                                        weight_q.push(M_RDATA.read().range(95,64));
                                        weight_q.push(M_RDATA.read().range(127,96));
                                        received_weight_cnt ++;
                                        data_address += 4;
                                        num_of_data += 4;
                                        // cout << "Received data: " << received_weight_cnt << " / " << expected_weight_cnt << endl;
                                        // cout << "Data : " << sc_lv32_to_float(weight_q.back()) << endl;
                                    } 
                                }

                                if(M_RLAST.read() == true){
                                    is_reading_data = false;
                                    M_RREADY.write(false);
                                }
                            }

                            wait(); 
                        }

                        cout << "Received weights with number of " << num_of_data << " data" << " in layer "<< layer_id_cnt << endl;

                        received_weight_cnt = 0;
                        num_of_data = 0;
                        
                        while(received_bias_cnt != expected_bias_cnt){
                            if(!is_reading_data){
                                M_ARADDR.write(data_address);
                                M_ARLEN = (expected_bias_cnt - received_bias_cnt) / 256 ? 255 : (expected_bias_cnt - received_bias_cnt) - 1;
                                M_ARVALID.write(true);

                                //cout <<"Request address: " << hex << data_address << endl;

                                if(M_ARREADY.read() == true && M_ARVALID.read() == true){
                                    is_reading_data = true;
                                    M_ARVALID.write(false);
                                }
                            }
                            else{   // reading data
                                M_RREADY.write(true);

                                if(M_ARSIZE.read() == 2){ // WORD
                                    if(M_RVALID.read() == true){
                                        bias_q.push(M_RDATA.read());
                                        received_bias_cnt++;
                                        data_address++;
                                        num_of_data++;
                                        // cout << "Received data: " << received_bias_cnt << " / " << expected_bias_cnt << endl;
                                        // cout << "Data : " << sc_lv32_to_float(bias_q.back()) << endl;
                                    }
                                }
                                else if(M_ARSIZE.read() == 4){ // DWORD
                                    if(M_RVALID.read() == true){
                                        bias_q.push(M_RDATA.read().range(31,0));
                                        bias_q.push(M_RDATA.read().range(63,32));
                                        bias_q.push(M_RDATA.read().range(95,64));
                                        bias_q.push(M_RDATA.read().range(127,96));
                                        received_bias_cnt ++;
                                        data_address += 4;
                                        num_of_data += 4;
                                        // cout << "Received data: " << received_bias_cnt << " / " << expected_bias_cnt << endl;
                                        // cout << "Data : " << sc_lv32_to_float(bias_q.back()) << endl;
                                    }
                                }

                                if(M_RLAST.read() == true){
                                    is_reading_data = false;
                                    M_RREADY.write(false);
                                }
                            }

                            wait();
                        }

                        cout << "Received bias with number of " << num_of_data << " data" << " in layer "<< layer_id_cnt << endl;

                        received_bias_cnt = 0;

                        // layer_id.write(layer_id_cnt);
                        // layer_id_type.write(false);
                        // layer_id_valid.write(true);

                        // data_received = 0;
                        // num_of_data = 0;
                        // first_data_receive = 0;

                        // // wait data valid signal
                        // wait();
                        // layer_id_valid.write(false);

                        // // wait receive data
                        // while(data_valid.read() != true){
                        //     wait();
                        // }

                        // // receive weight
                        // while(data_valid.read() == true){
                        //     float weight_float;
                        //     weight_float = data.read();

                        //     data_received = float_to_sc_lv32(weight_float);

                        //     if(first_data_receive){
                        //         weight_q.push(data_received);
                        //     }

                        //     // if(num_of_data % 100000 == 0){
                        //     //     cout << "Received weight with number of " << num_of_data << " data" << endl;
                        //     // }
                        //     num_of_data++;
                        //     first_data_receive = 1;
                        //     wait();
                        // }

                        // float weight_float;
                        // weight_float = data.read();

                        // data_received = float_to_sc_lv32(weight_float);

                        // if(first_data_receive){
                        //     weight_q.push(data_received);
                        // }

                        // //cout << "Received weights with number of " << num_of_data << " data" << endl;


                        // layer_id.write(layer_id_cnt);
                        // layer_id_type.write(true);
                        // layer_id_valid.write(true);

                        // data_received = 0;
                        // num_of_data = 0;
                        // first_data_receive = 0;


                        // // wait data valid signal
                        // wait();
                        // layer_id_valid.write(false);

                        // // wait receive data
                        // while(data_valid.read() != true){
                        //     wait();
                        // }

                        // // receive data
                        // while(data_valid.read() == true){
                        //     float bias_float;
                        //     bias_float = data.read();

                        //     data_received = float_to_sc_lv32(bias_float);

                        //     if(first_data_receive){
                        //         bias_q.push(data_received);
                        //     }

                        //     num_of_data++;
                        //     first_data_receive = 1;
                        //     wait();
                        // }

                        // float bias_float;
                        // bias_float = data.read();

                        // data_received = float_to_sc_lv32(bias_float);

                        // if(first_data_receive){
                        //     bias_q.push(data_received);
                        // }

                        //cout << "Received bias with number of " << num_of_data << " data" << endl;

                        switch (layer_id_cnt){
                            case (1):
                                dst_id = 1;
                                break;
                            case (2):
                                dst_id = 2;
                                break;
                            case (3):
                                dst_id = 3;
                                break;
                            case (4):
                                dst_id = 4;
                                break;
                            case (5):
                                dst_id = 5;
                                break;
                            case (6):
                                dst_id = 6;
                                break;
                            case (7):
                                dst_id = 7;
                                break;
                            case (8):
                                dst_id = 8;
                                break;
                            default:
                                dst_id = 0;
                                break;
                        }

                        int num_of_pkt = 2;

                        if (layer_id_cnt == 1){
                            num_of_pkt = 3;
                        }

                        cout << "Wait controller send data" << " in layer "<< layer_id_cnt << endl;;

                        for (int send_cnt = 0; send_cnt < num_of_pkt; send_cnt++){
                            // Sending value
                            queue<sc_lv<32> > datas_q;
                            int packet_type;
                            int packet_size;

                            // First send weights then biases
                            if (send_cnt == 0 && num_of_pkt == 2){
                                // 1 is weight
                                packet_type = 1;
                                datas_q = weight_q;
                                packet_size = weight_q.size();
                            }
                            else if (send_cnt == 1  && num_of_pkt == 2){
                                // 0 is bias
                                packet_type = 0;
                                datas_q = bias_q;
                                packet_size = bias_q.size();
                            }
                            else{
                                if (send_cnt == 0){
                                    // 2 is img
                                    packet_type = 2;
                                    datas_q = img_q;
                                    packet_size = img_q.size();

                                }
                                else if (send_cnt == 1){
                                    // 1 is weight
                                    packet_type = 1;
                                    datas_q = weight_q;
                                    packet_size = weight_q.size();
                                }
                                else{
                                    // 0 is bias
                                    packet_type = 0;
                                    datas_q = bias_q;
                                    packet_size = bias_q.size();
                                }
                            }

                            int send_count = 0;

                            

                            while (send_count < packet_size + 1){
                                req_tx.write(true);
                            
                                if(send_count == 0){
                                    sc_lv<130> header;
                                    header.range(129,128) = "10";
                                    header.range(127,124) = src_id;
                                    header.range(123,120) = dst_id;
                                    header.range(119,118) = packet_type;
                                    header.range(117,0) = 0;
                                    flit_tx.write(header);
                                    send_count++;
                                }
                                else if(ack_tx.read() == true){
                                    if(send_count == packet_size - 3){
                                        sc_lv<32> data_val;
                                        data_val = datas_q.front(); 
                                        datas_q.pop();

                                        sc_lv<130> data_flit;
                                        data_flit.range(129,128) = "01";
                                        data_flit.range(127,96) = data_val;
                                        data_val = datas_q.front(); 
                                        datas_q.pop();
                                        data_flit.range(95,64) = data_val;
                                        data_val = datas_q.front();
                                        datas_q.pop();
                                        data_flit.range(63,32) = data_val;
                                        data_val = datas_q.front();
                                        datas_q.pop();
                                        data_flit.range(31,0) = data_val;

                                        flit_tx.write(data_flit);
                                        send_count+= 4;
                                    }
                                    else{
                                        sc_lv<32> data_val;
                                        data_val = datas_q.front();
                                        datas_q.pop();

                                        sc_lv<130> data_flit;
                                        data_flit.range(129,128) = "00";
                                        data_flit.range(127,96) = data_val;
                                        data_val = datas_q.front(); 
                                        datas_q.pop();
                                        data_flit.range(95,64) = data_val;
                                        data_val = datas_q.front();
                                        datas_q.pop();
                                        data_flit.range(63,32) = data_val;
                                        data_val = datas_q.front();
                                        datas_q.pop();
                                        data_flit.range(31,0) = data_val;

                                        flit_tx.write(data_flit);
                                        send_count+= 4;
                                    }
                                }
                                wait();
                            }

                            req_tx.write(0);
                            flit_tx.write(0);
                            wait();
                        }

                        cout << "Send data complete" << " in layer "<< layer_id_cnt << endl;;

                        layer_id_cnt++;

                        if (layer_id_cnt == 9){
                            state = 1;
                            //cout << "Change state to state 1" << endl;
                        }

                        break;
                    }

                    case 1: {
                        sc_lv<130> data_flit = flit_rx.read();

                        tail_received = false;
                        if(rst.read() == true){ 
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
                    
                        if(req_rx.read() == true && ack_rx.read() == true){ // && ack_rx.read() == true
                            if(data_flit.range(129,128) == "10"){     // header
                            
                                packet_rx = new Packet;
                            
                                packet_rx->source_id = sc_dt::sc_lv<4>(data_flit.range(127,124)).to_uint();
                                packet_rx->dest_id = sc_dt::sc_lv<4>(data_flit.range(123,120)).to_uint();
                                packet_rx->data_type = sc_dt::sc_lv<4>(data_flit.range(119,118)).to_uint();
                            }
                            else if(data_flit.range(129,128) == "01"){    // tail
                                float data_val = sc_lv32_to_float(data_flit.range(127,96));
                                packet_rx->datas.push_back(data_val);
                                data_val = sc_lv32_to_float(data_flit.range(95,64));
                                packet_rx->datas.push_back(data_val);
                                data_val = sc_lv32_to_float(data_flit.range(63,32));
                                packet_rx->datas.push_back(data_val);
                                data_val = sc_lv32_to_float(data_flit.range(31,0));
                                packet_rx->datas.push_back(data_val);
                            
                                tail_received = true;
                                state = 2;
                                //cout << "Core_" << id << " receive tails" << endl;
                            }
                            else{   // body
                                float data_val = sc_lv32_to_float(data_flit.range(127,96));
                                packet_rx->datas.push_back(data_val);
                                data_val = sc_lv32_to_float(data_flit.range(95,64));
                                packet_rx->datas.push_back(data_val);
                                data_val = sc_lv32_to_float(data_flit.range(63,32));
                                packet_rx->datas.push_back(data_val);
                                data_val = sc_lv32_to_float(data_flit.range(31,0));
                                packet_rx->datas.push_back(data_val);
                            }
                        }

                        break;
                    }
                    case 2: {
                        result_q = packet_rx->datas;

                        // softmax
                        vec_1d softmax_output = softmax(result_q);

                        // read in labels
                        string file_path = file_dir + "imagenet_classes.txt";
                        ifstream file(file_path.c_str());
                        vector<string> labels;
                        string label;


                        while(getline(file, label)) {
                            labels.push_back(label);
                        }
                    
                        // get top 100 labels
                        vector<pair<string, float> > top_100_pos = sorting_class(softmax_output, labels);
                        vector<pair<string, float> > top_100_val = sorting_class(result_q, labels);
                    
                        // display top 100 labels
                        cout << "Top 100 classes:" << endl;
                        cout << "=================================================" << endl;
                        cout << right << setw(5) << "idx" 
                             << " | " << setw(8) << "val"
                             << " | " << setw(11) << "possibility"
                             << " | " << "class name" << endl;
                        cout << "-------------------------------------------------" << endl;
                    
                    
                        for(int i = 0; i < 100; i++) {
                            file.clear();
                            file.seekg(0, ios::beg);
                            int index = 0;
                            string class_name = top_100_pos[i].first;
                            while(getline(file, label)) {
                                if(label == class_name) {
                                    break;
                                }
                                index++;
                            }
                        
                            cout << right << setw(5) << index
                                 << " | " << setw(8) << fixed << setprecision(2) << top_100_val[i].second
                                 << " | " << setw(11) << fixed << setprecision(2) << top_100_pos[i].second * 100
                                 << " | " << class_name << endl;
                        }
                        cout << "=================================================" << endl;

                        //cout << "Using cycle = " << sc_time_stamp().value() / 10 << std::endl;
                        cout << "Using cycle = " << cycle_cnt << std::endl;

                        sc_stop();
                        break;
                    }
                }

            
            }
            wait();
        }
    
    }

    void cycle() {
        while (true) {
            if (rst.read()) {
                cycle_cnt = 0;
            } else {
                cycle_cnt++;
            }
            
            wait();
        }
    }
    SC_CTOR(Controller);

    Controller(sc_module_name name, sc_trace_file *tf = NULL) : sc_module(name)
    {
        // Constructor
        SC_THREAD(run);
        dont_initialize();
        sensitive << clk.pos();


        SC_THREAD(cycle);
        dont_initialize();
        sensitive << clk.pos();

        // trace signals
        sc_trace(tf, rst, "m_controller.rst");
        sc_trace(tf, clk, "m_controller.clk");

        // sc_trace(tf, layer_id, "m_controller.layer_id");
        // sc_trace(tf, layer_id_type, "m_controller.layer_id_type");
        // sc_trace(tf, layer_id_valid, "m_controller.layer_id_valid");

        // sc_trace(tf, data_received, "m_controller.data_received");
        // sc_trace(tf, data_valid, "m_controller.data_valid");
        // sc_trace(tf, data, "m_controller.data");

        sc_trace(tf, flit_tx, "m_controller.flit_tx");
        sc_trace(tf, req_tx, "m_controller.req_tx");
        sc_trace(tf, ack_tx, "m_controller.ack_tx");

        sc_trace(tf, flit_rx, "m_controller.flit_rx");
        sc_trace(tf, req_rx, "m_controller.req_rx");
        sc_trace(tf, ack_rx, "m_controller.ack_rx");

        sc_trace(tf, M_ARID, "m_controller.ARID");
        sc_trace(tf, M_ARADDR, "m_controller.ARADDR");
        sc_trace(tf, M_ARLEN, "m_controller.ARLEN");
        sc_trace(tf, M_ARSIZE, "m_controller.ARSIZE");
        sc_trace(tf, M_ARBURST, "m_controller.ARBURST");
        sc_trace(tf, M_ARVALID, "m_controller.ARVALID");
        sc_trace(tf, M_ARREADY, "m_controller.ARREADY");

        sc_trace(tf, M_RID, "m_controller.RID");
        sc_trace(tf, M_RDATA, "m_controller.RDATA");
        sc_trace(tf, M_RRESP, "m_controller.RRESP");
        sc_trace(tf, M_RLAST, "m_controller.RLAST");
        sc_trace(tf, M_RVALID, "m_controller.RVALID");
        sc_trace(tf, M_RREADY, "m_controller.RREADY");
    }


};

#endif
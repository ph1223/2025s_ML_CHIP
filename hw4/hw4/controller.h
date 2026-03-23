#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "pe.h"
#include <queue>
#include "systemc.h"
#include "Utils.h"
using namespace std;

SC_MODULE( Controller ) {
    sc_in  < bool >  rst;
    sc_in  < bool >  clk;
    
    // to ROM
    sc_out < int >   layer_id;       // '0' means input data
    sc_out < bool >  layer_id_type;  // '0' means weight, '1' means bias (for layer_id == 0, we don't care this signal)
    sc_out < bool >  layer_id_valid;

    // from ROM
    sc_in  < float > data;
    sc_in  < bool >  data_valid;
    
    // to router0
    sc_out < sc_lv<34> > flit_tx;
    sc_out < bool > req_tx;
    sc_in  < bool > ack_tx;

    // from router0
    sc_in  < sc_lv<34> > flit_rx;
    sc_in  < bool > req_rx;
    sc_out < bool > ack_rx;

    // Trace file
    sc_trace_file *tf;
    sc_signal<sc_lv<32> > data_received;

    int pe_cnt_tx, state;
    int layer_id_cnt;
    int dst_id, src_id;

    Packet *packet_rx;
    vector<float> result_q;

    bool tail_received;

    string file_dir;

    void run(){
        while(true){
            if (rst.read()){
                layer_id.write(0);
                layer_id_type.write(0);
                layer_id_valid.write(0);
            
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
                            layer_id.write(layer_id_cnt);
                            layer_id_type.write(false);
                            layer_id_valid.write(true);


                            // wait data valid signal
                            wait();
                            layer_id_valid.write(false);

                            // wait receive data
                            while(data_valid.read() != true){
                                wait();
                            }

                            // receive data
                            while(data_valid.read() == true){
                                float img_float;
                                img_float = data.read();

                                data_received = float_to_sc_lv32(img_float);
                                
                                if(first_data_receive){
                                    img_q.push(data_received);
                                }
                                

                                num_of_data++;
                                first_data_receive = 1;

                                wait();
                            }

                            float img_float;
                            img_float = data.read();
                            data_received = float_to_sc_lv32(img_float);
                            
                            if(first_data_receive){
                                img_q.push(data_received);
                            }

                            //cout << "Received Imgs with number of " << num_of_data << " data" << endl;
                            layer_id_cnt++;
                        }

                        layer_id.write(layer_id_cnt);
                        layer_id_type.write(false);
                        layer_id_valid.write(true);

                        data_received = 0;
                        num_of_data = 0;
                        first_data_receive = 0;

                        // wait data valid signal
                        wait();
                        layer_id_valid.write(false);

                        // wait receive data
                        while(data_valid.read() != true){
                            wait();
                        }

                        // receive weight
                        while(data_valid.read() == true){
                            float weight_float;
                            weight_float = data.read();

                            data_received = float_to_sc_lv32(weight_float);

                            if(first_data_receive){
                                weight_q.push(data_received);
                            }

                            // if(num_of_data % 100000 == 0){
                            //     cout << "Received weight with number of " << num_of_data << " data" << endl;
                            // }
                            num_of_data++;
                            first_data_receive = 1;
                            wait();
                        }

                        float weight_float;
                        weight_float = data.read();

                        data_received = float_to_sc_lv32(weight_float);

                        if(first_data_receive){
                            weight_q.push(data_received);
                        }

                        //cout << "Received weights with number of " << num_of_data << " data" << endl;


                        layer_id.write(layer_id_cnt);
                        layer_id_type.write(true);
                        layer_id_valid.write(true);

                        data_received = 0;
                        num_of_data = 0;
                        first_data_receive = 0;


                        // wait data valid signal
                        wait();
                        layer_id_valid.write(false);

                        // wait receive data
                        while(data_valid.read() != true){
                            wait();
                        }

                        // receive data
                        while(data_valid.read() == true){
                            float bias_float;
                            bias_float = data.read();

                            data_received = float_to_sc_lv32(bias_float);

                            if(first_data_receive){
                                bias_q.push(data_received);
                            }

                            num_of_data++;
                            first_data_receive = 1;
                            wait();
                        }

                        float bias_float;
                        bias_float = data.read();

                        data_received = float_to_sc_lv32(bias_float);

                        if(first_data_receive){
                            bias_q.push(data_received);
                        }

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

                        //cout << "Wait send data" << endl;

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
                                    sc_lv<34> header;
                                    header.range(33,32) = "10";
                                    header.range(31,28) = src_id;
                                    header.range(27,24) = dst_id;
                                    header.range(23,22) = packet_type;
                                    header.range(21,0) = 0;
                                    flit_tx.write(header);
                                    send_count++;
                                }
                                else if(ack_tx.read() == true){
                                    if(send_count == packet_size){
                                        sc_lv<32> data_val;
                                        data_val = datas_q.front(); 
                                        datas_q.pop();

                                        sc_lv<34> data_flit;
                                        data_flit.range(33,32) = "01";
                                        data_flit.range(31,0) = data_val;
                                        flit_tx.write(data_flit);
                                        send_count++;
                                    }
                                    else{
                                        sc_lv<32> data_val;
                                        data_val = datas_q.front();
                                        datas_q.pop();

                                        sc_lv<34> data_flit;
                                        data_flit.range(33,32) = "00";
                                        data_flit.range(31,0) = data_val;
                                        flit_tx.write(data_flit);
                                        send_count++;
                                    }
                                }
                                wait();
                            }

                            req_tx.write(0);
                            flit_tx.write(0);
                            wait();
                        }

                        //cout << "Send data complete" << endl;

                        layer_id_cnt++;

                        if (layer_id_cnt == 9){
                            state = 1;
                            //cout << "Change state to state 1" << endl;
                        }

                        break;
                    }
                    case 1: {
                        sc_lv<34> data_flit = flit_rx.read();

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
                            if(data_flit.range(33,32) == "10"){     // header
                            
                                packet_rx = new Packet;
                            
                                packet_rx->source_id = sc_dt::sc_lv<4>(data_flit.range(31,28)).to_uint();
                                packet_rx->dest_id = sc_dt::sc_lv<4>(data_flit.range(27,24)).to_uint();
                                packet_rx->data_type = sc_dt::sc_lv<4>(data_flit.range(23,22)).to_uint();
                            }
                            else if(data_flit.range(33,32) == "01"){    // tail
                                float data_val = sc_lv32_to_float(data_flit.range(31,0));
                                packet_rx->datas.push_back(data_val);
                            
                                tail_received = true;
                                state = 2;
                                // cout << "Controller receive tails" << endl;
                            }
                            else{   // body
                                float data_val = sc_lv32_to_float(data_flit.range(31,0));
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

                        sc_stop();
                        break;
                    }
                }

            
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

        // trace signals
        sc_trace(tf, rst, "m_controller.rst");
        sc_trace(tf, clk, "m_controller.clk");

        sc_trace(tf, layer_id, "m_controller.layer_id");
        sc_trace(tf, layer_id_type, "m_controller.layer_id_type");
        sc_trace(tf, layer_id_valid, "m_controller.layer_id_valid");

        sc_trace(tf, data_received, "m_controller.data_received");
        sc_trace(tf, data_valid, "m_controller.data_valid");
        sc_trace(tf, data, "m_controller.data");

        sc_trace(tf, flit_tx, "m_controller.flit_tx");
        sc_trace(tf, req_tx, "m_controller.req_tx");
        sc_trace(tf, ack_tx, "m_controller.ack_tx");

        sc_trace(tf, flit_rx, "m_controller.flit_rx");
        sc_trace(tf, req_rx, "m_controller.req_rx");
        sc_trace(tf, ack_rx, "m_controller.ack_rx");
    }


};

#endif
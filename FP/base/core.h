#ifndef CORE_H
#define CORE_H

#include "systemc.h"
#include "pe.h"
#include <string>
#include <queue>
#include <sstream>
#include "Utils.h"
#include "Conv_functions.h"
#include "Layer_param.h"

using namespace std;


SC_MODULE( Core ) {
    sc_in  < bool >  rst_n;
    sc_in  < bool >  clk;
    // receive
    sc_in  < sc_lv<34> > flit_rx;	// The input channel
    sc_in  < bool > req_rx;	        // The request associated with the input channel
    sc_out < bool > ack_rx;	        // The outgoing ack signal associated with the input channel
    // transmit
    sc_out < sc_lv<34> > flit_tx;	// The output channel
    sc_out < bool > req_tx;	        // The request associated with the output channel
    sc_in  < bool > ack_tx;	        // The outgoing ack signal associated with the output channel

    SC_HAS_PROCESS(Core);

    int id;
    sc_trace_file *tf;

    Packet *packet_tx, *packet_rx;

    vector<float> weights_q;
    vector<float> biases_q;
    vector<float> img_q;
    vector<float> result;

    bool done_processing;
    bool tail_received;

    void send_packet(){
        int send_count; 
        int num_packets;
        queue<float> data_q;
        int src_id, dest_id, data_type;
        bool done_flag;

        while(true){
            if(rst_n.read() == true){ 
                req_tx.write(false);
                flit_tx.write(0);

                send_count = 0;
                packet_tx = NULL;
                done_flag = false;
                done_processing = false;
                tail_received = false;
            }
            else if(packet_tx == NULL && done_processing == true){
                packet_tx = new Packet;
                packet_tx->source_id = id;

                switch(id){
                    case(1):
                        dest_id = 2;
                        break;
                    case(2):
                        dest_id = 3;
                        break;
                    case(3):
                        dest_id = 4;
                        break;
                    case(4):
                        dest_id = 5;
                        break;
                    case(5):
                        dest_id = 6;
                        break;
                    case(6):
                        dest_id = 7;
                        break;
                    case(7):
                        dest_id = 8;
                        break;
                    case(8):
                        dest_id = 0;
                        break;
                    default:
                        dest_id = 1;
                        break;
                }

                packet_tx->dest_id = dest_id;
                packet_tx->datas = result;
                packet_tx->data_type = 2;

                done_processing = false;



                // need checking for null ptr to perform operation on packet_tx since it is a pointer
                if (packet_tx != NULL)
                {   
                    vector<float> datas_f = packet_tx->datas;
                    // copy datas_f into data_q
                    for (int i = 0; i < datas_f.size(); i++)
                    {
                        data_q.push(datas_f[i]);
                    }
                    num_packets = data_q.size() + 1; // 1 for additional header
                    src_id = packet_tx->source_id;
                    dest_id = packet_tx->dest_id;
                    data_type = packet_tx->data_type;
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
                    header.range(23,22) = data_type;
                    header.range(21,0) = 0;
                    flit_tx.write(header);
                    send_count++;
                }
                else if(ack_tx.read() == true && req_tx.read() == true){
                    if(send_count == num_packets - 1){
                        float data_val;
                        data_val = data_q.front(); 
                        data_q.pop();
                        sc_lv<34> data_flit;
                        data_flit.range(33,32) = "01";
                        data_flit.range(31,0) = float_to_sc_lv32(data_val);
                        flit_tx.write(data_flit);
                        send_count++;

                        //cout << "Core_" << id << " Sending tails" << endl;
                    }
                    else{
                        float data_val;
                        data_val = data_q.front();
                        data_q.pop();
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

            tail_received = false;
            if(rst_n.read() == true){ 
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
                    //cout << "Core_" << id << " receive tails" << endl;
                }
                else{   // body
                    float data_val = sc_lv32_to_float(data_flit.range(31,0));
                    packet_rx->datas.push_back(data_val);
                }
            }

            

            wait();
        }
    }

    void calculate(){
        while(true){
            if(tail_received == true && packet_rx != NULL){
                int data_type = packet_rx->data_type;
                //cout << "Core_id: " << id << " Receiving " << "data: " << data_type << endl;

                switch(data_type){
                    case 2: // image
                        img_q = packet_rx->datas;
                        //cout << "Core_id:" << id << " Receiving " << "image data" << endl;
                        break;
                    case 1: // weights
                        weights_q = packet_rx->datas;
                        //cout << "Core_id:" << id << " Receiving " << "weights data" << endl;
                        break;
                    case 0: // biases
                        biases_q = packet_rx->datas;
                        //cout << "Core_id:" << id << " Receiving " << "biases data" << endl;
                        break;
                    default:
                        break;
                }

                if (img_q.size() > 0 && weights_q.size() > 0 && biases_q.size() > 0)
                {   
                    float *weights = new float[weights_q.size()];
                    float *biases = new float[biases_q.size()];
                    float *img = new float[img_q.size()];

                    for (int i = 0; i < weights_q.size(); i++){
                        weights[i] = weights_q[i];
                    }

                    for (int i = 0; i < biases_q.size(); i++){
                        biases[i] = biases_q[i];
                    }

                    for (int i = 0; i < img_q.size(); i++){
                        img[i] = img_q[i];
                    }

                    if(id == 1){
                        img = padding(img);
                    } 

                    // initialize the parameters
                    int input_img_channel, input_img_size;
                    int conv_in_channel_num, conv_out_channel_num, conv_kernel_size;
                    int conv_padding_size, conv_stride;
                    int mp_out_channel_num, mp_out_shape;
                    int mp_kernel_size, mp_stride;
                    int output_channel_num;
                    int input_channel_num;

                    // layer param
                    switch(id){
                        case (1):{
                            // Conversion to correct size for processing
                            input_img_channel = INPUT_IMG_CHANNEL;
                            input_img_size = PAD_IMG_SIZE;

                            // conv
                            conv_in_channel_num = CONV1_IN_CHANNEL_NUM;
                            conv_out_channel_num = CONV1_OUT_CHANNEL_NUM;
                            conv_kernel_size = CONV1_KERNEL_SIZE;
                            conv_stride = CONV1_STRIDE;
                            conv_padding_size = CONV1_PADDING_SIZE;

                            // mp
                            mp_out_channel_num = MP1_OUT_CHANNEL_NUM;
                            mp_out_shape = MP1_OUT_SHAPE;

                            mp_kernel_size = MP1_KERNEL_SIZE;
                            mp_stride = MP1_STRIDE;

                            break;
                        }
                        case (2):{
                            // Conversion to correct size for processing
                            input_img_channel = CONV2_IN_CHANNEL_NUM;
                            input_img_size = MP1_OUT_SHAPE;

                            // conv
                            conv_out_channel_num = CONV2_OUT_CHANNEL_NUM;
                            conv_in_channel_num = CONV2_IN_CHANNEL_NUM;
                            conv_kernel_size = CONV2_KERNEL_SIZE;
                            conv_stride = CONV2_STRIDE;
                            conv_padding_size = CONV2_PADDING_SIZE;

                            // mp
                            mp_out_channel_num = MP2_OUT_CHANNEL_NUM;
                            mp_out_shape = MP2_OUT_SHAPE;

                            mp_kernel_size = MP2_KERNEL_SIZE;
                            mp_stride = MP2_STRIDE;
                            break;
                        }
                        case (3):{
                            // Conversion to correct size for processing
                            input_img_channel = CONV3_IN_CHANNEL_NUM;
                            input_img_size = CONV3_IN_IMG_SIZE;

                            // conv
                            conv_out_channel_num = CONV3_OUT_CHANNEL_NUM;
                            conv_in_channel_num = CONV3_IN_CHANNEL_NUM;
                            conv_kernel_size = CONV3_KERNEL_SIZE;
                            conv_stride = CONV3_STRIDE;
                            conv_padding_size = CONV3_PADDING_SIZE;
                            break;
                        }
                        case (4):{
                            // Conversion to correct size for processing
                            input_img_channel = CONV4_IN_CHANNEL_NUM;
                            input_img_size = CONV4_IN_IMG_SIZE;

                            // conv
                            conv_out_channel_num = CONV4_OUT_CHANNEL_NUM;
                            conv_in_channel_num = CONV4_IN_CHANNEL_NUM;
                            conv_kernel_size = CONV4_KERNEL_SIZE;
                            conv_stride = CONV4_STRIDE;
                            conv_padding_size = CONV4_PADDING_SIZE;

                            break;
                        }
                        case (5):{ // Conversion to correct size for processing
                            input_img_channel = CONV5_IN_CHANNEL_NUM;
                            input_img_size = CONV5_IN_IMG_SIZE;

                            // conv
                            conv_out_channel_num = CONV5_OUT_CHANNEL_NUM;
                            conv_in_channel_num = CONV5_IN_CHANNEL_NUM;
                            conv_kernel_size = CONV5_KERNEL_SIZE;

                            conv_stride = CONV5_STRIDE;
                            conv_padding_size = CONV5_PADDING_SIZE;

                            // mp
                            mp_out_channel_num = MP5_OUT_CHANNEL_NUM;
                            mp_out_shape = MP5_OUT_SHAPE;

                            mp_kernel_size = MP5_KERNEL_SIZE;
                            mp_stride = MP5_STRIDE;
                            break;
                        }
                        case (6):{
                            output_channel_num = FC6_OUT_NUM;
                            input_channel_num = FC6_IN_NUM;

                            break;
                        }
                        case (7):{
                            output_channel_num = FC7_OUT_NUM;
                            input_channel_num = FC7_IN_NUM;

                            break;
                        }
                        case (8):{
                            output_channel_num = FC8_OUT_NUM;
                            input_channel_num = FC8_IN_NUM;

                            break;
                        }
                    }
                    // Conv layer
                    switch(id){
                        case(1):
                        case(2):
                        case(5):{
                            vec_3d data = convert1dTo3d(img, input_img_channel, input_img_size, input_img_size);
                            vec_4d conv_weights = convert1dTo4d(weights, conv_out_channel_num, conv_in_channel_num, conv_kernel_size, conv_kernel_size);
                            vec_1d conv_bias = convert1dToTensor1d(biases, conv_out_channel_num);

                            // Conv1
                            vec_3d conv_output = convolution(data, conv_weights, conv_bias, conv_in_channel_num, conv_out_channel_num, conv_kernel_size, conv_stride, conv_padding_size);

                            // relu1
                            conv_output = relu_3d(conv_output);

                            // maxpool1

                            vec_3d maxpool_output = max_pooling_3d(conv_output, conv_out_channel_num, mp_kernel_size, mp_stride);

                            result = convert3dTo1dvec(maxpool_output, mp_out_channel_num, maxpool_output[0].size(), maxpool_output[0][0].size());

                            //cout << "Core_" << id << " finish processing" << endl;

                            release_vec_3d(data);
                            release_vec_4d(conv_weights);
                            release_vec_1d(conv_bias);
                            break;
                        }
                        case(3):
                        case(4):{
                            vec_3d data = convert1dTo3d(img, input_img_channel, input_img_size, input_img_size);
                            vec_4d conv_weights = convert1dTo4d(weights, conv_out_channel_num, conv_in_channel_num, conv_kernel_size, conv_kernel_size);
                            vec_1d conv_bias = convert1dToTensor1d(biases, conv_out_channel_num);

                            // Conv1
                            vec_3d conv_output = convolution(data, conv_weights, conv_bias, conv_in_channel_num, conv_out_channel_num, conv_kernel_size, conv_stride, conv_padding_size);

                            // relu1
                            conv_output = relu_3d(conv_output);

                            result = convert3dTo1dvec(conv_output, conv_out_channel_num, input_img_size, input_img_size);

                            //cout << "Core_" << id << " finish processing" << endl;

                            release_vec_3d(data);
                            release_vec_4d(conv_weights);
                            release_vec_1d(conv_bias);
                            
                            break;
                        }
                        case(6):
                        case(7):
                        case(8):{
                            vec_1d data = convert1dToTensor1d(img, input_channel_num);
                            vec_2d fc_weights = convert1dTo2dvec(weights, output_channel_num, input_channel_num);
                            vec_1d fc_bias = convert1dToTensor1d(biases, output_channel_num);

                            if(id == 8){
                                // FC
                                result = fc(data, fc_weights, fc_bias, input_channel_num, output_channel_num);
                            }
                            else{
                                // FC
                                result = fc(data, fc_weights, fc_bias, input_channel_num, output_channel_num);

                                // relu
                                result = relu_1d(result);
                            }

                            //cout << "Core_" << id << " finish processing FC" << endl;
                            

                            release_vec_1d(data);
                            release_vec_2d(fc_weights);
                            release_vec_1d(fc_bias);
                            
                            break;
                        }

                    }

                    

                    done_processing = true;
                }
                else{
                    done_processing = false;
                }
            }

            wait();
        }
    }

    Core(sc_module_name name, int id, sc_trace_file *tf = NULL) : sc_module(name), id(id), tf(tf) {
        SC_THREAD(send_packet);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(receive_packet);
        sensitive << clk.pos();
        dont_initialize();

        SC_THREAD(calculate);
        dont_initialize();
        sensitive << clk.pos();

        std::stringstream ss;

        ss << "core_" << id << ".clk";
        sc_trace(tf, clk, ss.str());

        ss.str(""); ss.clear(); 
        ss << "core_" << id << ".rst_n";
        sc_trace(tf, rst_n, ss.str());

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

#endif
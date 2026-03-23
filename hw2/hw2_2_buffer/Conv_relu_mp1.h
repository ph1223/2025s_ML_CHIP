#ifndef CONV_RELU_MP1_H
#define CONV_RELU_MP1_H

#include <systemc.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <Layer_param.h>
#include <Utils.h>
#include <Conv_functions.h>

typedef vector<float> vec_1d;
typedef vector<vec_1d> vec_2d;
typedef vector<vec_2d> vec_3d;
typedef vector<vec_3d> vec_4d;

using namespace std;

SC_MODULE(Conv_relu_mp1){
    // Given 4 inputs
public:
    sc_port<sc_signal_in_if<float *> > img_i;
    sc_port<sc_signal_in_if<bool> > valid_i;
    sc_in<bool> clk;

    // Output uses fifo port fort outer connection
    sc_port<sc_signal_out_if<bool> > valid_o;
    sc_port<sc_signal_out_if<float *> > img_o;
    
    void run(){
        while(1){
            wait();
            valid_o->write(0);
            if(valid_i->read() == 1){
                string file_dir = "../data/";

                // Read in conv1 weights and bias
                vec_4d conv1_weights(CONV1_OUT_CHANNEL_NUM, vec_3d(CONV1_IN_CHANNEL_NUM, vec_2d(CONV1_KERNEL_SIZE, vec_1d(CONV1_KERNEL_SIZE))));
                vec_1d conv1_bias(CONV1_OUT_CHANNEL_NUM);

                conv1_weights = read_weights_4d(file_dir+"conv1_weight.txt", CONV1_OUT_CHANNEL_NUM, CONV1_IN_CHANNEL_NUM, CONV1_KERNEL_SIZE, CONV1_KERNEL_SIZE);
                conv1_bias = read_1d(file_dir + "conv1_bias.txt", CONV1_OUT_CHANNEL_NUM);

                vec_3d data_padded = convert1dTo3d(img_i->read(), INPUT_IMG_CHANNEL, PAD_IMG_SIZE, PAD_IMG_SIZE);

                // Conv1
                vec_3d conv1_output = convolution(data_padded, conv1_weights, conv1_bias, CONV1_IN_CHANNEL_NUM, CONV1_OUT_CHANNEL_NUM, CONV1_KERNEL_SIZE, 4, 0);

                // relu1
                conv1_output = relu_3d(conv1_output);

                // maxpool1

                vec_3d maxpool1_output = max_pooling_3d(conv1_output, CONV1_OUT_CHANNEL_NUM, 3, 2);

                float *result = convert3dTo1d(maxpool1_output, CONV1_OUT_CHANNEL_NUM, maxpool1_output[0].size(), maxpool1_output[0][0].size());

                wait();
                // Write output image
                img_o->write(result);
                valid_o->write(1);

                // release conv1_weight and conv1_bias
                release_vec_4d(conv1_weights);
                release_vec_1d(conv1_bias);
                
            }
            
        }
    }

    SC_CTOR(Conv_relu_mp1)
    {
        SC_THREAD(run);
        dont_initialize();
        sensitive << clk.pos();
    }
};

#endif
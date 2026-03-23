#ifndef CONV_RELU_MP2_H
#define CONV_RELU_MP2_H

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

SC_MODULE(Conv_relu_mp2){
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

                // Read in conv2 weights and bias
                vec_4d conv2_weights(CONV2_OUT_CHANNEL_NUM, vec_3d(CONV2_IN_CHANNEL_NUM, vec_2d(CONV2_KERNEL_SIZE, vec_1d(CONV2_KERNEL_SIZE))));
                vec_1d conv2_bias(CONV2_OUT_CHANNEL_NUM);

                conv2_weights = read_weights_4d(file_dir + "conv2_weight.txt", CONV2_OUT_CHANNEL_NUM, CONV2_IN_CHANNEL_NUM, CONV2_KERNEL_SIZE, CONV2_KERNEL_SIZE);
                conv2_bias = read_1d(file_dir + "conv2_bias.txt", CONV2_OUT_CHANNEL_NUM);

                vec_3d maxpool1_output = convert1dTo3d(img_i->read(), MP1_OUT_CHANNEL_NUM, MP1_OUT_SHAPE, MP1_OUT_SHAPE);

                // conv2
                vec_3d conv2_output = convolution(maxpool1_output, conv2_weights, conv2_bias, CONV2_IN_CHANNEL_NUM, CONV2_OUT_CHANNEL_NUM, CONV2_KERNEL_SIZE, 1, 2);

                // relu2
                conv2_output = relu_3d(conv2_output);

                // maxpool2
                vec_3d maxpool2_output = max_pooling_3d(conv2_output, CONV2_OUT_CHANNEL_NUM, 3, 2);

                float *result = convert3dTo1d(maxpool2_output, MP2_OUT_CHANNEL_NUM, maxpool2_output[0].size(), maxpool2_output[0][0].size());

                wait();
                // Write output image
                img_o->write(result);
                valid_o->write(1);

                // release conv2_weight and conv2_bias
                release_vec_4d(conv2_weights);
                release_vec_1d(conv2_bias);
                
            }
            
        }
    }

    SC_CTOR(Conv_relu_mp2)
    {
        SC_THREAD(run);
        dont_initialize();
        sensitive << clk.pos();
    }
};

#endif
#ifndef CONV_RELU_MP5_H
#define CONV_RELU_MP5_H

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

SC_MODULE(Conv_relu_mp5){
    // Given 4 inputs
    public:
    sc_port<sc_fifo_in_if<float *> > img_i;
    sc_port<sc_fifo_in_if<bool> > valid_i;
    sc_in<bool> clk;

    // Output uses fifo port fort outer connection
    sc_port<sc_fifo_out_if<bool> > valid_o;
    sc_port<sc_fifo_out_if<float *> > img_o;
    
    void run(){
        while(1){
            wait();
            valid_o->write(0);

            if(valid_i->read() == 1){
                string file_dir = "../data/"; 

                // Read in conv5 weights and bias
                vec_4d conv5_weights(CONV5_OUT_CHANNEL_NUM, vec_3d(CONV5_IN_CHANNEL_NUM, vec_2d(CONV5_KERNEL_SIZE, vec_1d(CONV5_KERNEL_SIZE))));
                vec_1d conv5_bias(CONV5_OUT_CHANNEL_NUM);

                conv5_weights = read_weights_4d(file_dir + "conv5_weight.txt", CONV5_OUT_CHANNEL_NUM, CONV5_IN_CHANNEL_NUM, CONV5_KERNEL_SIZE, CONV5_KERNEL_SIZE);
                conv5_bias = read_1d(file_dir + "conv5_bias.txt", CONV5_OUT_CHANNEL_NUM);


                vec_3d conv4_output = convert1dTo3d(img_i->read(), CONV5_OUT_CHANNEL_NUM, CONV5_OUT_SHAPE, CONV5_OUT_SHAPE);

                // conv5
                vec_3d conv5_output = convolution(conv4_output, conv5_weights, conv5_bias, CONV5_IN_CHANNEL_NUM, CONV5_OUT_CHANNEL_NUM, CONV5_KERNEL_SIZE, 1, 1);

                // relu5
                conv5_output = relu_3d(conv5_output);

                // maxpool5
                vec_3d maxpool5_output = max_pooling_3d(conv5_output, CONV5_OUT_CHANNEL_NUM, 3, 2);
                float *result = convert3dTo1d(maxpool5_output, MP5_OUT_CHANNEL_NUM, MP5_OUT_SHAPE, MP5_OUT_SHAPE);

                wait();
                // Write output image
                img_o->write(result);
                valid_o->write(1);

                // release conv5_weight and conv5_bias
                release_vec_4d(conv5_weights);
                release_vec_1d(conv5_bias);

                
            }
            
        }
    }

    SC_CTOR(Conv_relu_mp5)
    {
        SC_THREAD(run);
        dont_initialize();
        sensitive << clk.pos();
    }
};

#endif
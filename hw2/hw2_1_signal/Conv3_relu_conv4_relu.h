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

SC_MODULE(Conv3_relu_conv4_relu){
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

                // Read in conv3 weights and bias
                vec_4d conv3_weights(CONV3_OUT_CHANNEL_NUM, vec_3d(CONV3_IN_CHANNEL_NUM, vec_2d(CONV3_KERNEL_SIZE, vec_1d(CONV3_KERNEL_SIZE))));
                vec_1d conv3_bias(CONV3_OUT_CHANNEL_NUM);

                conv3_weights = read_weights_4d(file_dir + "conv3_weight.txt", CONV3_OUT_CHANNEL_NUM, CONV3_IN_CHANNEL_NUM, CONV3_KERNEL_SIZE, CONV3_KERNEL_SIZE);
                conv3_bias = read_1d(file_dir + "conv3_bias.txt", CONV3_OUT_CHANNEL_NUM);

                vec_3d maxpool2_output = convert1dTo3d(img_i->read(), MP2_OUT_CHANNEL_NUM, MP2_OUT_SHAPE, MP2_OUT_SHAPE);

                // conv3
                vec_3d conv3_output = convolution(maxpool2_output, conv3_weights, conv3_bias, CONV3_IN_CHANNEL_NUM, CONV3_OUT_CHANNEL_NUM, CONV3_KERNEL_SIZE, 1, 1);

                // relu3
                conv3_output = relu_3d(conv3_output);

                // release conv3_weight and conv3_bias
                release_vec_4d(conv3_weights);
                release_vec_1d(conv3_bias);

                // Read in conv4 weights and bias
                vec_4d conv4_weights(CONV4_OUT_CHANNEL_NUM, vec_3d(CONV4_IN_CHANNEL_NUM, vec_2d(CONV4_KERNEL_SIZE, vec_1d(CONV4_KERNEL_SIZE))));
                vec_1d conv4_bias(CONV4_OUT_CHANNEL_NUM);

                conv4_weights = read_weights_4d(file_dir + "conv4_weight.txt", CONV4_OUT_CHANNEL_NUM, CONV4_IN_CHANNEL_NUM, CONV4_KERNEL_SIZE, CONV4_KERNEL_SIZE);
                conv4_bias = read_1d(file_dir + "conv4_bias.txt", CONV4_OUT_CHANNEL_NUM);


                // conv4

                vec_3d conv4_output = convolution(conv3_output, conv4_weights, conv4_bias, CONV4_IN_CHANNEL_NUM, CONV4_OUT_CHANNEL_NUM, CONV4_KERNEL_SIZE, 1, 1);


                // relu4
                conv4_output = relu_3d(conv4_output);

                float *result = convert3dTo1d(conv4_output, CONV4_OUT_CHANNEL_NUM, CONV4_OUT_SHAPE, CONV4_OUT_SHAPE);

                wait();
                // Write output image
                img_o->write(result);
                valid_o->write(1);

                // release conv4_weight and conv4_bias
                release_vec_4d(conv4_weights);
                release_vec_1d(conv4_bias);
                
            }
            
        }
    }

    SC_CTOR(Conv3_relu_conv4_relu)
    {
        SC_THREAD(run);
        dont_initialize();
        sensitive << clk.pos();
    }
};
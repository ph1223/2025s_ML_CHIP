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

SC_MODULE(Fc){
    // Given 4 inputs
    public:
    sc_port<sc_fifo_in_if<float *> > img_i;
    sc_port<sc_fifo_in_if<bool> > valid_i;
    sc_in<bool> clk;

    // Output uses fifo port fort outer connection
    sc_port<sc_fifo_out_if<bool> > valid_o;
    sc_port<sc_fifo_out_if<float *> > img_o;

    string file_dir;
    string weight_dir;
    string bias_dir;

    int fc_num;
    int input_channel_num;
    int output_channel_num;
    
    void run(){
        while(1){
            wait();
            valid_o->write(0);

            if(valid_i->read() == 1){

                // read in fc6 weights and bias
                vec_2d fc6_weights(output_channel_num, vec_1d(input_channel_num));
                vec_1d fc6_bias(output_channel_num);

                fc6_weights = read_weights_2d(file_dir + weight_dir, output_channel_num, input_channel_num);
                fc6_bias = read_1d(file_dir + bias_dir, output_channel_num);

                // fc6
                vec_1d fc6_output = fc(convert1dToTensor1d(img_i->read(), input_channel_num), fc6_weights, fc6_bias, input_channel_num, output_channel_num);

                // relu6
                if(fc_num != 8)
                    fc6_output = relu_1d(fc6_output);

                wait();
                // Write output image
                img_o->write(convertTensor1dTo1d(fc6_output, output_channel_num));
                valid_o->write(1);

                 // release fc6_weight and fc6_bias
                release_vec_2d(fc6_weights);
                release_vec_1d(fc6_bias);

                
            }
            
        }
    }

    SC_CTOR(Fc)
    {
        SC_THREAD(run);
        dont_initialize();
        sensitive << clk.pos();
    }
};
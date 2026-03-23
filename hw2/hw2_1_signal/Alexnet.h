#ifndef ALEXNET_H
#define ALEXNET_H

#include <systemc.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <Layer_param.h>
#include <Utils.h>
#include <Conv_functions.h>
#include <iomanip>
#include <Conv_relu_mp1.h>
#include <Conv_relu_mp2.h>
#include <Conv3_relu_conv4_relu.h>
#include <Conv_relu_mp5.h>
#include <Fc.h>

using namespace std;
using namespace sc_core;



typedef vector<float> vec_1d;
typedef vector<vec_1d> vec_2d;
typedef vector<vec_2d> vec_3d;
typedef vector<vec_3d> vec_4d;

SC_MODULE(Alexnet) {
    SC_HAS_PROCESS(Alexnet);
    // Given 4 inputs
    sc_in<bool> clk, start;
    sc_out<bool> done;

    sc_signal<float*> img_padding_o;
    sc_signal<float*> conv1_output_o;
    sc_signal<float*> conv2_output_o;
    sc_signal<float*> conv34_output_o;
    sc_signal<float*> conv5_output_o;
    sc_signal<float*> fc6_output_o;
    sc_signal<float*> fc7_output_o;
    sc_signal<float*> fc8_output_o;

    sc_signal<bool> start_pad;
    sc_signal<bool> pad_conv1_valid;
    sc_signal<bool> conv1_conv2_valid;
    sc_signal<bool> conv2_conv34_valid;
    sc_signal<bool> conv34_conv5_valid;
    sc_signal<bool> conv5_fc6_valid;
    sc_signal<bool> fc6_fc7_valid;
    sc_signal<bool> fc7_fc8_valid;
    sc_signal<bool> fc8_out_valid;

    Conv_relu_mp1 m_conv1{"m_conv1"};
    Conv_relu_mp2 m_conv2{"m_conv2"};
    Conv3_relu_conv4_relu m_conv34{"m_conv34"};
    Conv_relu_mp5 m_conv5{"m_conv5"};
    Fc m_fc6{"m_fc6"};
    Fc m_fc7{"m_fc7"};
    Fc m_fc8{"m_fc8"};

    string input_file;

    string file_dir = "../data/";

    void start_input() {
        while (1) {
            wait();
            if (start.read() == 1) {
                start_pad.write(1);
            }
            else{
                start_pad.write(0);
            }
        }
    }

    void process() {
        
        while(1){
            wait();
            //cout << "Start padding:"<< start.read()  << endl; 
            //cout << start_pad.read() << endl;
            if(start_pad.read() == 1){
                
                // Load data
                vec_3d data(INPUT_IMG_CHANNEL, vec_2d(INPUT_IMG_HEIGHT, vec_1d(INPUT_IMG_WIDTH)));
                data = load_data(file_dir + input_file, INPUT_IMG_CHANNEL, INPUT_IMG_HEIGHT, INPUT_IMG_WIDTH);

                // Pad data
                vec_3d data_padded(INPUT_IMG_CHANNEL, vec_2d(PAD_IMG_SIZE, vec_1d(PAD_IMG_SIZE)));

                for(int c = 0; c < INPUT_IMG_CHANNEL; c++) {
                    for(int h = 0; h < INPUT_IMG_HEIGHT; h++) {
                        for(int w = 0; w < INPUT_IMG_WIDTH; w++) {
                            data_padded[c][h + 2][w + 2] = data[c][h][w];
                        }
                    }
                }

                float *result = convert3dTo1d(data_padded, INPUT_IMG_CHANNEL, PAD_IMG_SIZE, PAD_IMG_SIZE);
                img_padding_o.write(result);
                pad_conv1_valid.write(1);
            }
            else{
                pad_conv1_valid.write(0);
            }
        }
        

        

        //cout << "Alexnet process done" << endl;
    }

    void output() {
        while(1){
            wait();
            if(fc8_out_valid.read() == 1){

                float *fc8_output = fc8_output_o.read();

                vec_1d fc8_output_vec = convert1dToTensor1d(fc8_output, FC8_OUT_NUM);

                // softmax
                vec_1d softmax_output = softmax(fc8_output_vec);

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
                vector<pair<string, float> > top_100_val = sorting_class(fc8_output_vec, labels);
            
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
                done.write(1);
            }
            else{
                done.write(0);
            }
        }
    }

    Alexnet(sc_module_name name, const string& file) : sc_module(name), input_file(file) {
        m_conv1.img_i(img_padding_o);
        m_conv1.valid_i(pad_conv1_valid);
        m_conv1.clk(clk);
        m_conv1.valid_o(conv1_conv2_valid);
        m_conv1.img_o(conv1_output_o);

        m_conv2.img_i(conv1_output_o);
        m_conv2.valid_i(conv1_conv2_valid);
        m_conv2.clk(clk);
        m_conv2.valid_o(conv2_conv34_valid);
        m_conv2.img_o(conv2_output_o);

        m_conv34.img_i(conv2_output_o);
        m_conv34.valid_i(conv2_conv34_valid);
        m_conv34.clk(clk);
        m_conv34.valid_o(conv34_conv5_valid);
        m_conv34.img_o(conv34_output_o);

        m_conv5.img_i(conv34_output_o);
        m_conv5.valid_i(conv34_conv5_valid);
        m_conv5.clk(clk);
        m_conv5.valid_o(conv5_fc6_valid);
        m_conv5.img_o(conv5_output_o);

        m_fc6.img_i(conv5_output_o);
        m_fc6.valid_i(conv5_fc6_valid);
        m_fc6.clk(clk);
        m_fc6.valid_o(fc6_fc7_valid);
        m_fc6.img_o(fc6_output_o);

        m_fc6.file_dir = "../data/";
        m_fc6.weight_dir = "fc6_weight.txt";
        m_fc6.bias_dir = "fc6_bias.txt";
        m_fc6.fc_num = 6;
        m_fc6.input_channel_num = FC6_IN_NUM;
        m_fc6.output_channel_num = FC6_OUT_NUM;

        m_fc7.img_i(fc6_output_o);
        m_fc7.valid_i(fc6_fc7_valid);
        m_fc7.clk(clk);
        m_fc7.valid_o(fc7_fc8_valid);
        m_fc7.img_o(fc7_output_o);

        m_fc7.file_dir = "../data/";
        m_fc7.weight_dir = "fc7_weight.txt";
        m_fc7.bias_dir = "fc7_bias.txt";
        m_fc7.fc_num = 7;
        m_fc7.input_channel_num = FC7_IN_NUM;
        m_fc7.output_channel_num = FC7_OUT_NUM;

        m_fc8.img_i(fc7_output_o);
        m_fc8.valid_i(fc7_fc8_valid);
        m_fc8.clk(clk);
        m_fc8.valid_o(fc8_out_valid);
        m_fc8.img_o(fc8_output_o);

        m_fc8.file_dir = "../data/";
        m_fc8.weight_dir = "fc8_weight.txt";
        m_fc8.bias_dir = "fc8_bias.txt";
        m_fc8.fc_num = 8;
        m_fc8.input_channel_num = FC8_IN_NUM;
        m_fc8.output_channel_num = FC8_OUT_NUM;
        
        SC_THREAD(start_input);
        dont_initialize();
        sensitive << clk.pos();

        SC_THREAD(process);
        dont_initialize();
        sensitive << clk.pos();

        SC_THREAD(output);
        dont_initialize();
        sensitive << clk.pos();

    }

    ~Alexnet() {

    }
};









#endif


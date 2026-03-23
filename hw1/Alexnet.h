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

using namespace std;
using namespace sc_core;



typedef vector<float> vec_1d;
typedef vector<vec_1d> vec_2d;
typedef vector<vec_2d> vec_3d;
typedef vector<vec_3d> vec_4d;

SC_MODULE(Alexnet) {
    SC_HAS_PROCESS(Alexnet);
    string input_file;

    string file_dir = "./data/";

    void process() {
        //cout << "Alexnet process start" << endl;
        //cout << "==================================================" << endl;
        //cout << "start loading " << input_file << " data" << endl;
        //cout << "==================================================" << endl;

        
        // Load data
        vec_3d data(INPUT_IMG_CHANNEL, vec_2d(INPUT_IMG_HEIGHT, vec_1d(INPUT_IMG_WIDTH)));
        data = load_data(file_dir + input_file, INPUT_IMG_CHANNEL, INPUT_IMG_HEIGHT, INPUT_IMG_WIDTH);
        //cout << "data loaded" << endl;
        //cout << "==================================================" << endl;

        // Display data
        //display_vec_3d(data, 1, 16, 16, 0, 1, 0, 16, 0, 16);
        //cout << "==================================================" << endl;

        vec_3d data_padded(INPUT_IMG_CHANNEL, vec_2d(PAD_IMG_SIZE, vec_1d(PAD_IMG_SIZE)));

        // Pad data
        for(int c = 0; c < INPUT_IMG_CHANNEL; c++) {
            for(int h = 0; h < INPUT_IMG_HEIGHT; h++) {
                for(int w = 0; w < INPUT_IMG_WIDTH; w++) {
                    data_padded[c][h + 2][w + 2] = data[c][h][w];
                }
            }
        }

        //cout << "data padded" << endl;
        //cout << "==================================================" << endl;

        // Display padded data
        //display_vec_3d(data_padded,1, 227, 227, 0, 1, 224, 227, 224, 227);
        //cout << "==================================================" << endl;

        // Read in conv1 weights and bias
        vec_4d conv1_weights(CONV1_OUT_CHANNEL_NUM, vec_3d(CONV1_IN_CHANNEL_NUM, vec_2d(CONV1_KERNEL_SIZE, vec_1d(CONV1_KERNEL_SIZE))));
        vec_1d conv1_bias(CONV1_OUT_CHANNEL_NUM);

        conv1_weights = read_weights_4d(file_dir+"conv1_weight.txt", CONV1_OUT_CHANNEL_NUM, CONV1_IN_CHANNEL_NUM, CONV1_KERNEL_SIZE, CONV1_KERNEL_SIZE);
        conv1_bias = read_1d(file_dir + "conv1_bias.txt", CONV1_OUT_CHANNEL_NUM);
        //cout << "conv1 weights and bias read" << endl;
        //cout << "==================================================" << endl;

        // Display resized data

        //display_vec_4d(conv1_weights, 1, 3, 11, 11, 0, 1, 0, 3, 0, 11, 0, 11);
        //display_vec_1d(conv1_bias,20, 0, 20);
        //cout << "==================================================" << endl;

        // Conv1
        //cout << "conv1 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d conv1_output = convolution(data_padded, conv1_weights, conv1_bias, CONV1_IN_CHANNEL_NUM, CONV1_OUT_CHANNEL_NUM, CONV1_KERNEL_SIZE, 4, 0);
        //cout << "conv1 done" << endl;
        //cout << "==================================================" << endl;

        // relu1
        //cout << "relu1 start" << endl;
        //cout << "==================================================" << endl;
        conv1_output = relu_3d(conv1_output);
        //cout << "relu1 done" << endl;
        //cout << "==================================================" << endl;

        // maxpool1
        //cout << "maxpool1 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d maxpool1_output = max_pooling_3d(conv1_output, CONV1_OUT_CHANNEL_NUM, 3, 2);
        //cout << "maxpool1 done" << endl;
        //cout << "==================================================" << endl;

        // release conv1_weight and conv1_bias
        release_vec_4d(conv1_weights);
        release_vec_1d(conv1_bias);

        // Read in conv2 weights and bias
        vec_4d conv2_weights(CONV2_OUT_CHANNEL_NUM, vec_3d(CONV2_IN_CHANNEL_NUM, vec_2d(CONV2_KERNEL_SIZE, vec_1d(CONV2_KERNEL_SIZE))));
        vec_1d conv2_bias(CONV2_OUT_CHANNEL_NUM);

        conv2_weights = read_weights_4d(file_dir + "conv2_weight.txt", CONV2_OUT_CHANNEL_NUM, CONV2_IN_CHANNEL_NUM, CONV2_KERNEL_SIZE, CONV2_KERNEL_SIZE);
        conv2_bias = read_1d(file_dir + "conv2_bias.txt", CONV2_OUT_CHANNEL_NUM);
        //cout << "conv2 weights and bias read" << endl;
        //cout << "==================================================" << endl;

        // conv2
        //cout << "conv2 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d conv2_output = convolution(maxpool1_output, conv2_weights, conv2_bias, CONV2_IN_CHANNEL_NUM, CONV2_OUT_CHANNEL_NUM, CONV2_KERNEL_SIZE, 1, 2);
        //cout << "conv2 done" << endl;
        //cout << "==================================================" << endl;

        // relu2
        //cout << "relu2 start" << endl;
        //cout << "==================================================" << endl;
        conv2_output = relu_3d(conv2_output);
        //cout << "relu2 done" << endl;
        //cout << "==================================================" << endl;

        // maxpool2
        //cout << "maxpool2 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d maxpool2_output = max_pooling_3d(conv2_output, CONV2_OUT_CHANNEL_NUM, 3, 2);
        //cout << "maxpool2 done" << endl;
        //cout << "==================================================" << endl;

        // release conv2_weight and conv2_bias
        release_vec_4d(conv2_weights);
        release_vec_1d(conv2_bias);

        // Read in conv3 weights and bias
        vec_4d conv3_weights(CONV3_OUT_CHANNEL_NUM, vec_3d(CONV3_IN_CHANNEL_NUM, vec_2d(CONV3_KERNEL_SIZE, vec_1d(CONV3_KERNEL_SIZE))));
        vec_1d conv3_bias(CONV3_OUT_CHANNEL_NUM);

        conv3_weights = read_weights_4d(file_dir + "conv3_weight.txt", CONV3_OUT_CHANNEL_NUM, CONV3_IN_CHANNEL_NUM, CONV3_KERNEL_SIZE, CONV3_KERNEL_SIZE);
        conv3_bias = read_1d(file_dir + "conv3_bias.txt", CONV3_OUT_CHANNEL_NUM);
        //cout << "conv3 weights and bias read" << endl;
        //cout << "==================================================" << endl;

        // conv3
        //cout << "conv3 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d conv3_output = convolution(maxpool2_output, conv3_weights, conv3_bias, CONV3_IN_CHANNEL_NUM, CONV3_OUT_CHANNEL_NUM, CONV3_KERNEL_SIZE, 1, 1);
        //cout << "conv3 done" << endl;
        //cout << "==================================================" << endl;

        // relu3
        //cout << "relu3 start" << endl;
        //cout << "==================================================" << endl;
        conv3_output = relu_3d(conv3_output);
        //cout << "relu3 done" << endl;
        //cout << "==================================================" << endl;

        // release conv3_weight and conv3_bias
        release_vec_4d(conv3_weights);
        release_vec_1d(conv3_bias);

        // Read in conv4 weights and bias
        vec_4d conv4_weights(CONV4_OUT_CHANNEL_NUM, vec_3d(CONV4_IN_CHANNEL_NUM, vec_2d(CONV4_KERNEL_SIZE, vec_1d(CONV4_KERNEL_SIZE))));
        vec_1d conv4_bias(CONV4_OUT_CHANNEL_NUM);

        conv4_weights = read_weights_4d(file_dir + "conv4_weight.txt", CONV4_OUT_CHANNEL_NUM, CONV4_IN_CHANNEL_NUM, CONV4_KERNEL_SIZE, CONV4_KERNEL_SIZE);
        conv4_bias = read_1d(file_dir + "conv4_bias.txt", CONV4_OUT_CHANNEL_NUM);
        //cout << "conv4 weights and bias read" << endl;
        //cout << "==================================================" << endl;

        // conv4
        //cout << "conv4 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d conv4_output = convolution(conv3_output, conv4_weights, conv4_bias, CONV4_IN_CHANNEL_NUM, CONV4_OUT_CHANNEL_NUM, CONV4_KERNEL_SIZE, 1, 1);
        //cout << "conv4 done" << endl;
        //cout << "==================================================" << endl;

        // relu4
        //cout << "relu4 start" << endl;
        //cout << "==================================================" << endl;
        conv4_output = relu_3d(conv4_output);
        //cout << "relu4 done" << endl;
        //cout << "==================================================" << endl;

        // release conv4_weight and conv4_bias
        release_vec_4d(conv4_weights);
        release_vec_1d(conv4_bias);

        // Read in conv5 weights and bias
        vec_4d conv5_weights(CONV5_OUT_CHANNEL_NUM, vec_3d(CONV5_IN_CHANNEL_NUM, vec_2d(CONV5_KERNEL_SIZE, vec_1d(CONV5_KERNEL_SIZE))));
        vec_1d conv5_bias(CONV5_OUT_CHANNEL_NUM);

        conv5_weights = read_weights_4d(file_dir + "conv5_weight.txt", CONV5_OUT_CHANNEL_NUM, CONV5_IN_CHANNEL_NUM, CONV5_KERNEL_SIZE, CONV5_KERNEL_SIZE);
        conv5_bias = read_1d(file_dir + "conv5_bias.txt", CONV5_OUT_CHANNEL_NUM);

        //cout << "conv5 weights and bias read" << endl;
        //cout << "==================================================" << endl;

        // conv5
        //cout << "conv5 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d conv5_output = convolution(conv4_output, conv5_weights, conv5_bias, CONV5_IN_CHANNEL_NUM, CONV5_OUT_CHANNEL_NUM, CONV5_KERNEL_SIZE, 1, 1);
        //cout << "conv5 done" << endl;
        //cout << "==================================================" << endl;

        // relu5
        //cout << "relu5 start" << endl;
        //cout << "==================================================" << endl;
        conv5_output = relu_3d(conv5_output);
        //cout << "relu5 done" << endl;
        //cout << "==================================================" << endl;

        // maxpool5
        //cout << "maxpool5 start" << endl;
        //cout << "==================================================" << endl;
        vec_3d maxpool5_output = max_pooling_3d(conv5_output, CONV5_OUT_CHANNEL_NUM, 3, 2);
        //cout << "maxpool5 done" << endl;
        //cout << "==================================================" << endl;

        // release conv5_weight and conv5_bias
        release_vec_4d(conv5_weights);
        release_vec_1d(conv5_bias);

        // read in fc6 weights and bias
        vec_2d fc6_weights(FC6_OUT_NUM, vec_1d(FC6_IN_NUM));
        vec_1d fc6_bias(FC6_OUT_NUM);

        fc6_weights = read_weights_2d(file_dir + "fc6_weight.txt", FC6_OUT_NUM, FC6_IN_NUM);
        fc6_bias = read_1d(file_dir + "fc6_bias.txt", FC6_OUT_NUM);
        //cout << "fc6 weights and bias read" << endl;
        //cout << "==================================================" << endl;


        // Flatten
        //cout << "flatten start" << endl;
        //cout << "==================================================" << endl;

        vec_1d flatten_output = flatten(maxpool5_output);
        //cout << "flatten done" << endl;
        //cout << "==================================================" << endl;

        // fc6
        //cout << "fc6 start" << endl;
        //cout << "==================================================" << endl;
        vec_1d fc6_output = fc(flatten_output, fc6_weights, fc6_bias, FC6_IN_NUM, FC6_OUT_NUM);
        //cout << "fc6 done" << endl;
        //cout << "==================================================" << endl;

        // relu6
        //cout << "relu6 start" << endl;
        //cout << "==================================================" << endl;
        fc6_output = relu_1d(fc6_output);
        //cout << "relu6 done" << endl;
        //cout << "==================================================" << endl;

        // release fc6_weight and fc6_bias
        release_vec_2d(fc6_weights);
        release_vec_1d(fc6_bias);

        // read in fc7 weights and bias
        vec_2d fc7_weights(FC7_OUT_NUM, vec_1d(FC7_IN_NUM));
        vec_1d fc7_bias(FC7_OUT_NUM);

        fc7_weights = read_weights_2d(file_dir + "fc7_weight.txt", FC7_OUT_NUM, FC7_IN_NUM);
        fc7_bias = read_1d(file_dir + "fc7_bias.txt", FC7_OUT_NUM);
        //cout << "fc7 weights and bias read" << endl;
        //cout << "==================================================" << endl;

        // fc7
        //cout << "fc7 start" << endl;
        //cout << "==================================================" << endl;
        vec_1d fc7_output = fc(fc6_output, fc7_weights, fc7_bias, FC7_IN_NUM, FC7_OUT_NUM);
        //cout << "fc7 done" << endl;
        //cout << "==================================================" << endl;

        // relu7
        //cout << "relu7 start" << endl;
        //cout << "==================================================" << endl;
        fc7_output = relu_1d(fc7_output);
        //cout << "relu7 done" << endl;
        //cout << "==================================================" << endl;

        // release fc7_weight and fc7_bias
        release_vec_2d(fc7_weights);
        release_vec_1d(fc7_bias);

        // read in fc8 weights and bias
        vec_2d fc8_weights(FC8_OUT_NUM, vec_1d(FC8_IN_NUM));
        vec_1d fc8_bias(FC8_OUT_NUM);

        fc8_weights = read_weights_2d(file_dir + "fc8_weight.txt", FC8_OUT_NUM, FC8_IN_NUM);
        fc8_bias = read_1d(file_dir + "fc8_bias.txt", FC8_OUT_NUM);
        //cout << "fc8 weights and bias read" << endl;
        //cout << "==================================================" << endl;

        // fc8
        //cout << "fc8 start" << endl;
        //cout << "==================================================" << endl;
        vec_1d fc8_output = fc(fc7_output, fc8_weights, fc8_bias, FC8_IN_NUM, FC8_OUT_NUM);
        //cout << "fc8 done" << endl;
        //cout << "==================================================" << endl;

        // release fc8_weight and fc8_bias
        release_vec_2d(fc8_weights);
        release_vec_1d(fc8_bias);

        // softmax
        //cout << "softmax start" << endl;
        //cout << "==================================================" << endl;
        vec_1d softmax_output = softmax(fc8_output);
        //cout << "softmax done" << endl;
        //cout << "==================================================" << endl;

        // read in labels
        //vector<string> labels = read_labels(file_dir + "imagenet_classes.txt");
        string file_path = file_dir + "imagenet_classes.txt";
        ifstream file(file_path.c_str());
        vector<string> labels;
        string label;


        while(getline(file, label)) {
            labels.push_back(label);
        }

        // get top 100 labels
        vector<pair<string, float> > top_100_pos = sorting_class(softmax_output, labels);
        vector<pair<string, float> > top_100_val = sorting_class(fc8_output, labels);

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

        //cout << "Alexnet process done" << endl;
    }

    Alexnet(sc_module_name name, const string& file) : sc_module(name), input_file(file) {
        SC_METHOD(process);
        //cout << "Alexnet constructed" << endl;
    }

    ~Alexnet() {
        //cout << "Alexnet destructed" << endl;
    }
};









#endif


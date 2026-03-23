#ifndef UTILS_H
#define UTILS_H

#include <systemc.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <Layer_param.h>

using namespace std;

typedef vector<float> vec_1d;
typedef vector<vec_1d> vec_2d;
typedef vector<vec_2d> vec_3d;
typedef vector<vec_3d> vec_4d;

vec_3d load_data(const string&, const int, const int, const int);
void display_vec_1d(const vec_1d&, const int, const int);
void display_vec_3d(const vec_3d&, const int, const int, const int, const int, const int, const int, const int, const int, const int);
void display_vec_4d(const vec_4d&, const int, const int, const int, const int, const int, const int, const int, const int, const int, const int, const int, const int);
vec_1d read_1d(const string&, const int);
vec_2d read_weights_2d(const string&, const int, const int);
vec_4d read_weights_4d(const string&, const int, const int, const int, const int);
void release_vec_4d(vec_4d&);
void release_vec_3d(vec_3d&);
void release_vec_2d(vec_2d&);
void release_vec_1d(vec_1d&);

vec_3d load_data(const string& input_file_path, const int num_channels, const int height, const int width) {
    ifstream input_file(input_file_path.c_str());
    float value;
    vec_3d data(num_channels, vec_2d(height, vec_1d(width, 0)));
    for(int c = 0; c < num_channels; c++) {
        for(int h = 0; h < height; h++) {
            for(int w = 0; w < width; w++) {
                input_file >> data[c][h][w];
            }
        }
    }

    input_file.close();

    return data;
}

void display_vec_1d(const vec_1d& data, const int num_elements, const int start, const int end) {
    for(int i = start; i < end && i < num_elements; i++) {
        cout << data[i] << " ";
    }
    cout << endl;
}

void display_vec_3d(const vec_3d& data, const int num_channels, const int height, const int width, const int start_channel, const int end_channel, const int start_height, const int end_height, const int start_width, const int end_width) {
    for(int c = start_channel; c < end_channel && c < num_channels; c++) {
        cout << "Channel " << c << endl;
        for(int h = start_height; h < end_height && h < height; h++) {
            for(int w = start_width; w < end_width && w < width; w++) {
                cout << data[c][h][w] << " ";
            }
            cout << endl;
        }
        cout << endl;
    }
}

void display_vec_4d(const vec_4d& data, const int num_out_channels, const int num_in_channels, const int kernel_height, const int kernel_width, const int start_out_channel, const int end_out_channel, const int start_in_channel, const int end_in_channel, const int start_height, const int end_height, const int start_width, const int end_width) {
    for(int o = start_out_channel; o < end_out_channel && o < num_out_channels; o++) {
        cout << "Output Channel " << o << endl;
        for(int i = start_in_channel; i < end_in_channel && i < num_in_channels; i++) {
            cout << "Input Channel " << i << endl;
            for(int h = start_height; h < end_height && h < kernel_height; h++) {
                for(int w = start_width; w < end_width && w < kernel_width; w++) {
                    cout << data[o][i][h][w] << " ";
                }
                cout << endl;
            }
            cout << endl;
        }
        cout << endl;
    }
}

vec_1d read_1d(const string& file_path, const int num_elements) {
    ifstream file(file_path.c_str());
    vec_1d data(num_elements, 0);

    for(int i = 0; i < num_elements; i++) {
        file >> data[i];
    }
    file.close();
    return data;
}

vec_2d read_weights_2d(const string& file_path, const int row, const int col) {
    ifstream file(file_path.c_str());
    vec_2d weights(row, vec_1d(col, 0));

    for(int r = 0; r < row; r++) {
        for(int c = 0; c < col; c++) {
            file >> weights[r][c];
        }
    }

    file.close();
    return weights;
}

vec_4d read_weights_4d(const string& file_path, const int num_out_channels, const int num_in_channels, const int kernel_height, const int kernel_width) {
    ifstream file(file_path.c_str());
    vec_4d weights(num_out_channels, vec_3d(num_in_channels, vec_2d(kernel_height, vec_1d(kernel_width, 0))));

    for(int o = 0; o < num_out_channels; o++) {
        for(int i = 0; i < num_in_channels; i++) {
            for(int h = 0; h < kernel_height; h++) {
                for(int w = 0; w < kernel_width; w++) {
                    file >> weights[o][i][h][w];
                }
            }
        }
    }

    file.close();
    return weights;
}

void release_vec_4d(vec_4d& data) {
    for(int o = 0; o < data.size(); o++) {
        for(int i = 0; i < data[o].size(); i++) {
            for(int h = 0; h < data[o][i].size(); h++) {
                data[o][i][h].clear();
            }
            data[o][i].clear();
        }
        data[o].clear();
    }
    data.clear();
}

void release_vec_3d(vec_3d& data) {
    for(int c = 0; c < data.size(); c++) {
        for(int h = 0; h < data[c].size(); h++) {
            data[c][h].clear();
        }
        data[c].clear();
    }
    data.clear();
}

void release_vec_2d(vec_2d& data) {
    for(int r = 0; r < data.size(); r++) {
        data[r].clear();
    }
    data.clear();
}

void release_vec_1d(vec_1d& data) {
    data.clear();
}

#endif
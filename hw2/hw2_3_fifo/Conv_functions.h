#ifndef CONV_FUNCTIONS_H
#define CONV_FUNCTIONS_H

#include <vector>
#include <algorithm>

using namespace std;


typedef vector<float> vec_1d;
typedef vector<vec_1d> vec_2d;
typedef vector<vec_2d> vec_3d;
typedef vector<vec_3d> vec_4d;

vec_3d convolution(const vec_3d&, const vec_4d&, const vec_1d&, const int, const int, const int, const int, const int);
vec_3d relu_3d(const vec_3d&);
vec_1d relu_1d(const vec_1d&);
vec_3d max_pooling_3d(const vec_3d&, const int, const int, const int);
vec_1d flatten(const vec_3d&);
vec_1d fc(const vec_1d&, const vec_2d&, const vec_1d&, const int, const int);
vec_1d softmax(const vec_1d&);
vector<string> read_labels(const string&);
string map_softmax_to_label(const vec_1d&, const vector<string>&);
vector<pair<string, float> > sorting_class(const vec_1d&, const vector<string>&);

vec_3d convolution(const vec_3d& input, const vec_4d& weight, const vec_1d& bias, 
    const int in_channel_num, const int out_channel_num, const int kernel_size, const int stride, const int padding){

    int input_height = input[0].size();
    int input_width = input[0][0].size();
    int output_height = (input_height + 2 * padding - kernel_size) / stride + 1;
    int output_width = (input_width + 2 * padding - kernel_size) / stride + 1;
    
    vec_3d input_padded(in_channel_num, vec_2d(input_height + 2 * padding, vec_1d(input_width + 2 * padding, 0)));
    vec_3d output(out_channel_num, vec_2d(output_height, vec_1d(output_width, 0)));

    for(int c = 0; c < in_channel_num; c++) {
        for(int h = 0; h < input_height; h++) {
            for(int w = 0; w < input_width; w++) {
                input_padded[c][h + padding][w + padding] = input[c][h][w];
            }
        }
    }

    for(int o = 0; o < out_channel_num; o++) {
        for(int h = 0; h < output_height; h++) {
            for(int w = 0; w < output_width; w++) {
                for(int c = 0; c < in_channel_num; c++) {
                    for(int i = 0; i < kernel_size; i++) {
                        for(int j = 0; j < kernel_size; j++) {
                            output[o][h][w] += input_padded[c][h * stride + i][w * stride + j] * weight[o][c][i][j];
                        }
                    }
                }
                output[o][h][w] += bias[o];
            }
        }
    }

    return output;
}

vec_3d relu_3d(const vec_3d& input) {
    vec_3d output(input.size(), vec_2d(input[0].size(), vec_1d(input[0][0].size(), 0)));

    for(int c = 0; c < input.size(); c++) {
        for(int h = 0; h < input[0].size(); h++) {
            for(int w = 0; w < input[0][0].size(); w++) {
                output[c][h][w] = input[c][h][w] > 0 ? input[c][h][w] : 0;
            }
        }
    }

    return output;
}

vec_1d relu_1d(const vec_1d& input) {
    vec_1d output(input.size(), 0);

    for(int i = 0; i < input.size(); i++) {
        output[i] = input[i] > 0 ? input[i] : 0;
    }

    return output;
}

vec_3d max_pooling_3d(const vec_3d& input, const int input_channel_num, const int kernel_size, const int stride) {
    int output_height = (input[0].size() - kernel_size) / stride + 1;
    int output_width = (input[0][0].size() - kernel_size) / stride + 1;

    vec_3d output(input_channel_num, vec_2d(output_height, vec_1d(output_width, 0)));

    for(int c = 0; c < input_channel_num; c++) {
        for(int h = 0; h < output_height; h++) {
            for(int w = 0; w < output_width; w++) {
                float max_val = input[c][h * stride][w * stride];
                for(int i = 0; i < kernel_size; i++) {
                    for(int j = 0; j < kernel_size; j++) {
                        max_val = max(max_val, input[c][h * stride + i][w * stride + j]);
                    }
                }
                output[c][h][w] = max_val;
            }
        }
    }

    return output;
}

vec_1d flatten(const vec_3d& input) {
    vec_1d output;

    size_t size = input.size() * input[0].size() * input[0][0].size();
    output.reserve(size);

    for(int c = 0; c < input.size(); c++) {
        for(int h = 0; h < input[0].size(); h++) {
            for(int w = 0; w < input[0][0].size(); w++) {
                output.push_back(input[c][h][w]);
            }
        }
    }

    return output;
}

vec_1d fc(const vec_1d& input, const vec_2d& weight, const vec_1d& bias, const int input_size, const int output_size) {
    vec_1d output(output_size, 0);

    for(int o = 0; o < output_size; o++) {
        for(int i = 0; i < input_size; i++) {
            output[o] += input[i] * weight[o][i];
        }
        output[o] += bias[o];
    }

    return output;
}

vec_1d softmax(const vec_1d& input) {
    vec_1d output(input.size(), 0);
    float sum = 0.0;

    for(int i = 0; i < input.size(); i++) {
        output[i] = exp(input[i]);
        sum += output[i];
    }

    for(int i = 0; i < input.size(); i++) {
        output[i] /= sum;
    }

    return output;
}

vector<string> read_labels(const string& file_path) {
    ifstream file(file_path.c_str());
    vector<string> labels;
    string label;

    while(getline(file, label)) {
        labels.push_back(label);
    }

    file.close();

    return labels;
}


string map_softmax_to_label(const vec_1d& softmax, const vector<string>& labels) {
    float max_val = 0.0;
    int max_idx = 0;

    for(int i = 0; i < softmax.size(); i++) {
        if(softmax[i] > softmax[max_idx]) {
            max_val = softmax[i];
            max_idx = i;
        }
    }

    return labels[max_idx];
}

struct ComparePairs {
    bool operator()(const pair<string, float>& a, const pair<string, float>& b) const {
        return a.second > b.second;
    }
};

vector<pair<string, float> > sorting_class(const vec_1d& softmax, const vector<string>& labels) {
    vector<pair<string, float> > result;
   
    for(int i = 0; i < softmax.size(); i++) {
        result.push_back(make_pair(labels[i], softmax[i]));
    }

    sort(result.begin(), result.end(), ComparePairs());

    return result;
}

#endif
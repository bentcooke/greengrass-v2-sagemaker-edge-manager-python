/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "sagemaker_edge_agent_client.hh"
#include "shm.hh"

void printUsage() {
    std::cout << "Usage: ./client [command_name]" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "\t ListModels" << std::endl;
    std::cout << "\t LoadModel [model_path] [model_name]" << std::endl;
    std::cout << "\t UnloadModel [model_name]" << std::endl;
    std::cout << "\t DescribeModel [model_name]" << std::endl;
    std::cout << "\t Predict [model_name] [input_bmp_image] [input_name] [w] [h] [c]" << std::endl;
    std::cout << "\t PredictSHM [model_name] [input_bmp_image] [input_name] [w] [h] [c]" << std::endl;
    std::cout << "\t PredictAndCapture [model_name] [input_bmp_image] [input_name] [w] [h] [c]" << std::endl;
    std::cout << "\t PredictSHMAndCapture [model_name] [input_bmp_image] [input_name] [w] [h] [c]" << std::endl;
}

std::vector<float> readImage(const std::string& file) {
    static constexpr size_t HEADER_SIZE = 54;

    std::ifstream bmp(file, std::ios::binary);

    std::array<char, HEADER_SIZE> header;
    bmp.read(header.data(), header.size());

    auto fileSize = *reinterpret_cast<uint32_t*>(&header[2]);
    auto dataOffset = *reinterpret_cast<uint32_t*>(&header[10]);
    auto width = *reinterpret_cast<uint32_t*>(&header[18]);
    auto height = *reinterpret_cast<uint32_t*>(&header[22]);
    auto depth = *reinterpret_cast<uint16_t*>(&header[28]);

    std::vector<char> img;

    constexpr int channels = 3;
    auto dataSize = channels * width * height;

    img.resize(dataSize);
    bmp.read(img.data(), img.size());

    char temp = 0;
    std::vector<float> in_data;
    for (auto i = 0; i < dataSize; i += 3) {
        temp = img[i];
        img[i] = img[i + 2];
        img[i + 2] = temp;
        in_data.push_back(float(img[i] & 0xff));
        in_data.push_back(float(img[i + 1] & 0xff));
        in_data.push_back(float(img[i + 2] & 0xff));
    }
    bmp.close();
    std::cout << "Done reading the image" << std::endl;
    return in_data;
}

long readImageSHM(const std::string& file, int64_t & dataSize) {
    static constexpr size_t HEADER_SIZE = 54;

    std::ifstream bmp(file, std::ios::binary);

    std::array<char, HEADER_SIZE> header;
    bmp.read(header.data(), header.size());

    auto fileSize = *reinterpret_cast<uint32_t*>(&header[2]);
    auto dataOffset = *reinterpret_cast<uint32_t*>(&header[10]);
    auto width = *reinterpret_cast<uint32_t*>(&header[18]);
    auto height = *reinterpret_cast<uint32_t*>(&header[22]);
    auto depth = *reinterpret_cast<uint16_t*>(&header[28]);

    std::vector<char> img;

    constexpr int channels = 3;
    dataSize = channels * width * height;
    img.resize(dataSize);
    bmp.read(img.data(), img.size());

    char temp = 0;
    long segment_id = createSHM(dataSize * sizeof(float));
    auto segment_address = attachSHMWrite(segment_id);
    float*  in_data = (float*)segment_address;;
    for (auto i = 0; i < dataSize; i += 3) {
        temp = img[i];
        img[i] = img[i + 2];
        img[i + 2] = temp;
        in_data[i] = float(img[i] & 0xff);
        in_data[i + 1] = float(img[i + 1] & 0xff);
        in_data[i + 2] = float(img[i + 2] & 0xff);
    }
    bmp.close();
    std::cout << "Done reading the image" << std::endl;
    return segment_id;
}

int loadModelExecutor(SageMakerEdgeAgentClient& agent_client, std::string model_path, std::string model_name) {
    if (!std::filesystem::exists(model_path)) {
        std::cout << "Model path " << model_path << " does not exist" << std::endl;
        return -1;
    }
    model_path = std::filesystem::absolute(model_path);
    return agent_client.LoadModel(model_path, model_name);
}

int predictExecutor(SageMakerEdgeAgentClient& agent_client, int argc, char** argv, bool shm_enabled, bool capture_enabled) {
    std::vector<int64_t> input_shape;
    int64_t input_size;
    int w, h, channels;
    std::string model_name = std::string(argv[2]);
    if (!agent_client.findModel(model_name)) {
        std::cout << "Model " << model_name << " has not been loaded" << std::endl;
        return -1;
    }
    std::string arg = argv[5];
    try {
        std::size_t pos;
        w = std::stoi(arg, &pos);
        if (pos < arg.size()) {
            std::cerr << "Trailing characters after number: " << arg << '\n';
        }
        arg = argv[6];
        pos = 0;
        h = std::stoi(arg, &pos);
        if (pos < arg.size()) {
            std::cerr << "Trailing characters after number: " << arg << '\n';
        }
        arg = argv[7];
        pos = 0;
        channels = std::stoi(arg, &pos);
        if (pos < arg.size()) {
            std::cerr << "Trailing characters after number: " << arg << '\n';
        }
        input_shape.push_back(1);
        input_shape.push_back(channels);
        input_shape.push_back(w);
        input_shape.push_back(h);
        input_size = 1 * channels * w * h;
    } catch (std::invalid_argument const& ex) {
        std::cerr << "Invalid number: " << arg << '\n'; return -1;
    } catch (std::out_of_range const& ex) {
        std::cerr << "Number out of range: " << arg << '\n'; return -1;
    }
    std::string input_name(argv[4]);
    std::vector<int64_t> shape(input_shape);
    std::string file_name(argv[3]);
    AWS::SageMaker::Edge::PredictResponse reply;
    if (shm_enabled) {
        int64_t dataSize;
        long segment_id = readImageSHM(file_name, dataSize);
        if (dataSize != input_size) {
            std::cout << "Image file that was read does not have input_size " << input_size
                      << " shape of " << w << "*" << h << "*" << channels << std::endl;
            return -1;
        }
        agent_client.PredictSHM(model_name, input_name, segment_id, shape, reply, capture_enabled);
    } else {
        std::vector<float> data = readImage(file_name);
        if (data.size() != input_size) {

            std::cout << "Image file that was read does not have input_size " << input_size
                      << " shape of " << w << "*" << h << "*" << channels << std::endl;
            return -1;
        }
        int ret = agent_client.Predict(model_name, input_name, data, shape, reply, capture_enabled);
        if(ret != 0) {
            return ret;
        }
    }
    std::vector<std::vector<float>> outputs;
    agent_client.getOutputTensors(outputs);
    for (int i = 0; i < outputs.size(); i++) {
        std::cout << "Flattened RAW Output Tensor:" << i + 1 << std::endl;
        for (int j = 0; j < outputs[i].size(); j++) {
            std::cout << outputs[i][j] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}

void parseArguments(int& argc, char** argv, SageMakerEdgeAgentClient& agent_client) {
    if (argc < 2) {
        printUsage();
        return;
    }
    std::string command = std::string(argv[1]);
    if (command == "ListModels") {
        agent_client.ListModels();
    } else if (command == "LoadModel") {
        if (argc < 4) {
            std::cout << "Usage:" << std::endl;
            std::cout << "\t ./client LoadModel [model_path] [model_name]" << std::endl;
            return;
        }
        loadModelExecutor(agent_client, argv[2], argv[3]);
    } else if (command == "UnloadModel") {
        if (argc < 3) {
            std::cout << "Usage:" << std::endl;
            std::cout << "\t ./client UnloadModel [model_name]" << std::endl;
            std::cout << "To find what models are already loaded, try ListModels command"
                      << std::endl;
            return;
        }
        std::string model_name = std::string(argv[2]);
        agent_client.UnLoadModel(model_name);
    } else if (command == "DescribeModel") {
        if (argc < 3) {
            std::cout << "Usage:" << std::endl;
            std::cout << "\t ./client DescribeModel [model_name]" << std::endl;
            std::cout << "To find what models are already loaded, try ListModels command"
                      << std::endl;
            return;
        }
        std::string model_name = std::string(argv[2]);
        agent_client.DescribeModel(model_name);
    } else if (command == "Predict" || command == "PredictSHM" || command == "PredictAndCapture" || command == "PredictSHMAndCapture") {
        if (argc < 8) {
            std::cout << "Usage:" << std::endl;
            std::cout << "\t ./client " << command << " [model_name] [input_bmp_image] [input_name] [w] [h] [c]"
                      << std::endl;
            std::cout << "To find what models are already loaded, try ListModels command"
                      << std::endl;
            return;
        }
        bool shm_enabled = (command == "PredictSHM");
        bool capture_enabled = (command == "PredictAndCapture" || command == "PredictSHMAndCapture");
        predictExecutor(agent_client, argc, argv, shm_enabled, capture_enabled);
    } else if(command == "Check") { // for dev purpose only
        if (argc < 5) {
            std::cout << "Usage:" << std::endl;
            std::cout << "\t ./client " << command << " [model_name] [model_path] [image_path]"
                      << std::endl;
            std::cout << "Supported Models: resnet18-v1, ssd-512-mobilenet-voc"
                      << std::endl;
            return;
        }
        std::string model_path_str = std::string(argv[3]);
        char model_path[model_path_str.length() + 1];
        std::strcpy(model_path, model_path_str.c_str());
        std::string imagePathStr = std::string(argv[4]);
        std::string imageSizeStr = "224";
        if(std::string(argv[2]) == "ssd-512-mobilenet-voc") {
            imagePathStr = "../../../distribution/resources/data/ssd_512_mobilenet_voc/000000000139.bmp";
            imageSizeStr = "512";
        } else if(std::string(argv[2]) != "resnet18-v1") {
            std::cout << "Supported Models: resnet18-v1, ssd-512-mobilenet-voc"
                      << std::endl;
            return;
        }
        char image_path[imagePathStr.length() + 1];
        std::strcpy(image_path, imagePathStr.c_str());
        char image_size[imageSizeStr.length() + 1];
        std::strcpy(image_size, imageSizeStr.c_str());
        char* args[9] = {(char *)(""), (char *)("Check"), argv[2],
                         (char *)(image_path), (char *)("data"),
                         (char *)(image_size), (char *)(image_size), (char *)("3"),
                         (char *)(model_path)};
        int ret = 0;
        ret = loadModelExecutor(agent_client, model_path_str, std::string(argv[2]));
        if(ret != 0) {abort();}
        ret = predictExecutor(agent_client, 8, args, false, true);
        if(ret != 0) {abort();}
        ret = predictExecutor(agent_client, 8, args, true, true);
        if(ret != 0) {abort();}
        ret = agent_client.UnLoadModel(std::string(argv[2]));
        if(ret != 0) {abort();}
    } else {
            std::cout << "command " << command << " not valid" << std::endl;
            printUsage();
    }
}


int main(int argc, char** argv) {
    SageMakerEdgeAgentClient agent_client(
        grpc::CreateChannel("unix:///tmp/sagemaker_edge_agent_example.sock", grpc::InsecureChannelCredentials()));
    parseArguments(argc, argv, agent_client);
    return 0;
}

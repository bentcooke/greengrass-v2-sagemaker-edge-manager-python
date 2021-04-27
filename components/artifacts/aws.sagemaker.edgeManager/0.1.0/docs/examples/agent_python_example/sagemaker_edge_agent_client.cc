/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#include <ctime>
#include "shm.hh"
#include "sagemaker_edge_agent_client.hh"

bool SageMakerEdgeAgentClient::findModel(const std::string& model_name) {
    AWS::SageMaker::Edge::ListModelsRequest request;
    AWS::SageMaker::Edge::ListModelsResponse reply;
    grpc::ClientContext context;
    grpc::Status status;
    status = stub_->ListModels(&context, request, &reply);
    if (status.ok()) {
        for (int i = 0; i < reply.models_size(); i++) {
            if (reply.models(i).name().c_str() == model_name) { return true; }
        }
    }
    return false;
}

int SageMakerEdgeAgentClient::DescribeModel(const std::string model_name) {
    AWS::SageMaker::Edge::DescribeModelRequest request;
    AWS::SageMaker::Edge::DescribeModelResponse reply;
    grpc::ClientContext context;
    grpc::Status status;
    try {
        request.set_name(model_name);
        status = stub_->DescribeModel(&context, request, &reply);
        if (status.ok()) {
            std::cout << "Model " << reply.model().name().c_str() << " has been loaded"
                      << std::endl;
            std::cout << "DescribeModel succeeded" << std::endl;
        } else {
            if (!findModel(model_name)) {
                std::cout << "Model " << model_name << " has not been loaded" << std::endl;
            }
            std::cout << "DescribeModel failed" << std::endl;
            return -1;
        }
    } catch (std::exception& e) { std::cout << "Exception: " << e.what() << std::endl; return -1;}
    return 0;
}

int SageMakerEdgeAgentClient::ListModels() {
    AWS::SageMaker::Edge::ListModelsRequest request;
    AWS::SageMaker::Edge::ListModelsResponse reply;
    grpc::ClientContext context;
    grpc::Status status;
    try {
        status = stub_->ListModels(&context, request, &reply);
        if (status.ok()) {
            std::cout << "There are " << reply.models_size() << " models" << std::endl;
            for (int i = 0; i < reply.models_size(); i++) {
                std::cout << "Model " << i << "  " << reply.models(i).name().c_str() << std::endl;
            }
            std::cout << "ListModels succeeded" << std::endl;
        } else {
            std::cout << "ListModels failed" << std::endl;
            return -1;
        }
    } catch (std::exception& e) { std::cout << "Exception: " << e.what() << std::endl; return -1;}
    return 0;
}

int SageMakerEdgeAgentClient::Predict(const std::string model_name, const std::string input_name,
                             std::vector<float>& data,
                             std::vector<int64_t> shape,
                             AWS::SageMaker::Edge::PredictResponse& reply,
                             bool capture_enabled) {
    if (!findModel(model_name)) {
        std::cout << "Model " << model_name << " has not been loaded" << std::endl;
        return -1;
    }
    AWS::SageMaker::Edge::PredictRequest request;
    grpc::ClientContext context;
    grpc::Status status;
    AWS::SageMaker::Edge::CaptureDataRequest capture_data_request;
    try {
        request.set_name(model_name);
        capture_data_request.set_model_name(model_name);
        AWS::SageMaker::Edge::Tensor t;
        AWS::SageMaker::Edge::TensorMetadata meta;

        meta.set_name(input_name);
        for (int i = 0; i < shape.size(); ++i) {
            meta.add_shape(shape[i]);
        }
        meta.set_data_type(AWS::SageMaker::Edge::DataType::FLOAT32);
        t.mutable_tensor_metadata()->CopyFrom(meta);
        // TODO cast to appropriate type
        char* cs = reinterpret_cast<char*>(data.data());
        int size = data.size() * sizeof(float);
        std::string sp(cs, size);
        t.set_byte_data(sp);
        request.add_tensors();
        request.mutable_tensors(0)->CopyFrom(t);
        status = stub_->Predict(&context, request, &reply);
        if (status.ok()) {
            std::cout << "Predict succeeded" << std::endl;
        } else {
            std::cout << "Predict failed" << std::endl;
            return -1;
        }
        // prepare CaptureDataRequest
        capture_data_request.set_model_name(model_name);
        std::string capture_id = "capture" + std::to_string(std::time(0));
        capture_data_request.set_capture_id(capture_id);
        // set input
        AWS::SageMaker::Edge::Tensor* input_tensor = capture_data_request.add_input_tensors();
        input_tensor->CopyFrom(t);
        for (int i = 0; i < reply.tensors_size(); ++i) {
            AWS::SageMaker::Edge::Tensor* output_tensor  = capture_data_request.add_output_tensors();
            output_tensor->CopyFrom(reply.tensors(i));
        }
        if(capture_enabled) {
            this->CaptureData(capture_data_request);
        }
    } catch (std::exception& e) { std::cout << "Exception: " << e.what() << std::endl; return -1;}

    SageMakerEdgeAgentClient::setInternalOutput(reply);
    return 0;
}

int SageMakerEdgeAgentClient::PredictSHM(const std::string model_name, const std::string input_name,
                             long& segment_id,
                             std::vector<int64_t> shape,
                             AWS::SageMaker::Edge::PredictResponse& reply,
                             bool capture_enabled) {
    if (!findModel(model_name)) {
        std::cout << "Model " << model_name << " has not been loaded" << std::endl;
        return -1;
    }
    AWS::SageMaker::Edge::PredictRequest request;
    grpc::ClientContext context;
    grpc::Status status;
    AWS::SageMaker::Edge::CaptureDataRequest capture_data_request;
    try {
        request.set_name(model_name);

        AWS::SageMaker::Edge::Tensor t;
        AWS::SageMaker::Edge::TensorMetadata meta;

        meta.set_name(input_name);
        for (int i = 0; i < shape.size(); ++i) {
            meta.add_shape(shape[i]);
        }
        auto shared_memory_handle = t.mutable_shared_memory_handle();
        shared_memory_handle->set_offset(0);
        makeSHMReadOnly(segment_id);
        shared_memory_handle->set_segment_id(segment_id);

        meta.set_data_type(AWS::SageMaker::Edge::DataType::FLOAT32);
        t.mutable_tensor_metadata()->CopyFrom(meta);
        request.add_tensors();
        request.mutable_tensors(0)->CopyFrom(t);

        status = stub_->Predict(&context, request, &reply);
        if (status.ok()) {
            std::cout << "Predict succeeded" << std::endl;
        } else {
            std::cout << "Predict failed" << std::endl;
            return -1;
        }
        // prepare CaptureDataRequest
        capture_data_request.set_model_name(model_name);
        std::string capture_id = "captureshm" + std::to_string(std::time(0));
        capture_data_request.set_capture_id(capture_id);
        // set input
        AWS::SageMaker::Edge::Tensor* input_tensor = capture_data_request.add_input_tensors();
        input_tensor->CopyFrom(t);
        for (int i = 0; i < reply.tensors_size(); ++i) {
            AWS::SageMaker::Edge::Tensor* output_tensor  = capture_data_request.add_output_tensors();
            output_tensor->CopyFrom(reply.tensors(i));
        }
        if(capture_enabled) {
            this->CaptureData(capture_data_request);
        }
    } catch (std::exception& e) { std::cout << "Exception: " << e.what() << std::endl; return -1;}

    SageMakerEdgeAgentClient::setInternalOutput(reply);
    return 0;
}

void SageMakerEdgeAgentClient::getOutputTensors(std::vector<std::vector<float>>& outputs) {
    AWS::SageMaker::Edge::PredictResponse reply = SageMakerEdgeAgentClient::getInternalOutput();
    outputs.resize(reply.tensors_size());
    for (int i = 0; i < reply.tensors_size(); i++) {
        const AWS::SageMaker::Edge::Tensor& t = reply.tensors(i);
        const std::string buff = t.byte_data();
        AWS::SageMaker::Edge::TensorMetadata tensor_meta = t.tensor_metadata();
        int shape_size = tensor_meta.shape_size();
        int size = 1;
        for (int j = 0; j < shape_size; j++) {
            size *= tensor_meta.shape(j);
        }
        const float* values = reinterpret_cast<const float*>(buff.data());
        outputs[i].resize(size);
        std::memcpy(outputs[i].data(), values, sizeof(float) * size);
    }
}

int SageMakerEdgeAgentClient::UnLoadModel(const std::string model_name) {
    AWS::SageMaker::Edge::UnLoadModelRequest request;
    AWS::SageMaker::Edge::UnLoadModelResponse reply;
    grpc::ClientContext context;
    grpc::Status status;

    try {
        request.set_name(model_name);
        status = stub_->UnLoadModel(&context, request, &reply);
        if (status.ok()) {
            std::cout << "Model " << model_name << " has been unloaded" << std::endl;
            std::cout << "UnLoadModel succeeded" << std::endl;
        } else {
            if (!findModel(model_name)) {
                std::cout << "Model " << model_name << " has not been loaded" << std::endl;
            }
            std::cout << "UnLoadModel failed" << std::endl;
            return -1;
        }
    } catch (std::exception& e) { std::cout << "Exception: " << e.what() << std::endl; return -1;}
    return 0;
}

int SageMakerEdgeAgentClient::LoadModel(const std::string model_url, const std::string model_name) {
    AWS::SageMaker::Edge::LoadModelRequest request;
    AWS::SageMaker::Edge::LoadModelResponse reply;
    grpc::ClientContext context;
    grpc::Status status;
    try {
        request.set_url(model_url);
        request.set_name(model_name);
        status = stub_->LoadModel(&context, request, &reply);
        if (status.ok()) {
            std::cout << "Model " << model_name << " located at " << model_url << " loaded"
                      << std::endl;
            std::cout << "LoadModel succeeded" << std::endl;
        } else {
            if (findModel(model_name)) {
                std::cout << "Alias " << model_name << " has been used" << std::endl;
            }
            std::cout << "LoadModel failed" << std::endl;
            return -1;
        }
    } catch (std::exception& e) { std::cout << "Exception: " << e.what() << std::endl; return -1;}
    return 0;
}

int SageMakerEdgeAgentClient::CaptureData(AWS::SageMaker::Edge::CaptureDataRequest &request) {
    AWS::SageMaker::Edge::CaptureDataResponse reply;
    grpc::ClientContext context;
    grpc::Status status;
    try {
        status = stub_->CaptureData(&context, request, &reply);
        if (status.ok()) {
            std::cout << "Capture Data succeeded" << std::endl;
        } else {
            std::cout << "Capture Data failed" << std::endl;
            return -1;
        }
    } catch (std::exception& e) { std::cout << "Exception: " << e.what() << std::endl; return -1;}
    return 0;
}

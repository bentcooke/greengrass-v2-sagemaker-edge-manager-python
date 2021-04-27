/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#pragma once

#include <grpcpp/grpcpp.h>
#include <vector>

#include "sagemaker_edge_agent.grpc.pb.h"


class SageMakerEdgeAgentClient {
public:

    int ListModels();

    int LoadModel(const std::string model_url, const std::string model_name);

    int UnLoadModel(const std::string model_name);

    int Predict(const std::string model_name, const std::string input_name,
                 std::vector<float>& data, std::vector<int64_t> shape,
                 AWS::SageMaker::Edge::PredictResponse& reply,
                 bool capture_enabled);

    int PredictSHM(const std::string model_name, const std::string input_name,
                 long& segment_id, std::vector<int64_t> shape,
                 AWS::SageMaker::Edge::PredictResponse& reply,
                 bool capture_enabled);

    int DescribeModel(const std::string model_name);

    void GetModelOutput(const std::string model_name);

    SageMakerEdgeAgentClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(AWS::SageMaker::Edge::Agent::NewStub(channel)) {}
    bool findModel(const std::string& model_name);

    void getOutputTensors(std::vector<std::vector<float>>& outputs);


private:
    std::unique_ptr<AWS::SageMaker::Edge::Agent::Stub> stub_;
    AWS::SageMaker::Edge::PredictResponse output_reply;
    void setInternalOutput(AWS::SageMaker::Edge::PredictResponse reply) { output_reply = reply; }
    int CaptureData(AWS::SageMaker::Edge::CaptureDataRequest &request);
    AWS::SageMaker::Edge::PredictResponse getInternalOutput() { return output_reply; };
};


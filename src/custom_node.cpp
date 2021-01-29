//*****************************************************************************
// Copyright 2020 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//*****************************************************************************
#include "custom_node.hpp"

#include <utility>

#include "custom_node_output_allocator.hpp"
#include "customnodesession.hpp"
#include "logging.hpp"
#include "node_library.hpp"

namespace ovms {

CustomNode::CustomNode(
    const std::string& nodeName,
    const NodeLibrary& library,
    const parameters_t& parameters,
    const std::unordered_map<std::string, std::string>& nodeOutputNameAlias) :
    Node(nodeName),
    library(library),
    parameters(parameters),
    nodeOutputNameAlias(nodeOutputNameAlias) {
    if (parameters.size() > 0) {
        this->_parameters = std::make_unique<struct CustomNodeParam[]>(parameters.size());

        int i = 0;
        for (const auto& [key, value] : parameters) {
            this->_parameters[i].key = key.c_str();
            this->_parameters[i].value = value.c_str();
            i++;
        }
    }
}

Status CustomNode::execute(session_key_t sessionKey, PipelineEventQueue& notifyEndQueue) {
    auto& nodeSession = getNodeSession(sessionKey);
    auto& customNodeSession = static_cast<CustomNodeSession&>(nodeSession);
    return customNodeSession.execute(notifyEndQueue, *this, this->library, this->_parameters, this->parameters.size());
}

Status CustomNode::fetchResults(NodeSession& nodeSession, SessionResults& nodeSessionOutputs) {
    auto& customNodeSession = static_cast<CustomNodeSession&>(nodeSession);
    const auto& sessionMetadata = nodeSession.getNodeSessionMetadata();
    SessionResult sessionResults{sessionMetadata, {}};
    auto it = nodeSessionOutputs.emplace(sessionMetadata.getSessionKey(), std::move(sessionResults));
    if (!it.second) {
        SPDLOG_LOGGER_ERROR(dag_executor_logger, "Failed to put node: {} session: {} results in node session outputs",
            getName(), nodeSession.getSessionKey());
        return StatusCode::INTERNAL_ERROR;
    }
    auto& metadataBlobResultsPair = it.first->second;
    auto& blobResults = metadataBlobResultsPair.second;
    return this->fetchResults(
        blobResults,
        customNodeSession.getOutputTensors(),
        customNodeSession.getOutputTensorsLength(),
        nodeSession.getSessionKey());
}

Status CustomNode::fetchResults(BlobMap& outputs, struct CustomNodeTensor* outputTensors, int outputsTensorsLength, session_key_t sessionKey) {
    static_cast<CustomNodeSession&>(this->getNodeSession(sessionKey)).clearInputs();

    for (const auto& node : this->next) {
        for (const auto& pair : node.get().getMappingByDependency(*this)) {
            const auto& output_name = pair.first;
            if (outputs.count(output_name) == 1) {
                continue;
            }
            const auto& realOutputName = this->getRealOutputName(output_name);
            SPDLOG_LOGGER_DEBUG(dag_executor_logger, "Node: {} session: {} Getting custom node output tensor with name: {}",
                getName(), sessionKey, realOutputName);

            bool outputFound = false;
            for (int i = 0; i < outputsTensorsLength; i++) {
                if (std::strcmp(outputTensors[i].name, realOutputName.c_str()) == 0) {
                    outputFound = true;
                    InferenceEngine::TensorDesc desc;
                    desc.setPrecision(InferenceEngine::Precision::FP32);  // TODO hardcoded for now
                    desc.setDims({1, 10});                                // TODO hardcoded for now
                    InferenceEngine::Blob::Ptr resultBlob = InferenceEngine::make_shared_blob<float>(
                        desc,
                        std::make_shared<CustomNodeOutputAllocator>(
                            outputTensors[i],
                            this->library));
                    resultBlob->allocate();
                    outputs.emplace(std::make_pair(output_name, std::move(resultBlob)));
                    SPDLOG_LOGGER_DEBUG(dag_executor_logger, "Node: {} session: {} Blob with name {} has been prepared under alias {}",
                        getName(), sessionKey, realOutputName, output_name);
                    break;
                }
            }

            if (!outputFound) {
                SPDLOG_LOGGER_ERROR(dag_executor_logger, "Node: {} session: {} Custom node output with name {} is missing",
                    getName(), sessionKey, realOutputName);
                return StatusCode::NODE_LIBRARY_MISSING_OUTPUT;
            }
        }
    }

    this->getNodeSession(sessionKey).release();
    return StatusCode::OK;
}

std::unique_ptr<NodeSession> CustomNode::createNodeSession(const NodeSessionMetadata& metadata) {
    return std::make_unique<CustomNodeSession>(metadata, getName(), previous.size(), this->library);
}

}  // namespace ovms

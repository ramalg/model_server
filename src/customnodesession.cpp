//*****************************************************************************
// Copyright 2021 Intel Corporation
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
#include "customnodesession.hpp"

#include <cstdint>
#include <utility>

#include "logging.hpp"
#include "node.hpp"
#include "node_library.hpp"
#include "nodeinputhandler.hpp"
#include "pipelineeventqueue.hpp"

namespace ovms {

CustomNodeSession::CustomNodeSession(const NodeSessionMetadata& metadata, const std::string& nodeName, uint32_t inputsCount, const NodeLibrary& library) :
    NodeSession(metadata, nodeName, inputsCount),
    library(library) {}

CustomNodeSession::CustomNodeSession(const NodeSessionMetadata&& metadata, const std::string& nodeName, uint32_t inputsCount, const NodeLibrary& library) :
    NodeSession(std::move(metadata), nodeName, inputsCount),
    library(library) {}

CustomNodeSession::~CustomNodeSession() = default;

Status CustomNodeSession::execute(PipelineEventQueue& notifyEndQueue, Node& node, const NodeLibrary& library, std::unique_ptr<struct CustomNodeParam[]>& parameters, int parametersLength) {
    const auto& blobMap = this->inputHandler->getInputs();
    auto inputTensorsLength = blobMap.size();
    auto inputTensors = std::make_unique<struct CustomNodeTensor[]>(inputTensorsLength);

    int i = 0;
    for (const auto& [name, blob] : blobMap) {
        inputTensors[i].name = static_cast<const char*>(name.c_str());
        inputTensors[i].data = static_cast<uint8_t*>(blob->buffer());
        inputTensors[i].dataLength = static_cast<uint64_t>(blob->byteSize());
        inputTensors[i].dims = static_cast<uint64_t*>(blob->getTensorDesc().getDims().data());
        inputTensors[i].dimsLength = static_cast<uint64_t>(blob->getTensorDesc().getDims().size());
        inputTensors[i].precision = 0;  // TODO
        i++;
    }

    int result = library.execute(inputTensors.get(), inputTensorsLength, &this->outputTensors, &this->outputTensorsLength, parameters.get(), parametersLength);
    if (result != 0) {
        SPDLOG_LOGGER_ERROR(dag_executor_logger, "Node {}; session: {}; has failed custom node execution with return code: {}", getName(), getSessionKey(), result);
        notifyEndQueue.push({node, getSessionKey()});
        return StatusCode::NODE_LIBRARY_EXECUTION_FAILED;
    }

    notifyEndQueue.push({node, getSessionKey()});
    return StatusCode::OK;
}

void CustomNodeSession::clearInputs() {
    this->inputHandler->clearInputs();
}

void CustomNodeSession::release() {
    this->library.releaseTensors(this->getOutputTensors());
}

}  // namespace ovms

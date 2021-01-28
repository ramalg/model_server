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

#include <utility>

#include "node.hpp"
#include "pipelineeventqueue.hpp"

namespace ovms {

CustomNodeSession::CustomNodeSession(const NodeSessionMetadata& metadata, const std::string& nodeName, uint32_t inputsCount) :
    NodeSession(metadata, nodeName, inputsCount) {}

CustomNodeSession::CustomNodeSession(const NodeSessionMetadata&& metadata, const std::string& nodeName, uint32_t inputsCount) :
    NodeSession(std::move(metadata), nodeName, inputsCount) {}

CustomNodeSession::~CustomNodeSession() = default;

Status CustomNodeSession::execute(PipelineEventQueue& notifyEndQueue, Node& node) {
    notifyEndQueue.push({node, getSessionKey()});
    return StatusCode::UNKNOWN_ERROR;
}

}  // namespace ovms

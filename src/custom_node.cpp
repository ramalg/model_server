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
    return customNodeSession.execute(notifyEndQueue, *this);

    // int i;
    // this->library.execute(nullptr, 0, nullptr, &i, this->_parameters.get(), this->parameters.size());
    // notifyEndQueue.push({*this, sessionKey});
    // return StatusCode::UNKNOWN_ERROR;
}

Status CustomNode::fetchResults(NodeSession& nodeSession, SessionResults& nodeSessionOutputs) {
    return StatusCode::UNKNOWN_ERROR;
}

std::unique_ptr<NodeSession> CustomNode::createNodeSession(const NodeSessionMetadata& metadata) {
    return std::make_unique<CustomNodeSession>(metadata, getName(), previous.size());
}

}  // namespace ovms

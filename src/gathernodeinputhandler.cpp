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
#include "gathernodeinputhandler.hpp"

#include "logging.hpp"

namespace ovms {

GatherNodeInputHandler::GatherNodeInputHandler(uint32_t inputsMissingCount, session_id_t shardsCount) :
    NodeInputHandler(inputsMissingCount) {
    expectedDependencies *= shardsCount;
}

void GatherNodeInputHandler::setInput(const std::string& inputName, InferenceEngine::Blob::Ptr& ptr, session_id_t shardId) {
    // TODO check against shardsCount & TEST
    auto it = shardsStorage.find(inputName);
    if (it == shardsStorage.end()) {
        shard_map_t shardMap{{shardId, ptr}};
        auto itDidInsertPair = shardsStorage.emplace(inputName, std::move(shardMap));  // TODO error check
        it = itDidInsertPair.first;
        if (!itDidInsertPair.second) {
            throw std::runtime_error("Tried to insert the same input twice with the same shard id");  // TODO recoverable error
        }
    } else {
        it->second.emplace(shardId, ptr);
    }
}

void GatherNodeInputHandler::notifyFinishedDependency() {
    NodeInputHandler::notifyFinishedDependency();
    if (expectedDependencies > 0) {
        return;
    }
    for (auto& [inputName, shardMap] : shardsStorage) {
        const auto shardsCount = shardMap.size();
        SPDLOG_LOGGER_DEBUG(dag_executor_logger, "Consolidating: {} shards for input: {}", shardsCount, inputName);
        auto tensorDesc = shardMap[0]->getTensorDesc();
        InferenceEngine::SizeVector newDims{1};
        for (auto dim : tensorDesc.getDims()) {
            newDims.emplace_back(dim);
        }
        newDims[1] = shardsCount;
        const InferenceEngine::TensorDesc consolidatedBlobDesc(
            tensorDesc.getPrecision(),
            newDims,
            InferenceEngine::Layout::ANY);
        auto consolidatedBlob = InferenceEngine::make_shared_blob<float>(consolidatedBlobDesc);
        consolidatedBlob->allocate();
        for (auto& [shardId, blob] : shardMap) {
            const auto memstep = sizeof(float) * blob->size();
            size_t offset = shardId * memstep;
            memcpy((char*)consolidatedBlob->buffer() + offset, blob->cbuffer(), memstep);  // TODO dispose ugly cast
        }
        inputBlobs.insert({inputName, consolidatedBlob});
    }
}
}  // namespace ovms

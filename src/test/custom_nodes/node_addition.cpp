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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "../../custom_node_interface.hpp"

extern "C" int execute(const struct CustomNodeTensor* inputs, int inputsLength, struct CustomNodeTensor** outputs, int* outputsLength, const struct CustomNodeParam* params, int paramsLength) {
    float addValue1 = 0.0f;
    float addValue2 = 0.0f;

    for (int i = 0; i < paramsLength; i++) {
        if (strcmp(params[i].key, "add_value_1") == 0) {
            addValue1 = atof(params[i].value);
        }
        if (strcmp(params[i].key, "add_value_2") == 0) {
            addValue2 = atof(params[i].value);
        }
    }

    std::cout << "CUSTOM ADDITION NODE => Parameters passed: add_value_1:" << addValue1 << "; add_value_2:" << addValue2 << std::endl;

    *outputsLength = 0;
    return 0;
}

extern "C" int releaseBuffer(struct CustomNodeTensor* output) {
    return 0;
}

extern "C" int releaseTensors(struct CustomNodeTensor* outputs) {
    return 0;
}

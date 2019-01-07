//
// Copyright 2018 WebAssembly Community Group participants
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
//

#include "wasp/binary/section.h"

namespace wasp {
namespace binary {

#define WASP_OPERATOR_EQ_NE_1(Name, f1)               \
  bool operator==(const Name& lhs, const Name& rhs) { \
    return lhs.f1 == rhs.f1;                          \
  }                                                   \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

#define WASP_OPERATOR_EQ_NE_2(Name, f1, f2)           \
  bool operator==(const Name& lhs, const Name& rhs) { \
    return lhs.f1 == rhs.f1 && lhs.f2 == rhs.f2;      \
  }                                                   \
  bool operator!=(const Name& lhs, const Name& rhs) { return !(lhs == rhs); }

WASP_OPERATOR_EQ_NE_2(KnownSection, id, data)
WASP_OPERATOR_EQ_NE_2(CustomSection, name, data)
WASP_OPERATOR_EQ_NE_1(Section, contents)

#undef WASP_OPERATOR_EQ_NE_1
#undef WASP_OPERATOR_EQ_NE_2

}  // namespace binary
}  // namespace wasp
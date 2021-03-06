//
// Copyright 2019 WebAssembly Community Group participants
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

#ifndef WASP_BINARY_READ_READ_VECTOR_H_
#define WASP_BINARY_READ_READ_VECTOR_H_

#include <vector>

#include "wasp/base/features.h"
#include "wasp/base/optional.h"
#include "wasp/base/span.h"
#include "wasp/base/string_view.h"
#include "wasp/base/types.h"
#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_count.h"

namespace wasp {
namespace binary {

template <typename T>
optional<std::vector<T>> ReadVector(SpanU8* data,
                                    const Features& features,
                                    Errors& errors,
                                    string_view desc) {
  ErrorsContextGuard guard{errors, *data, desc};
  std::vector<T> result;
  WASP_TRY_READ(len, ReadCount(data, features, errors));
  result.reserve(len);
  for (u32 i = 0; i < len; ++i) {
    WASP_TRY_READ(elt, Read<T>(data, features, errors));
    result.emplace_back(std::move(elt));
  }
  return result;
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_VECTOR_H_

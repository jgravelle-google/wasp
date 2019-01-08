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

#ifndef WASP_BINARY_READ_READ_INDIRECT_NAME_ASSOC_H_
#define WASP_BINARY_READ_READ_INDIRECT_NAME_ASSOC_H_

#include "wasp/binary/name_section.h"  /// XXX

#include "wasp/binary/errors_context_guard.h"
#include "wasp/binary/read/macros.h"
#include "wasp/binary/read/read.h"
#include "wasp/binary/read/read_index.h"
#include "wasp/binary/read/read_name_assoc.h"
#include "wasp/binary/read/read_vector.h"

namespace wasp {
namespace binary {

template <typename Errors>
optional<IndirectNameAssoc> Read(SpanU8* data,
                                 Errors& errors,
                                 Tag<IndirectNameAssoc>) {
  ErrorsContextGuard<Errors> guard{errors, *data, "indirect name assoc"};
  WASP_TRY_READ(index, ReadIndex(data, errors, "index"));
  WASP_TRY_READ(name_map, ReadVector<NameAssoc>(data, errors, "name map"));
  return IndirectNameAssoc{index, std::move(name_map)};
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_READ_READ_INDIRECT_NAME_ASSOC_H_
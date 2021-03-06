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

#include "wasp/binary/symbol_info.h"

#include <cassert>

#include "src/base/operator_eq_ne_macros.h"

namespace wasp {
namespace binary {

SymbolInfo::SymbolInfo(Flags flags, const Base& base)
    : flags{flags}, desc{base} {
  assert(base.kind == SymbolInfoKind::Function ||
         base.kind == SymbolInfoKind::Global ||
         base.kind == SymbolInfoKind::Event);
}

SymbolInfo::SymbolInfo(Flags flags, const Data& data)
    : flags{flags}, desc{data} {}

SymbolInfo::SymbolInfo(Flags flags, const Section& section)
    : flags{flags}, desc{section} {}

WASP_OPERATOR_EQ_NE_2(SymbolInfo, flags, desc)
WASP_OPERATOR_EQ_NE_4(SymbolInfo::Flags,
                      binding,
                      visibility,
                      undefined,
                      explicit_name)
WASP_OPERATOR_EQ_NE_3(SymbolInfo::Base, kind, index, name)
WASP_OPERATOR_EQ_NE_2(SymbolInfo::Data, name, defined)
WASP_OPERATOR_EQ_NE_3(SymbolInfo::Data::Defined, index, offset, size)
WASP_OPERATOR_EQ_NE_1(SymbolInfo::Section, section)

}  // namespace binary
}  // namespace wasp

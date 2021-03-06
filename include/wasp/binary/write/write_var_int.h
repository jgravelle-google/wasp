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

#ifndef WASP_BINARY_WRITE_WRITE_VAR_INT_H_
#define WASP_BINARY_WRITE_WRITE_VAR_INT_H_

#include <type_traits>

#include "wasp/base/types.h"
#include "wasp/binary/var_int.h"
#include "wasp/binary/write/write_u8.h"

namespace wasp {
namespace binary {

template <typename T, typename Iterator, typename Cond>
Iterator WriteVarIntLoop(T value, Iterator out, Cond&& end_cond) {
  do {
    const u8 byte = value & VarInt<T>::kByteMask;
    value >>= 7;
    if (end_cond(value, byte)) {
      out = Write(byte, out);
      break;
    } else {
      out = Write(byte | VarInt<T>::kExtendBit, out);
    }
  } while (true);
  return out;
}

// Unsigned integers.
template <typename T, typename Iterator>
typename std::enable_if<!std::is_signed<T>::value, Iterator>::type WriteVarInt(
    T value,
    Iterator out) {
  return WriteVarIntLoop(value, out,
                         [](T value, u8 byte) { return value == 0; });
}

// Signed integers.
template <typename T, typename Iterator>
typename std::enable_if<std::is_signed<T>::value, Iterator>::type WriteVarInt(
    T value,
    Iterator out) {
  if (value < 0) {
    return WriteVarIntLoop(value, out, [](T value, u8 byte) {
      return value == -1 && (byte & VarInt<T>::kSignBit) != 0;
    });
  } else {
    return WriteVarIntLoop(value, out, [](T value, u8 byte) {
      return value == 0 && (byte & VarInt<T>::kSignBit) == 0;
    });
  }
}

}  // namespace binary
}  // namespace wasp

#endif  // WASP_BINARY_WRITE_WRITE_VAR_INT_H_

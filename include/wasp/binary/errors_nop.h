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

#ifndef WASP_BINARY_ERRORS_NOP_H_
#define WASP_BINARY_ERRORS_NOP_H_

#include "wasp/binary/errors.h"

namespace wasp {
namespace binary {

class ErrorsNop : public Errors {
 protected:
  void HandlePushContext(SpanU8 pos, string_view desc) override {}
  void HandlePopContext() override {}
  void HandleOnError(SpanU8 pos, string_view message) override {}
};

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_ERRORS_NOP_H_

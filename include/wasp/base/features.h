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

#ifndef WASP_BASE_FEATURES_H_
#define WASP_BASE_FEATURES_H_

namespace wasp {

class Features {
 public:
  void EnableAll();

#define WASP_V(variable, flag, default_)                          \
  bool variable##_enabled() const { return variable##_enabled_; } \
  void enable_##variable() { variable##_enabled_ = true; }        \
  void disable_##variable() { variable##_enabled_ = false; }      \
  void set_##variable##_enabled(bool value) { variable##_enabled_ = value; }
#include "wasp/base/features.def"
#undef WASP_V

 private:
#define WASP_V(variable, flag, default_) bool variable##_enabled_ = default_;
#include "wasp/base/features.def"
#undef WASP_V
};

}  // namespace wasp

#endif  // WASP_BASE_FEATURES_H_

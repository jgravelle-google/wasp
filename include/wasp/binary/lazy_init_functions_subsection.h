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

#ifndef WASP_BINARY_LAZY_INIT_FUNCTIONS_SUBSECTION_H_
#define WASP_BINARY_LAZY_INIT_FUNCTIONS_SUBSECTION_H_

#include "wasp/base/span.h"
#include "wasp/binary/init_function.h"
#include "wasp/binary/lazy_section.h"
#include "wasp/binary/linking_subsection.h"
#include "wasp/binary/read/read_init_function.h"

namespace wasp {

class Features;

namespace binary {

class Errors;

using LazyInitFunctionsSubsection = LazySection<InitFunction>;

LazyInitFunctionsSubsection ReadInitFunctionsSubsection(SpanU8,
                                                        const Features&,
                                                        Errors&);

LazyInitFunctionsSubsection ReadInitFunctionsSubsection(LinkingSubsection,
                                                        const Features&,
                                                        Errors&);

}  // namespace binary
}  // namespace wasp

#endif // WASP_BINARY_LAZY_INIT_FUNCTIONS_SUBSECTION_H_

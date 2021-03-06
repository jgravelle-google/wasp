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

#include "wasp/binary/read/read_section.h"
#include "wasp/base/format.h"
#include "wasp/binary/data_count_section.h"
#include "wasp/binary/errors.h"
#include "wasp/binary/formatters.h"
#include "wasp/binary/lazy_code_section.h"
#include "wasp/binary/lazy_data_section.h"
#include "wasp/binary/lazy_element_section.h"
#include "wasp/binary/lazy_export_section.h"
#include "wasp/binary/lazy_function_section.h"
#include "wasp/binary/lazy_global_section.h"
#include "wasp/binary/lazy_import_section.h"
#include "wasp/binary/lazy_memory_section.h"
#include "wasp/binary/lazy_name_section.h"
#include "wasp/binary/lazy_table_section.h"
#include "wasp/binary/lazy_type_section.h"
#include "wasp/binary/linking_section.h"
#include "wasp/binary/read/read_count.h"
#include "wasp/binary/read/read_data_count.h"
#include "wasp/binary/read/read_u32.h"
#include "wasp/binary/relocation_section.h"

namespace wasp {
namespace binary {

LazyCodeSection ReadCodeSection(SpanU8 data,
                                const Features& features,
                                Errors& errors) {
  return LazyCodeSection{data, features, errors};
}

LazyCodeSection ReadCodeSection(KnownSection sec,
                                const Features& features,
                                Errors& errors) {
  return ReadCodeSection(sec.data, features, errors);
}

LazyDataSection ReadDataSection(SpanU8 data,
                                const Features& features,
                                Errors& errors) {
  return LazyDataSection{data, features, errors};
}

LazyDataSection ReadDataSection(KnownSection sec,
                                const Features& features,
                                Errors& errors) {
  return ReadDataSection(sec.data, features, errors);
}

DataCountSection ReadDataCountSection(SpanU8 data,
                                      const Features& features,
                                      Errors& errors) {
  SpanU8 copy = data;
  return Read<DataCount>(&copy, features, errors);
}

DataCountSection ReadDataCountSection(KnownSection sec,
                                      const Features& features,
                                      Errors& errors) {
  return ReadDataCountSection(sec.data, features, errors);
}

LazyElementSection ReadElementSection(SpanU8 data,
                                      const Features& features,
                                      Errors& errors) {
  return LazyElementSection{data, features, errors};
}

LazyElementSection ReadElementSection(KnownSection sec,
                                      const Features& features,
                                      Errors& errors) {
  return ReadElementSection(sec.data, features, errors);
}

LazyExportSection ReadExportSection(SpanU8 data,
                                    const Features& features,
                                    Errors& errors) {
  return LazyExportSection{data, features, errors};
}

LazyExportSection ReadExportSection(KnownSection sec,
                                    const Features& features,
                                    Errors& errors) {
  return ReadExportSection(sec.data, features, errors);
}

LazyFunctionSection ReadFunctionSection(SpanU8 data,
                                        const Features& features,
                                        Errors& errors) {
  return LazyFunctionSection{data, features, errors};
}

LazyFunctionSection ReadFunctionSection(KnownSection sec,
                                        const Features& features,
                                        Errors& errors) {
  return ReadFunctionSection(sec.data, features, errors);
}

LazyGlobalSection ReadGlobalSection(SpanU8 data,
                                    const Features& features,
                                    Errors& errors) {
  return LazyGlobalSection{data, features, errors};
}

LazyGlobalSection ReadGlobalSection(KnownSection sec,
                                    const Features& features,
                                    Errors& errors) {
  return ReadGlobalSection(sec.data, features, errors);
}

LazyImportSection ReadImportSection(SpanU8 data,
                                    const Features& features,
                                    Errors& errors) {
  return LazyImportSection{data, features, errors};
}

LazyImportSection ReadImportSection(KnownSection sec,
                                    const Features& features,
                                    Errors& errors) {
  return ReadImportSection(sec.data, features, errors);
}

LinkingSection::LinkingSection(SpanU8 data,
                               const Features& features,
                               Errors& errors)
    : data{data},
      version{Read<u32>(&data, features, errors)},
      subsections{data, features, errors} {
  constexpr u32 kVersion = 2;
  if (version && version != kVersion) {
    errors.OnError(data, format("Expected linking section version: {}, got {}",
                                kVersion, *version));
  }
}

LinkingSection ReadLinkingSection(SpanU8 data,
                                  const Features& features,
                                  Errors& errors) {
  return LinkingSection{data, features, errors};
}

LinkingSection ReadLinkingSection(CustomSection sec,
                                  const Features& features,
                                  Errors& errors) {
  return LinkingSection{sec.data, features, errors};
}

LazyMemorySection ReadMemorySection(SpanU8 data,
                                    const Features& features,
                                    Errors& errors) {
  return LazyMemorySection{data, features, errors};
}

LazyMemorySection ReadMemorySection(KnownSection sec,
                                    const Features& features,
                                    Errors& errors) {
  return ReadMemorySection(sec.data, features, errors);
}

LazyNameSection ReadNameSection(SpanU8 data,
                                const Features& features,
                                Errors& errors) {
  return LazyNameSection{data, features, errors};
}

LazyNameSection ReadNameSection(CustomSection sec,
                                const Features& features,
                                Errors& errors) {
  return LazyNameSection{sec.data, features, errors};
}

RelocationSection::RelocationSection(SpanU8 data,
                                     const Features& features,
                                     Errors& errors)
    : data{data},
      section_index{Read<u32>(&data, features, errors)},
      count{ReadCount(&data, features, errors)},
      entries{data, features, errors} {}

RelocationSection ReadRelocationSection(SpanU8 data,
                                        const Features& features,
                                        Errors& errors) {
  return RelocationSection{data, features, errors};
}

RelocationSection ReadRelocationSection(CustomSection sec,
                                        const Features& features,
                                        Errors& errors) {
  return RelocationSection{sec.data, features, errors};
}

LazyTableSection ReadTableSection(SpanU8 data,
                                  const Features& features,
                                  Errors& errors) {
  return LazyTableSection{data, features, errors};
}

LazyTableSection ReadTableSection(KnownSection sec,
                                  const Features& features,
                                  Errors& errors) {
  return ReadTableSection(sec.data, features, errors);
}

LazyTypeSection ReadTypeSection(SpanU8 data,
                                const Features& features,
                                Errors& errors) {
  return LazyTypeSection{data, features, errors};
}

LazyTypeSection ReadTypeSection(KnownSection sec,
                                const Features& features,
                                Errors& errors) {
  return ReadTypeSection(sec.data, features, errors);
}

}  // namespace binary
}  // namespace wasp

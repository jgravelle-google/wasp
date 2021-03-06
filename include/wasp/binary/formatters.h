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

#ifndef WASP_BINARY_FORMATTERS_H_
#define WASP_BINARY_FORMATTERS_H_

#include "wasp/base/format.h"
#include "wasp/binary/br_on_exn_immediate.h"
#include "wasp/binary/br_table_immediate.h"
#include "wasp/binary/code.h"
#include "wasp/binary/comdat.h"
#include "wasp/binary/comdat_symbol.h"
#include "wasp/binary/comdat_symbol_kind.h"
#include "wasp/binary/constant_expression.h"
#include "wasp/binary/data_count.h"
#include "wasp/binary/data_segment.h"
#include "wasp/binary/element_expression.h"
#include "wasp/binary/element_segment.h"
#include "wasp/binary/export.h"
#include "wasp/binary/expression.h"
#include "wasp/binary/function.h"
#include "wasp/binary/global.h"
#include "wasp/binary/import.h"
#include "wasp/binary/indirect_name_assoc.h"
#include "wasp/binary/init_function.h"
#include "wasp/binary/instruction.h"
#include "wasp/binary/linking_subsection.h"
#include "wasp/binary/linking_subsection_id.h"
#include "wasp/binary/locals.h"
#include "wasp/binary/memory.h"
#include "wasp/binary/name_assoc.h"
#include "wasp/binary/name_subsection.h"
#include "wasp/binary/name_subsection_id.h"
#include "wasp/binary/relocation_entry.h"
#include "wasp/binary/relocation_type.h"
#include "wasp/binary/section.h"
#include "wasp/binary/segment_info.h"
#include "wasp/binary/shared.h"
#include "wasp/binary/start.h"
#include "wasp/binary/symbol_info.h"
#include "wasp/binary/symbol_info_kind.h"
#include "wasp/binary/table.h"
#include "wasp/binary/type_entry.h"

namespace fmt {

#define WASP_DEFINE_FORMATTER(Name)                                   \
  template <>                                                         \
  struct formatter<::wasp::binary::Name> : formatter<string_view> {   \
    template <typename Ctx>                                           \
    typename Ctx::iterator format(const ::wasp::binary::Name&, Ctx&); \
  } /* No semicolon. */

WASP_DEFINE_FORMATTER(ValueType);
WASP_DEFINE_FORMATTER(BlockType);
WASP_DEFINE_FORMATTER(ElementType);
WASP_DEFINE_FORMATTER(ExternalKind);
WASP_DEFINE_FORMATTER(Mutability);
WASP_DEFINE_FORMATTER(SegmentType);
WASP_DEFINE_FORMATTER(Shared);
WASP_DEFINE_FORMATTER(SectionId);
WASP_DEFINE_FORMATTER(NameSubsectionId);
WASP_DEFINE_FORMATTER(MemArgImmediate);
WASP_DEFINE_FORMATTER(Limits);
WASP_DEFINE_FORMATTER(Locals);
WASP_DEFINE_FORMATTER(Section);
WASP_DEFINE_FORMATTER(KnownSection);
WASP_DEFINE_FORMATTER(CustomSection);
WASP_DEFINE_FORMATTER(TypeEntry);
WASP_DEFINE_FORMATTER(FunctionType);
WASP_DEFINE_FORMATTER(TableType);
WASP_DEFINE_FORMATTER(MemoryType);
WASP_DEFINE_FORMATTER(GlobalType);
WASP_DEFINE_FORMATTER(Import);
WASP_DEFINE_FORMATTER(Export);
WASP_DEFINE_FORMATTER(Expression);
WASP_DEFINE_FORMATTER(ConstantExpression);
WASP_DEFINE_FORMATTER(ElementExpression);
WASP_DEFINE_FORMATTER(Opcode);
WASP_DEFINE_FORMATTER(CallIndirectImmediate);
WASP_DEFINE_FORMATTER(BrTableImmediate);
WASP_DEFINE_FORMATTER(BrOnExnImmediate);
WASP_DEFINE_FORMATTER(InitImmediate);
WASP_DEFINE_FORMATTER(CopyImmediate);
WASP_DEFINE_FORMATTER(ShuffleImmediate);
WASP_DEFINE_FORMATTER(Instruction);
WASP_DEFINE_FORMATTER(Function);
WASP_DEFINE_FORMATTER(Table);
WASP_DEFINE_FORMATTER(Memory);
WASP_DEFINE_FORMATTER(Global);
WASP_DEFINE_FORMATTER(Start);
WASP_DEFINE_FORMATTER(ElementSegment);
WASP_DEFINE_FORMATTER(Code);
WASP_DEFINE_FORMATTER(DataSegment);
WASP_DEFINE_FORMATTER(DataCount);
WASP_DEFINE_FORMATTER(NameAssoc);
WASP_DEFINE_FORMATTER(IndirectNameAssoc);
WASP_DEFINE_FORMATTER(InitFunction);
WASP_DEFINE_FORMATTER(NameSubsection);
WASP_DEFINE_FORMATTER(Comdat);
WASP_DEFINE_FORMATTER(ComdatSymbol);
WASP_DEFINE_FORMATTER(ComdatSymbolKind);
WASP_DEFINE_FORMATTER(LinkingSubsection);
WASP_DEFINE_FORMATTER(LinkingSubsectionId);
WASP_DEFINE_FORMATTER(RelocationEntry);
WASP_DEFINE_FORMATTER(RelocationType);
WASP_DEFINE_FORMATTER(SegmentInfo);
WASP_DEFINE_FORMATTER(SymbolInfo);
WASP_DEFINE_FORMATTER(SymbolInfo::Flags::Binding);
WASP_DEFINE_FORMATTER(SymbolInfo::Flags::Visibility);
WASP_DEFINE_FORMATTER(SymbolInfo::Flags::Undefined);
WASP_DEFINE_FORMATTER(SymbolInfo::Flags::ExplicitName);
WASP_DEFINE_FORMATTER(SymbolInfoKind);

#undef WASP_DEFINE_FORMATTER

}  // namespace fmt

#include "wasp/binary/formatters-inl.h"

#endif  // WASP_BINARY_FORMATTERS_H_

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

#include "src/binary/reader.h"

#include <cmath>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "src/base/types.h"

using namespace ::wasp;
using namespace ::wasp::binary;

namespace {

struct ErrorContext {
  SpanU8 pos;
  std::string desc;
};

struct ErrorContextLoc {
  SpanU8::index_type pos;
  std::string desc;
};

using Error = std::vector<ErrorContext>;
using ExpectedError = std::vector<ErrorContextLoc>;

class TestErrors {
 public:
  void PushContext(SpanU8 pos, string_view desc) {
    context_stack.push_back(ErrorContext{pos, desc.to_string()});
  }

  void PopContext() {
    context_stack.pop_back();
  }

  void OnError(SpanU8 pos, string_view message) {
    errors.emplace_back();
    auto& error = errors.back();
    for (const auto& ctx: context_stack) {
      error.push_back(ctx);
    }
    error.push_back(ErrorContext{pos, message.to_string()});
  }

  std::vector<ErrorContext> context_stack;
  std::vector<Error> errors;
};

template <size_t N>
SpanU8 MakeSpanU8(const char (&str)[N]) {
  return SpanU8{
      reinterpret_cast<const u8*>(str),
      static_cast<SpanU8::index_type>(N - 1)};  // -1 to remove \0 at end.
}

void ExpectNoErrors(const TestErrors& errors) {
  EXPECT_TRUE(errors.errors.empty());
  EXPECT_TRUE(errors.context_stack.empty());
}

void ExpectErrors(const std::vector<ExpectedError>& expected_errors,
                  const TestErrors& errors,
                  SpanU8 orig_data) {
  EXPECT_TRUE(errors.context_stack.empty());
  ASSERT_EQ(expected_errors.size(), errors.errors.size());
  for (size_t j = 0; j < expected_errors.size(); ++j) {
    const ExpectedError& expected = expected_errors[j];
    const Error& actual = errors.errors[j];
    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < actual.size(); ++i) {
      EXPECT_EQ(expected[i].pos, actual[i].pos.data() - orig_data.data());
      EXPECT_EQ(expected[i].desc, actual[i].desc);
    }
  }
}

void ExpectError(const ExpectedError& expected,
                 const TestErrors& errors,
                 SpanU8 orig_data) {
  ExpectErrors({expected}, errors, orig_data);
}

template <typename T>
void ExpectEmptyOptional(const optional<T>& actual) {
  EXPECT_FALSE(actual.has_value());
}

template <typename T>
void ExpectOptional(const T& expected, const optional<T>& actual) {
  ASSERT_TRUE(actual.has_value());
  EXPECT_EQ(expected, *actual);
}

template <typename T>
void ExpectRead(const T& expected, SpanU8 data) {
  TestErrors errors;
  auto result = Read<T>(&data, errors);
  ExpectNoErrors(errors);
  ExpectOptional(expected, result);
  EXPECT_EQ(0u, data.size());
}

template <typename T>
void ExpectReadFailure(const ExpectedError& expected, SpanU8 data) {
  TestErrors errors;
  const SpanU8 orig_data = data;
  auto result = Read<T>(&data, errors);
  ExpectError(expected, errors, orig_data);
  ExpectEmptyOptional(result);
}

}  // namespace

TEST(ReaderTest, ReadU8) {
  ExpectRead<u8>(32, MakeSpanU8("\x20"));
  ExpectReadFailure<u8>({{0, "Unable to read u8"}}, MakeSpanU8(""));
}

TEST(ReaderTest, ReadBytes) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 3, errors);
  ExpectNoErrors(errors);
  ExpectOptional(data, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadBytes_Leftovers) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 2, errors);
  ExpectNoErrors(errors);
  ExpectOptional(data.subspan(0, 2), result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReaderTest, ReadBytes_Fail) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x12\x34\x56");
  SpanU8 copy = data;
  auto result = ReadBytes(&copy, 4, errors);
  ExpectEmptyOptional(result);
  ExpectError({{0, "Unable to read 4 bytes"}}, errors, data);
}

TEST(ReaderTest, ReadU32) {
  ExpectRead<u32>(32u, MakeSpanU8("\x20"));
  ExpectRead<u32>(448u, MakeSpanU8("\xc0\x03"));
  ExpectRead<u32>(33360u, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<u32>(101718048u, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<u32>(1042036848u, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
}

TEST(ReaderTest, ReadU32_TooLong) {
  ExpectReadFailure<u32>(
      {{0, "u32"},
       {5, "Last byte of u32 must be zero extension: expected 0x2, got 0x12"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\x12"));
}

TEST(ReaderTest, ReadU32_PastEnd) {
  ExpectReadFailure<u32>({{0, "u32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<u32>({{0, "u32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<u32>({{0, "u32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<u32>({{0, "u32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<u32>({{0, "u32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

TEST(ReaderTest, ReadS32) {
  ExpectRead<s32>(32, MakeSpanU8("\x20"));
  ExpectRead<s32>(-16, MakeSpanU8("\x70"));
  ExpectRead<s32>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s32>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s32>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s32>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s32>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s32>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s32>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s32>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
}

TEST(ReaderTest, ReadS32_TooLong) {
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x5 or 0x7d, got 0x15"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0\x15"));
  ExpectReadFailure<s32>({{0, "s32"},
                          {5,
                           "Last byte of s32 must be sign extension: expected "
                           "0x3 or 0x7b, got 0x73"}},
                         MakeSpanU8("\xff\xff\xff\xff\x73"));
}

TEST(ReaderTest, ReadS32_PastEnd) {
  ExpectReadFailure<s32>({{0, "s32"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s32>({{0, "s32"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s32>({{0, "s32"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s32>({{0, "s32"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s32>({{0, "s32"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
}

TEST(ReaderTest, ReadS64) {
  ExpectRead<s64>(32, MakeSpanU8("\x20"));
  ExpectRead<s64>(-16, MakeSpanU8("\x70"));
  ExpectRead<s64>(448, MakeSpanU8("\xc0\x03"));
  ExpectRead<s64>(-3648, MakeSpanU8("\xc0\x63"));
  ExpectRead<s64>(33360, MakeSpanU8("\xd0\x84\x02"));
  ExpectRead<s64>(-753072, MakeSpanU8("\xd0\x84\x52"));
  ExpectRead<s64>(101718048, MakeSpanU8("\xa0\xb0\xc0\x30"));
  ExpectRead<s64>(-32499680, MakeSpanU8("\xa0\xb0\xc0\x70"));
  ExpectRead<s64>(1042036848, MakeSpanU8("\xf0\xf0\xf0\xf0\x03"));
  ExpectRead<s64>(-837011344, MakeSpanU8("\xf0\xf0\xf0\xf0\x7c"));
  ExpectRead<s64>(13893120096, MakeSpanU8("\xe0\xe0\xe0\xe0\x33"));
  ExpectRead<s64>(-12413554592, MakeSpanU8("\xe0\xe0\xe0\xe0\x51"));
  ExpectRead<s64>(1533472417872, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x2c"));
  ExpectRead<s64>(-287593715632, MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\x77"));
  ExpectRead<s64>(139105536057408, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x1f"));
  ExpectRead<s64>(-124777254608832, MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x63"));
  ExpectRead<s64>(1338117014066474,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x02"));
  ExpectRead<s64>(-12172681868045014,
                  MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\x6a"));
  ExpectRead<s64>(1070725794579330814,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x0e"));
  ExpectRead<s64>(-3540960223848057090,
                  MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\x4e"));
}

TEST(ReaderTest, ReadS64_TooLong) {
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xf0"}},
      MakeSpanU8("\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>(
      {{0, "s64"},
       {10,
        "Last byte of s64 must be sign extension: expected 0x0 or 0x7f, got "
        "0xff"}},
      MakeSpanU8("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"));
}

TEST(ReaderTest, ReadS64_PastEnd) {
  ExpectReadFailure<s64>({{0, "s64"}, {0, "Unable to read u8"}},
                         MakeSpanU8(""));
  ExpectReadFailure<s64>({{0, "s64"}, {1, "Unable to read u8"}},
                         MakeSpanU8("\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {2, "Unable to read u8"}},
                         MakeSpanU8("\xd0\x84"));
  ExpectReadFailure<s64>({{0, "s64"}, {3, "Unable to read u8"}},
                         MakeSpanU8("\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {4, "Unable to read u8"}},
                         MakeSpanU8("\xf0\xf0\xf0\xf0"));
  ExpectReadFailure<s64>({{0, "s64"}, {5, "Unable to read u8"}},
                         MakeSpanU8("\xe0\xe0\xe0\xe0\xe0"));
  ExpectReadFailure<s64>({{0, "s64"}, {6, "Unable to read u8"}},
                         MakeSpanU8("\xd0\xd0\xd0\xd0\xd0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {7, "Unable to read u8"}},
                         MakeSpanU8("\xc0\xc0\xc0\xc0\xc0\xd0\x84"));
  ExpectReadFailure<s64>({{0, "s64"}, {8, "Unable to read u8"}},
                         MakeSpanU8("\xaa\xaa\xaa\xaa\xaa\xa0\xb0\xc0"));
  ExpectReadFailure<s64>({{0, "s64"}, {9, "Unable to read u8"}},
                         MakeSpanU8("\xfe\xed\xfe\xed\xfe\xed\xfe\xed\xfe"));
}

TEST(ReaderTest, ReadF32) {
  ExpectRead<f32>(0.0f, MakeSpanU8("\x00\x00\x00\x00"));
  ExpectRead<f32>(-1.0f, MakeSpanU8("\x00\x00\x80\xbf"));
  ExpectRead<f32>(1234567.0f, MakeSpanU8("\x38\xb4\x96\x49"));
  ExpectRead<f32>(INFINITY, MakeSpanU8("\x00\x00\x80\x7f"));
  ExpectRead<f32>(-INFINITY, MakeSpanU8("\x00\x00\x80\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\xc0\x7f");
    TestErrors errors;
    auto result = Read<f32>(&data, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReaderTest, ReadF32_PastEnd) {
  ExpectReadFailure<f32>({{0, "f32"}, {0, "Unable to read 4 bytes"}},
                         MakeSpanU8("\x00\x00\x00"));
}

TEST(ReaderTest, ReadF64) {
  ExpectRead<f64>(0.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00\x00"));
  ExpectRead<f64>(-1.0, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xbf"));
  ExpectRead<f64>(111111111111111,
                  MakeSpanU8("\xc0\x71\xbc\x93\x84\x43\xd9\x42"));
  ExpectRead<f64>(INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\x7f"));
  ExpectRead<f64>(-INFINITY, MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf0\xff"));

  // NaN
  {
    auto data = MakeSpanU8("\x00\x00\x00\x00\x00\x00\xf8\x7f");
    TestErrors errors;
    auto result = Read<f64>(&data, errors);
    ExpectNoErrors(errors);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(std::isnan(*result));
    EXPECT_EQ(0u, data.size());
  }
}

TEST(ReaderTest, ReadF64_PastEnd) {
  ExpectReadFailure<f64>({{0, "f64"}, {0, "Unable to read 8 bytes"}},
                         MakeSpanU8("\x00\x00\x00\x00\x00\x00\x00"));
}

TEST(ReaderTest, ReadCount) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, errors);
  ExpectNoErrors(errors);
  ExpectOptional(1u, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadCount_PastEnd) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05\x00\x00\x00");
  SpanU8 copy = data;
  auto result = ReadCount(&copy, errors);
  ExpectError({{1, "Count is longer than the data length: 5 > 3"}}, errors,
              data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadString) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadString(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(string_view{"hello"}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadString_Leftovers) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x01more");
  SpanU8 copy = data;
  auto result = ReadString(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(string_view{"m"}, result);
  EXPECT_EQ(3u, copy.size());
}

TEST(ReaderTest, ReadString_FailLength) {
  {
    TestErrors errors;
    const SpanU8 data = MakeSpanU8("");
    SpanU8 copy = data;
    auto result = ReadString(&copy, errors, "test");
    ExpectError({{0, "test"}, {0, "index"}, {0, "Unable to read u8"}}, errors,
                data);
    ExpectEmptyOptional(result);
    EXPECT_EQ(0u, copy.size());
  }

  {
    TestErrors errors;
    const SpanU8 data = MakeSpanU8("\xc0");
    SpanU8 copy = data;
    auto result = ReadString(&copy, errors, "test");
    ExpectError({{0, "test"}, {0, "index"}, {1, "Unable to read u8"}}, errors,
                data);
    ExpectEmptyOptional(result);
    EXPECT_EQ(0u, copy.size());
  }
}

TEST(ReaderTest, ReadString_Fail) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x06small");
  SpanU8 copy = data;
  auto result = ReadString(&copy, errors, "test");
  ExpectError({{0, "test"}, {1, "Count is longer than the data length: 6 > 5"}},
              errors, data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(5u, copy.size());
}

TEST(ReaderTest, ReadVector_u8) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8("\x05hello");
  SpanU8 copy = data;
  auto result = ReadVector<u8>(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(std::vector<u8>{'h', 'e', 'l', 'l', 'o'}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadVector_u32) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x03"  // Count.
      "\x05"
      "\x80\x01"
      "\xcc\xcc\x0c");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, errors, "test");
  ExpectNoErrors(errors);
  ExpectOptional(std::vector<u32>{5, 128, 206412}, result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadVector_FailLength) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, errors, "test");
  ExpectError({{0, "test"}, {1, "Count is longer than the data length: 2 > 1"}},
              errors, data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(1u, copy.size());
}

TEST(ReaderTest, ReadVector_PastEnd) {
  TestErrors errors;
  const SpanU8 data = MakeSpanU8(
      "\x02"  // Count.
      "\x05"
      "\x80");
  SpanU8 copy = data;
  auto result = ReadVector<u32>(&copy, errors, "test");
  ExpectError({{0, "test"}, {2, "u32"}, {3, "Unable to read u8"}}, errors,
              data);
  ExpectEmptyOptional(result);
  EXPECT_EQ(0u, copy.size());
}

TEST(ReaderTest, ReadValueType) {
  ExpectRead<ValueType>(ValueType::I32, MakeSpanU8("\x7f"));
  ExpectRead<ValueType>(ValueType::I64, MakeSpanU8("\x7e"));
  ExpectRead<ValueType>(ValueType::F32, MakeSpanU8("\x7d"));
  ExpectRead<ValueType>(ValueType::F64, MakeSpanU8("\x7c"));
}

TEST(ReaderTest, ReadValueType_Unknown) {
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 16"}}, MakeSpanU8("\x10"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ValueType>(
      {{0, "value type"}, {1, "Unknown value type: 255"}},
      MakeSpanU8("\xff\x7f"));
}

TEST(ReaderTest, ReadBlockType) {
  ExpectRead<BlockType>(BlockType::I32, MakeSpanU8("\x7f"));
  ExpectRead<BlockType>(BlockType::I64, MakeSpanU8("\x7e"));
  ExpectRead<BlockType>(BlockType::F32, MakeSpanU8("\x7d"));
  ExpectRead<BlockType>(BlockType::F64, MakeSpanU8("\x7c"));
  ExpectRead<BlockType>(BlockType::Void, MakeSpanU8("\x40"));
}

TEST(ReaderTest, ReadBlockType_Unknown) {
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 0"}}, MakeSpanU8("\x00"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<BlockType>(
      {{0, "block type"}, {1, "Unknown block type: 255"}},
      MakeSpanU8("\xff\x7f"));
}

TEST(ReaderTest, ReadElementType) {
  ExpectRead<ElementType>(ElementType::Funcref, MakeSpanU8("\x70"));
}

TEST(ReaderTest, ReadElementType_Unknown) {
  ExpectReadFailure<ElementType>(
      {{0, "element type"}, {1, "Unknown element type: 0"}},
      MakeSpanU8("\x00"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ElementType>(
      {{0, "element type"}, {1, "Unknown element type: 240"}},
      MakeSpanU8("\xf0\x7f"));
}

TEST(ReaderTest, ReadExternalKind) {
  ExpectRead<ExternalKind>(ExternalKind::Function, MakeSpanU8("\x00"));
  ExpectRead<ExternalKind>(ExternalKind::Table, MakeSpanU8("\x01"));
  ExpectRead<ExternalKind>(ExternalKind::Memory, MakeSpanU8("\x02"));
  ExpectRead<ExternalKind>(ExternalKind::Global, MakeSpanU8("\x03"));
}

TEST(ReaderTest, ReadExternalKind_Unknown) {
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 4"}},
      MakeSpanU8("\x04"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<ExternalKind>(
      {{0, "external kind"}, {1, "Unknown external kind: 132"}},
      MakeSpanU8("\x84\x00"));
}

TEST(ReaderTest, ReadMutability) {
  ExpectRead<Mutability>(Mutability::Const, MakeSpanU8("\x00"));
  ExpectRead<Mutability>(Mutability::Var, MakeSpanU8("\x01"));
}

TEST(ReaderTest, ReadMutability_Unknown) {
  ExpectReadFailure<Mutability>(
      {{0, "mutability"}, {1, "Unknown mutability: 4"}}, MakeSpanU8("\x04"));

  // Overlong encoding is not allowed.
  ExpectReadFailure<Mutability>(
      {{0, "mutability"}, {1, "Unknown mutability: 132"}},
      MakeSpanU8("\x84\x00"));
}

TEST(ReaderTest, ReadSectionId) {
  ExpectRead<SectionId>(SectionId::Custom, MakeSpanU8("\x00"));
  ExpectRead<SectionId>(SectionId::Type, MakeSpanU8("\x01"));
  ExpectRead<SectionId>(SectionId::Import, MakeSpanU8("\x02"));
  ExpectRead<SectionId>(SectionId::Function, MakeSpanU8("\x03"));
  ExpectRead<SectionId>(SectionId::Table, MakeSpanU8("\x04"));
  ExpectRead<SectionId>(SectionId::Memory, MakeSpanU8("\x05"));
  ExpectRead<SectionId>(SectionId::Global, MakeSpanU8("\x06"));
  ExpectRead<SectionId>(SectionId::Export, MakeSpanU8("\x07"));
  ExpectRead<SectionId>(SectionId::Start, MakeSpanU8("\x08"));
  ExpectRead<SectionId>(SectionId::Element, MakeSpanU8("\x09"));
  ExpectRead<SectionId>(SectionId::Code, MakeSpanU8("\x0a"));
  ExpectRead<SectionId>(SectionId::Data, MakeSpanU8("\x0b"));

  // Overlong encoding.
  ExpectRead<SectionId>(SectionId::Custom, MakeSpanU8("\x80\x00"));
}

TEST(ReaderTest, ReadSectionId_Unknown) {
  ExpectReadFailure<SectionId>({{0, "section"}, {1, "Unknown section: 12"}},
                               MakeSpanU8("\x0c"));
}

TEST(ReaderTest, ReadOpcode) {
  ExpectRead<Opcode>(Opcode::Unreachable, MakeSpanU8("\x00"));
  ExpectRead<Opcode>(Opcode::Nop, MakeSpanU8("\x01"));
  ExpectRead<Opcode>(Opcode::Block, MakeSpanU8("\x02"));
  ExpectRead<Opcode>(Opcode::Loop, MakeSpanU8("\x03"));
  ExpectRead<Opcode>(Opcode::If, MakeSpanU8("\x04"));
  ExpectRead<Opcode>(Opcode::Else, MakeSpanU8("\x05"));
  ExpectRead<Opcode>(Opcode::End, MakeSpanU8("\x0b"));
  ExpectRead<Opcode>(Opcode::Br, MakeSpanU8("\x0c"));
  ExpectRead<Opcode>(Opcode::BrIf, MakeSpanU8("\x0d"));
  ExpectRead<Opcode>(Opcode::BrTable, MakeSpanU8("\x0e"));
  ExpectRead<Opcode>(Opcode::Return, MakeSpanU8("\x0f"));
  ExpectRead<Opcode>(Opcode::Call, MakeSpanU8("\x10"));
  ExpectRead<Opcode>(Opcode::CallIndirect, MakeSpanU8("\x11"));
  ExpectRead<Opcode>(Opcode::Drop, MakeSpanU8("\x1a"));
  ExpectRead<Opcode>(Opcode::Select, MakeSpanU8("\x1b"));
  ExpectRead<Opcode>(Opcode::GetLocal, MakeSpanU8("\x20"));
  ExpectRead<Opcode>(Opcode::SetLocal, MakeSpanU8("\x21"));
  ExpectRead<Opcode>(Opcode::TeeLocal, MakeSpanU8("\x22"));
  ExpectRead<Opcode>(Opcode::GetGlobal, MakeSpanU8("\x23"));
  ExpectRead<Opcode>(Opcode::SetGlobal, MakeSpanU8("\x24"));
  ExpectRead<Opcode>(Opcode::I32Load, MakeSpanU8("\x28"));
  ExpectRead<Opcode>(Opcode::I64Load, MakeSpanU8("\x29"));
  ExpectRead<Opcode>(Opcode::F32Load, MakeSpanU8("\x2a"));
  ExpectRead<Opcode>(Opcode::F64Load, MakeSpanU8("\x2b"));
  ExpectRead<Opcode>(Opcode::I32Load8S, MakeSpanU8("\x2c"));
  ExpectRead<Opcode>(Opcode::I32Load8U, MakeSpanU8("\x2d"));
  ExpectRead<Opcode>(Opcode::I32Load16S, MakeSpanU8("\x2e"));
  ExpectRead<Opcode>(Opcode::I32Load16U, MakeSpanU8("\x2f"));
  ExpectRead<Opcode>(Opcode::I64Load8S, MakeSpanU8("\x30"));
  ExpectRead<Opcode>(Opcode::I64Load8U, MakeSpanU8("\x31"));
  ExpectRead<Opcode>(Opcode::I64Load16S, MakeSpanU8("\x32"));
  ExpectRead<Opcode>(Opcode::I64Load16U, MakeSpanU8("\x33"));
  ExpectRead<Opcode>(Opcode::I64Load32S, MakeSpanU8("\x34"));
  ExpectRead<Opcode>(Opcode::I64Load32U, MakeSpanU8("\x35"));
  ExpectRead<Opcode>(Opcode::I32Store, MakeSpanU8("\x36"));
  ExpectRead<Opcode>(Opcode::I64Store, MakeSpanU8("\x37"));
  ExpectRead<Opcode>(Opcode::F32Store, MakeSpanU8("\x38"));
  ExpectRead<Opcode>(Opcode::F64Store, MakeSpanU8("\x39"));
  ExpectRead<Opcode>(Opcode::I32Store8, MakeSpanU8("\x3a"));
  ExpectRead<Opcode>(Opcode::I32Store16, MakeSpanU8("\x3b"));
  ExpectRead<Opcode>(Opcode::I64Store8, MakeSpanU8("\x3c"));
  ExpectRead<Opcode>(Opcode::I64Store16, MakeSpanU8("\x3d"));
  ExpectRead<Opcode>(Opcode::I64Store32, MakeSpanU8("\x3e"));
  ExpectRead<Opcode>(Opcode::MemorySize, MakeSpanU8("\x3f"));
  ExpectRead<Opcode>(Opcode::MemoryGrow, MakeSpanU8("\x40"));
  ExpectRead<Opcode>(Opcode::I32Const, MakeSpanU8("\x41"));
  ExpectRead<Opcode>(Opcode::I64Const, MakeSpanU8("\x42"));
  ExpectRead<Opcode>(Opcode::F32Const, MakeSpanU8("\x43"));
  ExpectRead<Opcode>(Opcode::F64Const, MakeSpanU8("\x44"));
  ExpectRead<Opcode>(Opcode::I32Eqz, MakeSpanU8("\x45"));
  ExpectRead<Opcode>(Opcode::I32Eq, MakeSpanU8("\x46"));
  ExpectRead<Opcode>(Opcode::I32Ne, MakeSpanU8("\x47"));
  ExpectRead<Opcode>(Opcode::I32LtS, MakeSpanU8("\x48"));
  ExpectRead<Opcode>(Opcode::I32LtU, MakeSpanU8("\x49"));
  ExpectRead<Opcode>(Opcode::I32GtS, MakeSpanU8("\x4a"));
  ExpectRead<Opcode>(Opcode::I32GtU, MakeSpanU8("\x4b"));
  ExpectRead<Opcode>(Opcode::I32LeS, MakeSpanU8("\x4c"));
  ExpectRead<Opcode>(Opcode::I32LeU, MakeSpanU8("\x4d"));
  ExpectRead<Opcode>(Opcode::I32GeS, MakeSpanU8("\x4e"));
  ExpectRead<Opcode>(Opcode::I32GeU, MakeSpanU8("\x4f"));
  ExpectRead<Opcode>(Opcode::I64Eqz, MakeSpanU8("\x50"));
  ExpectRead<Opcode>(Opcode::I64Eq, MakeSpanU8("\x51"));
  ExpectRead<Opcode>(Opcode::I64Ne, MakeSpanU8("\x52"));
  ExpectRead<Opcode>(Opcode::I64LtS, MakeSpanU8("\x53"));
  ExpectRead<Opcode>(Opcode::I64LtU, MakeSpanU8("\x54"));
  ExpectRead<Opcode>(Opcode::I64GtS, MakeSpanU8("\x55"));
  ExpectRead<Opcode>(Opcode::I64GtU, MakeSpanU8("\x56"));
  ExpectRead<Opcode>(Opcode::I64LeS, MakeSpanU8("\x57"));
  ExpectRead<Opcode>(Opcode::I64LeU, MakeSpanU8("\x58"));
  ExpectRead<Opcode>(Opcode::I64GeS, MakeSpanU8("\x59"));
  ExpectRead<Opcode>(Opcode::I64GeU, MakeSpanU8("\x5a"));
  ExpectRead<Opcode>(Opcode::F32Eq, MakeSpanU8("\x5b"));
  ExpectRead<Opcode>(Opcode::F32Ne, MakeSpanU8("\x5c"));
  ExpectRead<Opcode>(Opcode::F32Lt, MakeSpanU8("\x5d"));
  ExpectRead<Opcode>(Opcode::F32Gt, MakeSpanU8("\x5e"));
  ExpectRead<Opcode>(Opcode::F32Le, MakeSpanU8("\x5f"));
  ExpectRead<Opcode>(Opcode::F32Ge, MakeSpanU8("\x60"));
  ExpectRead<Opcode>(Opcode::F64Eq, MakeSpanU8("\x61"));
  ExpectRead<Opcode>(Opcode::F64Ne, MakeSpanU8("\x62"));
  ExpectRead<Opcode>(Opcode::F64Lt, MakeSpanU8("\x63"));
  ExpectRead<Opcode>(Opcode::F64Gt, MakeSpanU8("\x64"));
  ExpectRead<Opcode>(Opcode::F64Le, MakeSpanU8("\x65"));
  ExpectRead<Opcode>(Opcode::F64Ge, MakeSpanU8("\x66"));
  ExpectRead<Opcode>(Opcode::I32Clz, MakeSpanU8("\x67"));
  ExpectRead<Opcode>(Opcode::I32Ctz, MakeSpanU8("\x68"));
  ExpectRead<Opcode>(Opcode::I32Popcnt, MakeSpanU8("\x69"));
  ExpectRead<Opcode>(Opcode::I32Add, MakeSpanU8("\x6a"));
  ExpectRead<Opcode>(Opcode::I32Sub, MakeSpanU8("\x6b"));
  ExpectRead<Opcode>(Opcode::I32Mul, MakeSpanU8("\x6c"));
  ExpectRead<Opcode>(Opcode::I32DivS, MakeSpanU8("\x6d"));
  ExpectRead<Opcode>(Opcode::I32DivU, MakeSpanU8("\x6e"));
  ExpectRead<Opcode>(Opcode::I32RemS, MakeSpanU8("\x6f"));
  ExpectRead<Opcode>(Opcode::I32RemU, MakeSpanU8("\x70"));
  ExpectRead<Opcode>(Opcode::I32And, MakeSpanU8("\x71"));
  ExpectRead<Opcode>(Opcode::I32Or, MakeSpanU8("\x72"));
  ExpectRead<Opcode>(Opcode::I32Xor, MakeSpanU8("\x73"));
  ExpectRead<Opcode>(Opcode::I32Shl, MakeSpanU8("\x74"));
  ExpectRead<Opcode>(Opcode::I32ShrS, MakeSpanU8("\x75"));
  ExpectRead<Opcode>(Opcode::I32ShrU, MakeSpanU8("\x76"));
  ExpectRead<Opcode>(Opcode::I32Rotl, MakeSpanU8("\x77"));
  ExpectRead<Opcode>(Opcode::I32Rotr, MakeSpanU8("\x78"));
  ExpectRead<Opcode>(Opcode::I64Clz, MakeSpanU8("\x79"));
  ExpectRead<Opcode>(Opcode::I64Ctz, MakeSpanU8("\x7a"));
  ExpectRead<Opcode>(Opcode::I64Popcnt, MakeSpanU8("\x7b"));
  ExpectRead<Opcode>(Opcode::I64Add, MakeSpanU8("\x7c"));
  ExpectRead<Opcode>(Opcode::I64Sub, MakeSpanU8("\x7d"));
  ExpectRead<Opcode>(Opcode::I64Mul, MakeSpanU8("\x7e"));
  ExpectRead<Opcode>(Opcode::I64DivS, MakeSpanU8("\x7f"));
  ExpectRead<Opcode>(Opcode::I64DivU, MakeSpanU8("\x80"));
  ExpectRead<Opcode>(Opcode::I64RemS, MakeSpanU8("\x81"));
  ExpectRead<Opcode>(Opcode::I64RemU, MakeSpanU8("\x82"));
  ExpectRead<Opcode>(Opcode::I64And, MakeSpanU8("\x83"));
  ExpectRead<Opcode>(Opcode::I64Or, MakeSpanU8("\x84"));
  ExpectRead<Opcode>(Opcode::I64Xor, MakeSpanU8("\x85"));
  ExpectRead<Opcode>(Opcode::I64Shl, MakeSpanU8("\x86"));
  ExpectRead<Opcode>(Opcode::I64ShrS, MakeSpanU8("\x87"));
  ExpectRead<Opcode>(Opcode::I64ShrU, MakeSpanU8("\x88"));
  ExpectRead<Opcode>(Opcode::I64Rotl, MakeSpanU8("\x89"));
  ExpectRead<Opcode>(Opcode::I64Rotr, MakeSpanU8("\x8a"));
  ExpectRead<Opcode>(Opcode::F32Abs, MakeSpanU8("\x8b"));
  ExpectRead<Opcode>(Opcode::F32Neg, MakeSpanU8("\x8c"));
  ExpectRead<Opcode>(Opcode::F32Ceil, MakeSpanU8("\x8d"));
  ExpectRead<Opcode>(Opcode::F32Floor, MakeSpanU8("\x8e"));
  ExpectRead<Opcode>(Opcode::F32Trunc, MakeSpanU8("\x8f"));
  ExpectRead<Opcode>(Opcode::F32Nearest, MakeSpanU8("\x90"));
  ExpectRead<Opcode>(Opcode::F32Sqrt, MakeSpanU8("\x91"));
  ExpectRead<Opcode>(Opcode::F32Add, MakeSpanU8("\x92"));
  ExpectRead<Opcode>(Opcode::F32Sub, MakeSpanU8("\x93"));
  ExpectRead<Opcode>(Opcode::F32Mul, MakeSpanU8("\x94"));
  ExpectRead<Opcode>(Opcode::F32Div, MakeSpanU8("\x95"));
  ExpectRead<Opcode>(Opcode::F32Min, MakeSpanU8("\x96"));
  ExpectRead<Opcode>(Opcode::F32Max, MakeSpanU8("\x97"));
  ExpectRead<Opcode>(Opcode::F32Copysign, MakeSpanU8("\x98"));
  ExpectRead<Opcode>(Opcode::F64Abs, MakeSpanU8("\x99"));
  ExpectRead<Opcode>(Opcode::F64Neg, MakeSpanU8("\x9a"));
  ExpectRead<Opcode>(Opcode::F64Ceil, MakeSpanU8("\x9b"));
  ExpectRead<Opcode>(Opcode::F64Floor, MakeSpanU8("\x9c"));
  ExpectRead<Opcode>(Opcode::F64Trunc, MakeSpanU8("\x9d"));
  ExpectRead<Opcode>(Opcode::F64Nearest, MakeSpanU8("\x9e"));
  ExpectRead<Opcode>(Opcode::F64Sqrt, MakeSpanU8("\x9f"));
  ExpectRead<Opcode>(Opcode::F64Add, MakeSpanU8("\xa0"));
  ExpectRead<Opcode>(Opcode::F64Sub, MakeSpanU8("\xa1"));
  ExpectRead<Opcode>(Opcode::F64Mul, MakeSpanU8("\xa2"));
  ExpectRead<Opcode>(Opcode::F64Div, MakeSpanU8("\xa3"));
  ExpectRead<Opcode>(Opcode::F64Min, MakeSpanU8("\xa4"));
  ExpectRead<Opcode>(Opcode::F64Max, MakeSpanU8("\xa5"));
  ExpectRead<Opcode>(Opcode::F64Copysign, MakeSpanU8("\xa6"));
  ExpectRead<Opcode>(Opcode::I32WrapI64, MakeSpanU8("\xa7"));
  ExpectRead<Opcode>(Opcode::I32TruncSF32, MakeSpanU8("\xa8"));
  ExpectRead<Opcode>(Opcode::I32TruncUF32, MakeSpanU8("\xa9"));
  ExpectRead<Opcode>(Opcode::I32TruncSF64, MakeSpanU8("\xaa"));
  ExpectRead<Opcode>(Opcode::I32TruncUF64, MakeSpanU8("\xab"));
  ExpectRead<Opcode>(Opcode::I64ExtendSI32, MakeSpanU8("\xac"));
  ExpectRead<Opcode>(Opcode::I64ExtendUI32, MakeSpanU8("\xad"));
  ExpectRead<Opcode>(Opcode::I64TruncSF32, MakeSpanU8("\xae"));
  ExpectRead<Opcode>(Opcode::I64TruncUF32, MakeSpanU8("\xaf"));
  ExpectRead<Opcode>(Opcode::I64TruncSF64, MakeSpanU8("\xb0"));
  ExpectRead<Opcode>(Opcode::I64TruncUF64, MakeSpanU8("\xb1"));
  ExpectRead<Opcode>(Opcode::F32ConvertSI32, MakeSpanU8("\xb2"));
  ExpectRead<Opcode>(Opcode::F32ConvertUI32, MakeSpanU8("\xb3"));
  ExpectRead<Opcode>(Opcode::F32ConvertSI64, MakeSpanU8("\xb4"));
  ExpectRead<Opcode>(Opcode::F32ConvertUI64, MakeSpanU8("\xb5"));
  ExpectRead<Opcode>(Opcode::F32DemoteF64, MakeSpanU8("\xb6"));
  ExpectRead<Opcode>(Opcode::F64ConvertSI32, MakeSpanU8("\xb7"));
  ExpectRead<Opcode>(Opcode::F64ConvertUI32, MakeSpanU8("\xb8"));
  ExpectRead<Opcode>(Opcode::F64ConvertSI64, MakeSpanU8("\xb9"));
  ExpectRead<Opcode>(Opcode::F64ConvertUI64, MakeSpanU8("\xba"));
  ExpectRead<Opcode>(Opcode::F64PromoteF32, MakeSpanU8("\xbb"));
  ExpectRead<Opcode>(Opcode::I32ReinterpretF32, MakeSpanU8("\xbc"));
  ExpectRead<Opcode>(Opcode::I64ReinterpretF64, MakeSpanU8("\xbd"));
  ExpectRead<Opcode>(Opcode::F32ReinterpretI32, MakeSpanU8("\xbe"));
  ExpectRead<Opcode>(Opcode::F64ReinterpretI64, MakeSpanU8("\xbf"));
}

TEST(ReaderTest, ReadOpcode_Unknown) {
  ExpectReadFailure<Opcode>({{0, "opcode"}, {1, "Unknown opcode: 6"}},
                            MakeSpanU8("\x06"));
  ExpectReadFailure<Opcode>({{0, "opcode"}, {1, "Unknown opcode: 255"}},
                            MakeSpanU8("\xff"));
}

TEST(ReaderTest, ReadMemArg) {
  ExpectRead<MemArg>(MemArg{0, 0}, MakeSpanU8("\x00\x00"));
  ExpectRead<MemArg>(MemArg{1, 256}, MakeSpanU8("\x01\x80\x02"));
}

TEST(ReaderTest, ReadLimits) {
  ExpectRead<Limits>(Limits{129}, MakeSpanU8("\x00\x81\x01"));
  ExpectRead<Limits>(Limits{2, 1000}, MakeSpanU8("\x01\x02\xe8\x07"));
}

TEST(ReaderTest, ReadLimits_BadFlags) {
  ExpectReadFailure<Limits>({{0, "limits"}, {1, "Invalid flags value: 2"}},
                            MakeSpanU8("\x02\x01"));
}

TEST(ReaderTest, ReadLimits_PastEnd) {
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {1, "min"}, {1, "u32"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
  ExpectReadFailure<Limits>(
      {{0, "limits"}, {2, "max"}, {2, "u32"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x01\x00"));
}

TEST(ReaderTest, ReadLocals) {
  ExpectRead<Locals>(Locals{2, ValueType::I32}, MakeSpanU8("\x02\x7f"));
  ExpectRead<Locals>(Locals{320, ValueType::F64}, MakeSpanU8("\xc0\x02\x7c"));
}

TEST(ReaderTest, ReadLocals_PastEnd) {
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {0, "count"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));
  ExpectReadFailure<Locals>(
      {{0, "locals"}, {2, "type"}, {2, "value type"}, {2, "Unable to read u8"}},
      MakeSpanU8("\xc0\x02"));
}

TEST(ReaderTest, ReadFunctionType) {
  ExpectRead<FunctionType>(FunctionType{{}, {}}, MakeSpanU8("\x00\x00"));
  ExpectRead<FunctionType>(
      FunctionType{{ValueType::I32, ValueType::I64}, {ValueType::F64}},
      MakeSpanU8("\x02\x7f\x7e\x01\x7c"));
}

TEST(ReaderTest, ReadFunctionType_PastEnd) {
  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {0, "param types"},
                                   {0, "index"},
                                   {0, "Unable to read u8"}},
                                  MakeSpanU8(""));

  ExpectReadFailure<FunctionType>(
      {{0, "function type"},
       {0, "param types"},
       {1, "Count is longer than the data length: 1 > 0"}},
      MakeSpanU8("\x01"));

  ExpectReadFailure<FunctionType>({{0, "function type"},
                                   {1, "result types"},
                                   {1, "index"},
                                   {1, "Unable to read u8"}},
                                  MakeSpanU8("\x00"));

  ExpectReadFailure<FunctionType>(
      {{0, "function type"},
       {1, "result types"},
       {2, "Count is longer than the data length: 1 > 0"}},
      MakeSpanU8("\x00\x01"));
}

TEST(ReaderTest, ReadTypeEntry) {
  ExpectRead<TypeEntry>(TypeEntry{FunctionType{{}, {ValueType::I32}}},
                        MakeSpanU8("\x60\x00\x01\x7f"));
}

TEST(ReaderTest, ReadTypeEntry_BadForm) {
  ExpectReadFailure<TypeEntry>(
      {{0, "type entry"}, {1, "Unknown type form: 64"}}, MakeSpanU8("\x40"));
}

TEST(ReaderTest, ReadTableType) {
  ExpectRead<TableType>(TableType{Limits{1}, ElementType::Funcref},
                        MakeSpanU8("\x70\x00\x01"));
  ExpectRead<TableType>(TableType{Limits{1, 2}, ElementType::Funcref},
                        MakeSpanU8("\x70\x01\x01\x02"));
}

TEST(ReaderTest, ReadTableType_BadElementType) {
  ExpectReadFailure<TableType>(
      {{0, "table type"}, {0, "element type"}, {1, "Unknown element type: 0"}},
      MakeSpanU8("\x00"));
}

TEST(ReaderTest, ReadTableType_PastEnd) {
  ExpectReadFailure<TableType>(
      {{0, "table type"}, {0, "element type"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<TableType>({{0, "table type"},
                                {1, "limits"},
                                {1, "flags"},
                                {1, "Unable to read u8"}},
                               MakeSpanU8("\x70"));
}

TEST(ReaderTest, ReadMemoryType) {
  ExpectRead<MemoryType>(MemoryType{Limits{1}}, MakeSpanU8("\x00\x01"));
  ExpectRead<MemoryType>(MemoryType{Limits{0, 128}},
                         MakeSpanU8("\x01\x00\x80\x01"));
}

TEST(ReaderTest, ReadMemoryType_PastEnd) {
  ExpectReadFailure<MemoryType>({{0, "memory type"},
                                 {0, "limits"},
                                 {0, "flags"},
                                 {0, "Unable to read u8"}},
                                MakeSpanU8(""));
}

TEST(ReaderTest, ReadGlobalType) {
  ExpectRead<GlobalType>(GlobalType{ValueType::I32, Mutability::Const},
                         MakeSpanU8("\x7f\x00"));
  ExpectRead<GlobalType>(GlobalType{ValueType::F32, Mutability::Var},
                         MakeSpanU8("\x7d\x01"));
}

TEST(ReaderTest, ReadGlobalType_PastEnd) {
  ExpectReadFailure<GlobalType>(
      {{0, "global type"}, {0, "value type"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<GlobalType>(
      {{0, "global type"}, {1, "mutability"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x7f"));
}

TEST(ReaderTest, ReadBrTableImmediate) {
  ExpectRead<BrTableImmediate>(BrTableImmediate{{}, 0}, MakeSpanU8("\x00\x00"));
  ExpectRead<BrTableImmediate>(BrTableImmediate{{1, 2}, 3},
                               MakeSpanU8("\x02\x01\x02\x03"));
}

TEST(ReaderTest, ReadBrTableImmediate_PastEnd) {
  ExpectReadFailure<BrTableImmediate>(
      {{0, "br_table"}, {0, "targets"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<BrTableImmediate>({{0, "br_table"},
                                       {1, "default target"},
                                       {1, "index"},
                                       {1, "Unable to read u8"}},
                                      MakeSpanU8("\x00"));
}

TEST(ReaderTest, ReadCallIndirectImmediate) {
  ExpectRead<CallIndirectImmediate>(CallIndirectImmediate{1, 0},
                                    MakeSpanU8("\x01\x00"));
  ExpectRead<CallIndirectImmediate>(CallIndirectImmediate{128, 0},
                                    MakeSpanU8("\x80\x01\x00"));
}

TEST(ReaderTest, ReadCallIndirectImmediate_BadReserved) {
  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"},
       {1, "reserved"},
       {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x00\x01"));
}

TEST(ReaderTest, ReadCallIndirectImmediate_PastEnd) {
  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<CallIndirectImmediate>(
      {{0, "call_indirect"}, {1, "reserved"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));
}

TEST(ReaderTest, ReadImport) {
  ExpectRead<Import<>>(Import<>{"a", "func", 11u},
                       MakeSpanU8("\x01\x61\x04\x66unc\x00\x0b"));

  ExpectRead<Import<>>(
      Import<>{"b", "table", TableType{Limits{1}, ElementType::Funcref}},
      MakeSpanU8("\x01\x62\x05table\x01\x70\x00\x01"));

  ExpectRead<Import<>>(Import<>{"c", "memory", MemoryType{Limits{0, 2}}},
                       MakeSpanU8("\x01\x63\x06memory\x02\x01\x00\x02"));

  ExpectRead<Import<>>(
      Import<>{"d", "global", GlobalType{ValueType::I32, Mutability::Const}},
      MakeSpanU8("\x01\x64\x06global\x03\x7f\x00"));
}

TEST(ReaderTest, ReadImportType_PastEnd) {
  ExpectReadFailure<Import<>>({{0, "import"},
                               {0, "module name"},
                               {0, "index"},
                               {0, "Unable to read u8"}},
                              MakeSpanU8(""));

  ExpectReadFailure<Import<>>({{0, "import"},
                               {1, "field name"},
                               {1, "index"},
                               {1, "Unable to read u8"}},
                              MakeSpanU8("\x00"));

  ExpectReadFailure<Import<>>(
      {{0, "import"}, {2, "external kind"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x00\x00"));

  ExpectReadFailure<Import<>>(
      {{0, "import"}, {3, "index"}, {3, "Unable to read u8"}},
      MakeSpanU8("\x00\x00\x00"));

  ExpectReadFailure<Import<>>({{0, "import"},
                               {3, "table type"},
                               {3, "element type"},
                               {3, "Unable to read u8"}},
                              MakeSpanU8("\x00\x00\x01"));

  ExpectReadFailure<Import<>>({{0, "import"},
                               {3, "memory type"},
                               {3, "limits"},
                               {3, "flags"},
                               {3, "Unable to read u8"}},
                              MakeSpanU8("\x00\x00\x02"));

  ExpectReadFailure<Import<>>({{0, "import"},
                               {3, "global type"},
                               {3, "value type"},
                               {3, "Unable to read u8"}},
                              MakeSpanU8("\x00\x00\x03"));
}

TEST(ReaderTest, ReadConstantExpression) {
  // i32.const
  {
    const auto data = MakeSpanU8("\x41\x00\x0b");
    ExpectRead<ConstantExpression<>>(ConstantExpression<>{data}, data);
  }

  // i64.const
  {
    const auto data = MakeSpanU8("\x42\x80\x80\x80\x80\x80\x01\x0b");
    ExpectRead<ConstantExpression<>>(ConstantExpression<>{data}, data);
  }

  // f32.const
  {
    const auto data = MakeSpanU8("\x43\x00\x00\x00\x00\x0b");
    ExpectRead<ConstantExpression<>>(ConstantExpression<>{data}, data);
  }

  // f64.const
  {
    const auto data = MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00\x0b");
    ExpectRead<ConstantExpression<>>(ConstantExpression<>{data}, data);
  }

  // get_global
  {
    const auto data = MakeSpanU8("\x23\x00\x0b");
    ExpectRead<ConstantExpression<>>(ConstantExpression<>{data}, data);
  }
}

TEST(ReaderTest, ReadConstantExpression_NoEnd) {
  // i32.const
  ExpectReadFailure<ConstantExpression<>>({{0, "Expected end instruction"}},
                                          MakeSpanU8("\x41\x00"));

  // i64.const
  ExpectReadFailure<ConstantExpression<>>(
      {{0, "Expected end instruction"}},
      MakeSpanU8("\x42\x80\x80\x80\x80\x80\x01"));

  // f32.const
  ExpectReadFailure<ConstantExpression<>>({{0, "Expected end instruction"}},
                                          MakeSpanU8("\x43\x00\x00\x00\x00"));

  // f64.const
  ExpectReadFailure<ConstantExpression<>>(
      {{0, "Expected end instruction"}},
      MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00"));

  // get_global
  ExpectReadFailure<ConstantExpression<>>({{0, "Expected end instruction"}},
                                          MakeSpanU8("\x23\x00"));
}

TEST(ReaderTest, ReadConstantExpression_TooLong) {
  ExpectReadFailure<ConstantExpression<>>({{0, "Expected end instruction"}},
                                          MakeSpanU8("\x41\x00\x01\x0b"));
}

TEST(ReaderTest, ReadConstantExpression_InvalidInstruction) {
  TestErrors errors;
  auto data = MakeSpanU8("\x06");
  const SpanU8 orig_data = data;
  auto result = Read<ConstantExpression<>>(&data, errors);
  ExpectErrors({{{0, "opcode"}, {1, "Unknown opcode: 6"}},
                {{0, "Unexpected end of constant expression"}}},
               errors, orig_data);
  ExpectEmptyOptional(result);
}

TEST(ReaderTest, ReadConstantExpression_IllegalInstruction) {
  ExpectReadFailure<ConstantExpression<>>(
      {{0, "Illegal instruction in constant expression: unreachable"}},
      MakeSpanU8("\x00"));
}

TEST(ReaderTest, ReadConstantExpression_PastEnd) {
  ExpectReadFailure<ConstantExpression<>>(
      {{0, "Unexpected end of constant expression"}}, MakeSpanU8(""));
}

TEST(ReaderTest, ReadInstruction) {
  using I = Instruction;
  using O = Opcode;

  ExpectRead<I>(I{O::Unreachable}, MakeSpanU8("\x00"));
  ExpectRead<I>(I{O::Nop}, MakeSpanU8("\x01"));
  ExpectRead<I>(I{O::Block, BlockType::I32}, MakeSpanU8("\x02\x7f"));
  ExpectRead<I>(I{O::Loop, BlockType::Void}, MakeSpanU8("\x03\x40"));
  ExpectRead<I>(I{O::If, BlockType::F64}, MakeSpanU8("\x04\x7c"));
  ExpectRead<I>(I{O::Else}, MakeSpanU8("\x05"));
  ExpectRead<I>(I{O::End}, MakeSpanU8("\x0b"));
  ExpectRead<I>(I{O::Br, Index{1}}, MakeSpanU8("\x0c\x01"));
  ExpectRead<I>(I{O::BrIf, Index{2}}, MakeSpanU8("\x0d\x02"));
  ExpectRead<I>(I{O::BrTable, BrTableImmediate{{3, 4, 5}, 6}},
                MakeSpanU8("\x0e\x03\x03\x04\x05\x06"));
  ExpectRead<I>(I{O::Return}, MakeSpanU8("\x0f"));
  ExpectRead<I>(I{O::Call, Index{7}}, MakeSpanU8("\x10\x07"));
  ExpectRead<I>(I{O::CallIndirect, CallIndirectImmediate{8, 0}},
                MakeSpanU8("\x11\x08\x00"));
  ExpectRead<I>(I{O::Drop}, MakeSpanU8("\x1a"));
  ExpectRead<I>(I{O::Select}, MakeSpanU8("\x1b"));
  ExpectRead<I>(I{O::GetLocal, Index{5}}, MakeSpanU8("\x20\x05"));
  ExpectRead<I>(I{O::SetLocal, Index{6}}, MakeSpanU8("\x21\x06"));
  ExpectRead<I>(I{O::TeeLocal, Index{7}}, MakeSpanU8("\x22\x07"));
  ExpectRead<I>(I{O::GetGlobal, Index{8}}, MakeSpanU8("\x23\x08"));
  ExpectRead<I>(I{O::SetGlobal, Index{9}}, MakeSpanU8("\x24\x09"));
  ExpectRead<I>(I{O::I32Load, MemArg{10, 11}}, MakeSpanU8("\x28\x0a\x0b"));
  ExpectRead<I>(I{O::I64Load, MemArg{12, 13}}, MakeSpanU8("\x29\x0c\x0d"));
  ExpectRead<I>(I{O::F32Load, MemArg{14, 15}}, MakeSpanU8("\x2a\x0e\x0f"));
  ExpectRead<I>(I{O::F64Load, MemArg{16, 17}}, MakeSpanU8("\x2b\x10\x11"));
  ExpectRead<I>(I{O::I32Load8S, MemArg{18, 19}}, MakeSpanU8("\x2c\x12\x13"));
  ExpectRead<I>(I{O::I32Load8U, MemArg{20, 21}}, MakeSpanU8("\x2d\x14\x15"));
  ExpectRead<I>(I{O::I32Load16S, MemArg{22, 23}}, MakeSpanU8("\x2e\x16\x17"));
  ExpectRead<I>(I{O::I32Load16U, MemArg{24, 25}}, MakeSpanU8("\x2f\x18\x19"));
  ExpectRead<I>(I{O::I64Load8S, MemArg{26, 27}}, MakeSpanU8("\x30\x1a\x1b"));
  ExpectRead<I>(I{O::I64Load8U, MemArg{28, 29}}, MakeSpanU8("\x31\x1c\x1d"));
  ExpectRead<I>(I{O::I64Load16S, MemArg{30, 31}}, MakeSpanU8("\x32\x1e\x1f"));
  ExpectRead<I>(I{O::I64Load16U, MemArg{32, 33}}, MakeSpanU8("\x33\x20\x21"));
  ExpectRead<I>(I{O::I64Load32S, MemArg{34, 35}}, MakeSpanU8("\x34\x22\x23"));
  ExpectRead<I>(I{O::I64Load32U, MemArg{36, 37}}, MakeSpanU8("\x35\x24\x25"));
  ExpectRead<I>(I{O::I32Store, MemArg{38, 39}}, MakeSpanU8("\x36\x26\x27"));
  ExpectRead<I>(I{O::I64Store, MemArg{40, 41}}, MakeSpanU8("\x37\x28\x29"));
  ExpectRead<I>(I{O::F32Store, MemArg{42, 43}}, MakeSpanU8("\x38\x2a\x2b"));
  ExpectRead<I>(I{O::F64Store, MemArg{44, 45}}, MakeSpanU8("\x39\x2c\x2d"));
  ExpectRead<I>(I{O::I32Store8, MemArg{46, 47}}, MakeSpanU8("\x3a\x2e\x2f"));
  ExpectRead<I>(I{O::I32Store16, MemArg{48, 49}}, MakeSpanU8("\x3b\x30\x31"));
  ExpectRead<I>(I{O::I64Store8, MemArg{50, 51}}, MakeSpanU8("\x3c\x32\x33"));
  ExpectRead<I>(I{O::I64Store16, MemArg{52, 53}}, MakeSpanU8("\x3d\x34\x35"));
  ExpectRead<I>(I{O::I64Store32, MemArg{54, 55}}, MakeSpanU8("\x3e\x36\x37"));
  ExpectRead<I>(I{O::MemorySize, u8{0}}, MakeSpanU8("\x3f\x00"));
  ExpectRead<I>(I{O::MemoryGrow, u8{0}}, MakeSpanU8("\x40\x00"));
  ExpectRead<I>(I{O::I32Const, s32{0}}, MakeSpanU8("\x41\x00"));
  ExpectRead<I>(I{O::I64Const, s64{0}}, MakeSpanU8("\x42\x00"));
  ExpectRead<I>(I{O::F32Const, f32{0}}, MakeSpanU8("\x43\x00\x00\x00\x00"));
  ExpectRead<I>(I{O::F64Const, f64{0}},
                MakeSpanU8("\x44\x00\x00\x00\x00\x00\x00\x00\x00"));
  ExpectRead<I>(I{O::I32Eqz}, MakeSpanU8("\x45"));
  ExpectRead<I>(I{O::I32Eq}, MakeSpanU8("\x46"));
  ExpectRead<I>(I{O::I32Ne}, MakeSpanU8("\x47"));
  ExpectRead<I>(I{O::I32LtS}, MakeSpanU8("\x48"));
  ExpectRead<I>(I{O::I32LtU}, MakeSpanU8("\x49"));
  ExpectRead<I>(I{O::I32GtS}, MakeSpanU8("\x4a"));
  ExpectRead<I>(I{O::I32GtU}, MakeSpanU8("\x4b"));
  ExpectRead<I>(I{O::I32LeS}, MakeSpanU8("\x4c"));
  ExpectRead<I>(I{O::I32LeU}, MakeSpanU8("\x4d"));
  ExpectRead<I>(I{O::I32GeS}, MakeSpanU8("\x4e"));
  ExpectRead<I>(I{O::I32GeU}, MakeSpanU8("\x4f"));
  ExpectRead<I>(I{O::I64Eqz}, MakeSpanU8("\x50"));
  ExpectRead<I>(I{O::I64Eq}, MakeSpanU8("\x51"));
  ExpectRead<I>(I{O::I64Ne}, MakeSpanU8("\x52"));
  ExpectRead<I>(I{O::I64LtS}, MakeSpanU8("\x53"));
  ExpectRead<I>(I{O::I64LtU}, MakeSpanU8("\x54"));
  ExpectRead<I>(I{O::I64GtS}, MakeSpanU8("\x55"));
  ExpectRead<I>(I{O::I64GtU}, MakeSpanU8("\x56"));
  ExpectRead<I>(I{O::I64LeS}, MakeSpanU8("\x57"));
  ExpectRead<I>(I{O::I64LeU}, MakeSpanU8("\x58"));
  ExpectRead<I>(I{O::I64GeS}, MakeSpanU8("\x59"));
  ExpectRead<I>(I{O::I64GeU}, MakeSpanU8("\x5a"));
  ExpectRead<I>(I{O::F32Eq}, MakeSpanU8("\x5b"));
  ExpectRead<I>(I{O::F32Ne}, MakeSpanU8("\x5c"));
  ExpectRead<I>(I{O::F32Lt}, MakeSpanU8("\x5d"));
  ExpectRead<I>(I{O::F32Gt}, MakeSpanU8("\x5e"));
  ExpectRead<I>(I{O::F32Le}, MakeSpanU8("\x5f"));
  ExpectRead<I>(I{O::F32Ge}, MakeSpanU8("\x60"));
  ExpectRead<I>(I{O::F64Eq}, MakeSpanU8("\x61"));
  ExpectRead<I>(I{O::F64Ne}, MakeSpanU8("\x62"));
  ExpectRead<I>(I{O::F64Lt}, MakeSpanU8("\x63"));
  ExpectRead<I>(I{O::F64Gt}, MakeSpanU8("\x64"));
  ExpectRead<I>(I{O::F64Le}, MakeSpanU8("\x65"));
  ExpectRead<I>(I{O::F64Ge}, MakeSpanU8("\x66"));
  ExpectRead<I>(I{O::I32Clz}, MakeSpanU8("\x67"));
  ExpectRead<I>(I{O::I32Ctz}, MakeSpanU8("\x68"));
  ExpectRead<I>(I{O::I32Popcnt}, MakeSpanU8("\x69"));
  ExpectRead<I>(I{O::I32Add}, MakeSpanU8("\x6a"));
  ExpectRead<I>(I{O::I32Sub}, MakeSpanU8("\x6b"));
  ExpectRead<I>(I{O::I32Mul}, MakeSpanU8("\x6c"));
  ExpectRead<I>(I{O::I32DivS}, MakeSpanU8("\x6d"));
  ExpectRead<I>(I{O::I32DivU}, MakeSpanU8("\x6e"));
  ExpectRead<I>(I{O::I32RemS}, MakeSpanU8("\x6f"));
  ExpectRead<I>(I{O::I32RemU}, MakeSpanU8("\x70"));
  ExpectRead<I>(I{O::I32And}, MakeSpanU8("\x71"));
  ExpectRead<I>(I{O::I32Or}, MakeSpanU8("\x72"));
  ExpectRead<I>(I{O::I32Xor}, MakeSpanU8("\x73"));
  ExpectRead<I>(I{O::I32Shl}, MakeSpanU8("\x74"));
  ExpectRead<I>(I{O::I32ShrS}, MakeSpanU8("\x75"));
  ExpectRead<I>(I{O::I32ShrU}, MakeSpanU8("\x76"));
  ExpectRead<I>(I{O::I32Rotl}, MakeSpanU8("\x77"));
  ExpectRead<I>(I{O::I32Rotr}, MakeSpanU8("\x78"));
  ExpectRead<I>(I{O::I64Clz}, MakeSpanU8("\x79"));
  ExpectRead<I>(I{O::I64Ctz}, MakeSpanU8("\x7a"));
  ExpectRead<I>(I{O::I64Popcnt}, MakeSpanU8("\x7b"));
  ExpectRead<I>(I{O::I64Add}, MakeSpanU8("\x7c"));
  ExpectRead<I>(I{O::I64Sub}, MakeSpanU8("\x7d"));
  ExpectRead<I>(I{O::I64Mul}, MakeSpanU8("\x7e"));
  ExpectRead<I>(I{O::I64DivS}, MakeSpanU8("\x7f"));
  ExpectRead<I>(I{O::I64DivU}, MakeSpanU8("\x80"));
  ExpectRead<I>(I{O::I64RemS}, MakeSpanU8("\x81"));
  ExpectRead<I>(I{O::I64RemU}, MakeSpanU8("\x82"));
  ExpectRead<I>(I{O::I64And}, MakeSpanU8("\x83"));
  ExpectRead<I>(I{O::I64Or}, MakeSpanU8("\x84"));
  ExpectRead<I>(I{O::I64Xor}, MakeSpanU8("\x85"));
  ExpectRead<I>(I{O::I64Shl}, MakeSpanU8("\x86"));
  ExpectRead<I>(I{O::I64ShrS}, MakeSpanU8("\x87"));
  ExpectRead<I>(I{O::I64ShrU}, MakeSpanU8("\x88"));
  ExpectRead<I>(I{O::I64Rotl}, MakeSpanU8("\x89"));
  ExpectRead<I>(I{O::I64Rotr}, MakeSpanU8("\x8a"));
  ExpectRead<I>(I{O::F32Abs}, MakeSpanU8("\x8b"));
  ExpectRead<I>(I{O::F32Neg}, MakeSpanU8("\x8c"));
  ExpectRead<I>(I{O::F32Ceil}, MakeSpanU8("\x8d"));
  ExpectRead<I>(I{O::F32Floor}, MakeSpanU8("\x8e"));
  ExpectRead<I>(I{O::F32Trunc}, MakeSpanU8("\x8f"));
  ExpectRead<I>(I{O::F32Nearest}, MakeSpanU8("\x90"));
  ExpectRead<I>(I{O::F32Sqrt}, MakeSpanU8("\x91"));
  ExpectRead<I>(I{O::F32Add}, MakeSpanU8("\x92"));
  ExpectRead<I>(I{O::F32Sub}, MakeSpanU8("\x93"));
  ExpectRead<I>(I{O::F32Mul}, MakeSpanU8("\x94"));
  ExpectRead<I>(I{O::F32Div}, MakeSpanU8("\x95"));
  ExpectRead<I>(I{O::F32Min}, MakeSpanU8("\x96"));
  ExpectRead<I>(I{O::F32Max}, MakeSpanU8("\x97"));
  ExpectRead<I>(I{O::F32Copysign}, MakeSpanU8("\x98"));
  ExpectRead<I>(I{O::F64Abs}, MakeSpanU8("\x99"));
  ExpectRead<I>(I{O::F64Neg}, MakeSpanU8("\x9a"));
  ExpectRead<I>(I{O::F64Ceil}, MakeSpanU8("\x9b"));
  ExpectRead<I>(I{O::F64Floor}, MakeSpanU8("\x9c"));
  ExpectRead<I>(I{O::F64Trunc}, MakeSpanU8("\x9d"));
  ExpectRead<I>(I{O::F64Nearest}, MakeSpanU8("\x9e"));
  ExpectRead<I>(I{O::F64Sqrt}, MakeSpanU8("\x9f"));
  ExpectRead<I>(I{O::F64Add}, MakeSpanU8("\xa0"));
  ExpectRead<I>(I{O::F64Sub}, MakeSpanU8("\xa1"));
  ExpectRead<I>(I{O::F64Mul}, MakeSpanU8("\xa2"));
  ExpectRead<I>(I{O::F64Div}, MakeSpanU8("\xa3"));
  ExpectRead<I>(I{O::F64Min}, MakeSpanU8("\xa4"));
  ExpectRead<I>(I{O::F64Max}, MakeSpanU8("\xa5"));
  ExpectRead<I>(I{O::F64Copysign}, MakeSpanU8("\xa6"));
  ExpectRead<I>(I{O::I32WrapI64}, MakeSpanU8("\xa7"));
  ExpectRead<I>(I{O::I32TruncSF32}, MakeSpanU8("\xa8"));
  ExpectRead<I>(I{O::I32TruncUF32}, MakeSpanU8("\xa9"));
  ExpectRead<I>(I{O::I32TruncSF64}, MakeSpanU8("\xaa"));
  ExpectRead<I>(I{O::I32TruncUF64}, MakeSpanU8("\xab"));
  ExpectRead<I>(I{O::I64ExtendSI32}, MakeSpanU8("\xac"));
  ExpectRead<I>(I{O::I64ExtendUI32}, MakeSpanU8("\xad"));
  ExpectRead<I>(I{O::I64TruncSF32}, MakeSpanU8("\xae"));
  ExpectRead<I>(I{O::I64TruncUF32}, MakeSpanU8("\xaf"));
  ExpectRead<I>(I{O::I64TruncSF64}, MakeSpanU8("\xb0"));
  ExpectRead<I>(I{O::I64TruncUF64}, MakeSpanU8("\xb1"));
  ExpectRead<I>(I{O::F32ConvertSI32}, MakeSpanU8("\xb2"));
  ExpectRead<I>(I{O::F32ConvertUI32}, MakeSpanU8("\xb3"));
  ExpectRead<I>(I{O::F32ConvertSI64}, MakeSpanU8("\xb4"));
  ExpectRead<I>(I{O::F32ConvertUI64}, MakeSpanU8("\xb5"));
  ExpectRead<I>(I{O::F32DemoteF64}, MakeSpanU8("\xb6"));
  ExpectRead<I>(I{O::F64ConvertSI32}, MakeSpanU8("\xb7"));
  ExpectRead<I>(I{O::F64ConvertUI32}, MakeSpanU8("\xb8"));
  ExpectRead<I>(I{O::F64ConvertSI64}, MakeSpanU8("\xb9"));
  ExpectRead<I>(I{O::F64ConvertUI64}, MakeSpanU8("\xba"));
  ExpectRead<I>(I{O::F64PromoteF32}, MakeSpanU8("\xbb"));
  ExpectRead<I>(I{O::I32ReinterpretF32}, MakeSpanU8("\xbc"));
  ExpectRead<I>(I{O::I64ReinterpretF64}, MakeSpanU8("\xbd"));
  ExpectRead<I>(I{O::F32ReinterpretI32}, MakeSpanU8("\xbe"));
  ExpectRead<I>(I{O::F64ReinterpretI64}, MakeSpanU8("\xbf"));
}

TEST(ReaderTest, ReadInstruction_BadMemoryReserved) {
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x3f\x01"));
  ExpectReadFailure<Instruction>(
      {{1, "reserved"}, {2, "Expected reserved byte 0, got 1"}},
      MakeSpanU8("\x40\x01"));
}

TEST(ReaderTest, ReadFunc) {
  ExpectRead<Function>(Function{1}, MakeSpanU8("\x01"));
}

TEST(ReaderTest, ReadFunc_PastEnd) {
  ExpectReadFailure<Function>(
      {{0, "function"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));
}

TEST(ReaderTest, ReadTable) {
  ExpectRead<Table>(Table{TableType{Limits{1}, ElementType::Funcref}},
                    MakeSpanU8("\x70\x00\x01"));
}

TEST(ReaderTest, ReadTable_PastEnd) {
  ExpectReadFailure<Table>({{0, "table"},
                            {0, "table type"},
                            {0, "element type"},
                            {0, "Unable to read u8"}},
                           MakeSpanU8(""));
}

TEST(ReaderTest, ReadMemory) {
  ExpectRead<Memory>(Memory{MemoryType{Limits{1, 2}}},
                     MakeSpanU8("\x01\x01\x02"));
}

TEST(ReaderTest, ReadMemory_PastEnd) {
  ExpectReadFailure<Memory>({{0, "memory"},
                             {0, "memory type"},
                             {0, "limits"},
                             {0, "flags"},
                             {0, "Unable to read u8"}},
                            MakeSpanU8(""));
}

TEST(ReaderTest, ReadGlobal) {
  // i32 global with i64.const constant expression. This will fail validation
  // but still can be successfully parsed.
  ExpectRead<Global<>>(
      Global<>{GlobalType{ValueType::I32, Mutability::Var},
               ConstantExpression<>{MakeSpanU8("\x42\x00\x0b")}},
      MakeSpanU8("\x7f\x01\x42\x00\x0b"));
}

TEST(ReaderTest, ReadGlobal_PastEnd) {
  ExpectReadFailure<Global<>>({{0, "global"},
                               {0, "global type"},
                               {0, "value type"},
                               {0, "Unable to read u8"}},
                              MakeSpanU8(""));

  ExpectReadFailure<Global<>>({{0, "global"},
                               {2, "Unexpected end of constant expression"}},
                              MakeSpanU8("\x7f\x00"));
}

TEST(ReaderTest, ReadExport) {
  ExpectRead<Export<>>(Export<>{ExternalKind::Function, "hi", 3},
                       MakeSpanU8("\x02hi\x00\x03"));
  ExpectRead<Export<>>(Export<>{ExternalKind::Table, "", 1000},
                       MakeSpanU8("\x00\x01\xe8\x07"));
  ExpectRead<Export<>>(Export<>{ExternalKind::Memory, "mem", 0},
                       MakeSpanU8("\x03mem\x02\x00"));
  ExpectRead<Export<>>(Export<>{ExternalKind::Global, "g", 1},
                       MakeSpanU8("\x01g\x03\x01"));
}

TEST(ReaderTest, ReadExport_PastEnd) {
  ExpectReadFailure<Export<>>(
      {{0, "export"}, {0, "name"}, {0, "index"}, {0, "Unable to read u8"}},
      MakeSpanU8(""));

  ExpectReadFailure<Export<>>(
      {{0, "export"}, {1, "external kind"}, {1, "Unable to read u8"}},
      MakeSpanU8("\x00"));

  ExpectReadFailure<Export<>>(
      {{0, "export"}, {2, "index"}, {2, "Unable to read u8"}},
      MakeSpanU8("\x00\x00"));
}

TEST(ReaderTest, ReadStart) {
  ExpectRead<Start>(Start{256}, MakeSpanU8("\x80\x02"));
}
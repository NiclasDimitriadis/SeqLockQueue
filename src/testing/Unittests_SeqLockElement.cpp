#define DOCTEST_CONFIG_IMPLEMENT

#include <optional>
#include <tuple>

#include "Element.hpp"

#include "doctest.h"

TEST_CASE("testing SeqLockElement::SeqLockElement with undefined behavior through data-race") {
  using testElementClass = Element::SeqLockElement<SLQ_Auxil::atomic_arr_copy_standin<std::uint8_t>, 8>;
  testElementClass testElement;
  CHECK(!std::get<0>(testElement.read(1)).has_value());
  testElement.insert(123);
  auto readRet = testElement.read(1);
  CHECK(std::get<0>(readRet).value() == 123);
  CHECK(std::get<1>(readRet) == 2);
}

TEST_CASE("testing SeqLockElement::SeqLockElement eliminating undefined behavior") {
  using testElementClass = Element::SeqLockElement<SLQ_Auxil::atomic_arr_copy_t<std::uint8_t>, 8>;
  testElementClass testElement;
  CHECK(!std::get<0>(testElement.read(1)).has_value());
  testElement.insert(123);
  auto readRet = testElement.read(1);
  CHECK(std::get<0>(readRet).value() == 123);
  CHECK(std::get<1>(readRet) == 2);
}

int main() {
  doctest::Context context;
  context.run();
}

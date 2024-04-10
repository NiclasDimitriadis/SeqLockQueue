#define DOCTEST_CONFIG_IMPLEMENT

#include <cstdint>
#include <tuple>

#include "doctest.h"

#include "SLQ_Auxil.hpp"

struct TestTuple{
    std::uint64_t ui64_1;
    std::uint64_t ui64_2;
    std::uint64_t ui64_3;
    std::uint8_t ui8_1;
    std::uint8_t ui8_2;
    std::uint8_t ui8_3;
    bool operator==(TestTuple other){
        return this->ui64_1 == other.ui64_1
        && this->ui64_2 == other.ui64_2
        && this->ui64_3 == other.ui64_3
        && this->ui8_1 == other.ui8_1
        && this->ui8_2 == other.ui8_2
        && this->ui8_3 == other.ui8_3;
    };
};

using TestAtomicArrCopy = SLQ_Auxil::atomic_arr_copy_t<TestTuple>;
using TestAtomicArrCopyStandin = SLQ_Auxil::atomic_arr_copy_standin<TestTuple>;

TEST_CASE("testing SLQ_Auxil"){
    SUBCASE("testing SLQ_Auxil::atomic_arr_copy"){
        auto test_tuple = TestTuple{1,2,3,4,5,6};
        auto test_atomic_arr_copy_1 = TestAtomicArrCopy(test_tuple);
        TestAtomicArrCopy test_atomic_arr_copy_2;
        test_atomic_arr_copy_2 = test_atomic_arr_copy_1;
        CHECK(static_cast<TestTuple>(test_atomic_arr_copy_1) == test_tuple);
        CHECK(static_cast<TestTuple>(test_atomic_arr_copy_2) == test_tuple);
    };

    SUBCASE("testing SLQ_Auxil::atomic_arr_copy_standin"){
        auto test_tuple = TestTuple{1,2,3,4,5,6};
        auto test_atomic_arr_copy_1 = TestAtomicArrCopyStandin(test_tuple);
        TestAtomicArrCopyStandin test_atomic_arr_copy_2;
        test_atomic_arr_copy_2 = test_atomic_arr_copy_1;
        CHECK(static_cast<TestTuple>(test_atomic_arr_copy_1) == test_tuple);
        CHECK(static_cast<TestTuple>(test_atomic_arr_copy_2) == test_tuple);
    };
};

int main() {
  doctest::Context context;
  context.run();
}

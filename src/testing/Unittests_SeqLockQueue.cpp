#define DOCTEST_CONFIG_IMPLEMENT

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <thread>

#include "doctest.h"

#include "Queue.hpp"

struct test_class_12bytes{
  int i1;
  int i2;
  int i3;
  std::uint64_t get_sum() const noexcept{
    return i1 + i2 + i3;
  };
  test_class_12bytes(): i1{std::rand()}, i2{std::rand()}, i3{std::rand()}{};
};

TEST_CASE("testing Queue::SeqLockQueue") {
  SUBCASE("testing enqueueing and dequeueing with shared cachline") {
    using slqClass = Queue::SeqLockQueue<int, 8, true, false>;
    slqClass testSlq;
    auto testReader = testSlq.get_reader();
    CHECK(!testReader.read_next_entry().has_value());
    int enqSum = 0;
    for (int i = 0; i < 8; ++i) {
      testSlq.enqueue(i);
      enqSum += i;
    };
    auto deqRes = testReader.read_next_entry();
    CHECK(deqRes.value() == 0);
    int deqSum = 0;
    for (int i = 0; i < 7; ++i) {
      deqSum += testReader.read_next_entry().value();
    };
    CHECK(enqSum == deqSum);
    testSlq.enqueue(123);
    CHECK(testReader.read_next_entry().value() == 123);
  }

  SUBCASE("testing enqueueing and dequeueing with shared cacheline") {
    using slqClass = Queue::SeqLockQueue<int, 4, false, false>;
    slqClass testSlq{};
    auto testReader = testSlq.get_reader();
    CHECK(!testReader.read_next_entry().has_value());
    int enqSum = 0;
    for (int i = 0; i < 4; ++i) {
      testSlq.enqueue(i);
      enqSum += i;
    };
    auto deqRes = testReader.read_next_entry();
    CHECK(deqRes.value() == 0);
    int deqSum = 0;
    for (int i = 0; i < 3; ++i) {
      deqSum += testReader.read_next_entry().value();
    };
    CHECK(enqSum == deqSum);
    testSlq.enqueue(123);
    CHECK(testReader.read_next_entry().value() == 123);
  }

  SUBCASE(
      "testing for correct behavior under concurrent enqueueing and dequeueing with UB") {
    static constexpr std::uint32_t nElements = 128 * 1048576;
    using slqClass = Queue::SeqLockQueue<int, nElements, true, true>;
    slqClass testSlq;
    std::int64_t enqSum{0}, deqSum1{0}, deqSum2{0};
    std::atomic_flag startSignal{false};

    std::thread enqThread([&]() {
      std::srand(std::time(nullptr));
      int randInt{0};
      while (!startSignal.test())
        ;
      for (int i = 0; i < nElements; ++i) {
        randInt = std::rand();
        testSlq.enqueue(randInt);
        enqSum += randInt;
      }
    });

    std::thread deqThread1([&]() {
      std::optional<int> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum1 += deqRes.value();
          ++nIter;
        }
      }
    });

    std::thread deqThread2([&]() {
      std::optional<int> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum2 += deqRes.value();
          ++nIter;
        }
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    startSignal.test_and_set();
    enqThread.join();
    deqThread1.join();
    deqThread2.join();

    CHECK(enqSum == deqSum1);
    CHECK(enqSum == deqSum2);
  }

    SUBCASE(
      "testing for correct behavior under concurrent enqueueing and dequeueing without UB") {
    static constexpr std::uint32_t nElements = 524288;
    using slqClass = Queue::SeqLockQueue<int, nElements, true, false>;
    slqClass testSlq;
    std::uint64_t enqSum{0};
    alignas(64) std::uint64_t deqSum1{0};
    alignas(64) std::uint64_t deqSum2{0};
    alignas(64) std::atomic_flag startSignal{false};

    std::thread enqThread([&]() {
      std::srand(std::time(nullptr));
      int randInt{0};
      while (!startSignal.test())
        ;
      for (int i = 0; i < nElements; ++i) {
        randInt = std::rand();
        testSlq.enqueue(randInt);
        enqSum += randInt;
      }
    });

    std::thread deqThread1([&]() {
      std::optional<int> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum1 += deqRes.value();
          ++nIter;
        }
      }
    });

    std::thread deqThread2([&]() {
      std::optional<int> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum2 += deqRes.value();
          ++nIter;
        }
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    startSignal.test_and_set();
    enqThread.join();
    deqThread1.join();
    deqThread2.join();

    CHECK(enqSum == deqSum1);
    CHECK(enqSum == deqSum2);
  }


SUBCASE(
      "testing for correct behavior under concurrent enqueueing and dequeueing without UB using 64-bit integers") {
    static constexpr std::uint32_t nElements = 524288;
    using slqClass = Queue::SeqLockQueue<std::uint64_t, nElements, true, false>;
    slqClass testSlq;
    std::uint64_t enqSum{0};
    alignas(64) std::uint64_t deqSum1{0};
    alignas(64) std::uint64_t deqSum2{0};
    alignas(64) std::atomic_flag startSignal{false};

    std::thread enqThread([&]() {
      std::srand(std::time(nullptr));
      std::uint64_t randInt{0};
      while (!startSignal.test())
        ;
      for (int i = 0; i < nElements; ++i) {
        randInt = std::rand();
        testSlq.enqueue(randInt);
        enqSum += randInt;
      }
    });

    std::thread deqThread1([&]() {
      std::optional<std::uint64_t> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum1 += deqRes.value();
          ++nIter;
        }
      }
    });

    std::thread deqThread2([&]() {
      std::optional<std::uint64_t> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum2 += deqRes.value();
          ++nIter;
        }
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    startSignal.test_and_set();
    enqThread.join();
    deqThread1.join();
    deqThread2.join();

    CHECK(enqSum == deqSum1);
    CHECK(enqSum == deqSum2);
  }

SUBCASE(
      "testing for correct behavior under concurrent enqueueing and dequeueing with copying, both byte-wise and in 8-byte chunks") {
    static constexpr std::uint32_t nElements = 524288;
    using slqClass = Queue::SeqLockQueue<test_class_12bytes, nElements, true, false>;
    slqClass testSlq;
    std::uint64_t enqSum{0};
    alignas(64) std::uint64_t deqSum1{0};
    alignas(64) std::uint64_t deqSum2{0};
    alignas(64) std::atomic_flag startSignal{false};

    std::thread enqThread([&]() {
      std::srand(std::time(nullptr));
      while (!startSignal.test())
        ;
      for (int i = 0; i < nElements; ++i) {
        const test_class_12bytes randObject;
        testSlq.enqueue(randObject);
        enqSum += randObject.get_sum();
      }
    });

    std::thread deqThread1([&]() {
      std::optional<test_class_12bytes> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum1 += deqRes.value().get_sum();
          ++nIter;
        }
      }
    });

    std::thread deqThread2([&]() {
      std::optional<test_class_12bytes> deqRes;
      int nIter{0};
      auto testReader = testSlq.get_reader();
      while (!startSignal.test());
      while (nIter < nElements) {
        deqRes = testReader.read_next_entry();
        if (deqRes.has_value()) {
          deqSum2 += deqRes.value().get_sum();
          ++nIter;
        }
      }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    startSignal.test_and_set();
    enqThread.join();
    deqThread1.join();
    deqThread2.join();

    CHECK(enqSum == deqSum1);
    CHECK(enqSum == deqSum2);
}
}

int main() {
  doctest::Context context;
  context.run();
}

#pragma once

#include <atomic>
#include <concepts>
#include <span>
#include <type_traits>
#include <utility>


namespace SLQ_Auxil {
template <typename I>
  requires std::unsigned_integral<I>
constexpr I ceil_(I i1, I i2) {
  if (i1 % i2 == 0)
    return i1;
  else
    return ceil_(static_cast<I>(i1 + 1), i2);
};

template <typename I>
  requires std::unsigned_integral<I>
constexpr I divisible_(I i1, I i2) {
  if (i2 % i1 == 0)
    return i1;
  else
    return divisible_(static_cast<I>(i1 + 1), i2);
};

template <typename I>
  requires std::unsigned_integral<I>
constexpr I divisible_or_ceil(I i1, I i2) {
  if (i1 < i2)
    return divisible_(i1, i2);
  else
    return ceil_(i1, i2);
};


template<typename...>
struct atomic_arr_copy{
  static_assert(false);
};

template<typename T, size_t... indices>
requires std::is_trivially_copyable_v<T>
&& std::is_default_constructible_v<T>
&& std::atomic<char>::is_always_lock_free
struct atomic_arr_copy<T, std::integer_sequence<size_t, indices...>>{
private:
  static constexpr size_t size = sizeof(T);
  using SrcSpanType = const std::span<const std::atomic<char>, size>;
  using DestSpanType = std::span<std::atomic<char>, size>;
  T value;
public:
  using type = T;

  // allow nothrow conversion to T
  operator const T&() const noexcept{return value;};

  explicit atomic_arr_copy(){};

  atomic_arr_copy(const T arg): value{arg}{};

  atomic_arr_copy& operator=(const atomic_arr_copy& other){
    // make relevant sections of memory accessible via std::span
    const auto source = SrcSpanType{reinterpret_cast<const std::atomic<char>*>(&other.value), size};
    auto dest = DestSpanType{reinterpret_cast<std::atomic<char>*>(&this->value), size};
    // atomically copy data
    (dest[indices].store(source[indices], std::memory_order_relaxed),...);
    return *this;
  };

  atomic_arr_copy(const atomic_arr_copy& other){
    *this = other;
  };

  atomic_arr_copy& operator=(const atomic_arr_copy&& other){
    return operator=(other);
  };

  atomic_arr_copy(const atomic_arr_copy&& other){
    *this = other;
};
};

template <typename T>
using atomic_arr_copy_t = atomic_arr_copy<T, std::make_index_sequence<sizeof(T)>>;

template <typename T>
requires std::is_default_constructible_v<T>
&& std::is_copy_assignable_v<T>
struct atomic_arr_copy_standin{
private:
  T value;
public:
  using type = T;

  // allow nothrow conversion to T
  operator const T&() const noexcept{return value;};

  explicit atomic_arr_copy_standin(){};

  atomic_arr_copy_standin(const T& arg): value(arg){};

  atomic_arr_copy_standin& operator=(const atomic_arr_copy_standin& other){
    this->value = other.value;
    return *this;
  };

  atomic_arr_copy_standin(const atomic_arr_copy_standin& other){
    *this = other;
  };

  atomic_arr_copy_standin& operator=(const atomic_arr_copy_standin&& other){
    return operator=(other);
  };

  atomic_arr_copy_standin(const atomic_arr_copy_standin&& other){
    *this = other;
  };
};

template<typename, bool>
struct UB_or_not_UB_logic{
    static_assert(false);
};

template<typename T>
struct UB_or_not_UB_logic<T, true>{
  using type = atomic_arr_copy_standin<T>;
};

template<typename T>
struct UB_or_not_UB_logic<T, false>{
  using type = atomic_arr_copy_t<T>;
};

template<typename T, bool accept_UB>
using UB_or_not_UB = UB_or_not_UB_logic<T, accept_UB>::type;
}

#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <tuple>


#include "SLQ_Auxil.hpp"

namespace Element {
template <typename ContentType, std::uint32_t alignment>
struct alignas(alignment) SeqLockElement {
  using PayloadType = ContentType::type;
  ContentType content;
  std::atomic<std::int64_t> version = 0;
  void insert(const PayloadType&) noexcept;
  std::tuple<std::optional<PayloadType>, std::int64_t>
  read(const std::int64_t) const noexcept;
  explicit SeqLockElement() noexcept = default;
  ~SeqLockElement() = default;
  SeqLockElement(const SeqLockElement&) = delete;
  // copy assigment operator needs to be defined
  SeqLockElement& operator=(const SeqLockElement&) noexcept;
  SeqLockElement(SeqLockElement&&) = delete;
  SeqLockElement& operator=(SeqLockElement&&) = delete;
};
};

#define TEMPLATE_PARAMS                                                        \
  template <typename ContentType, std::uint32_t alignment>

#define SEQ_LOCK_ELEMENT Element::SeqLockElement<ContentType, alignment>

TEMPLATE_PARAMS
void SEQ_LOCK_ELEMENT::insert(const PayloadType& new_content) noexcept {
  this->version.fetch_add(1,std::memory_order_acquire);
  this->content = ContentType(new_content);
  this->version.fetch_add(1,std::memory_order_release);
};

TEMPLATE_PARAMS
std::tuple<std::optional<typename SEQ_LOCK_ELEMENT::PayloadType>, std::int64_t>
SEQ_LOCK_ELEMENT::read(const std::int64_t prev_version) const noexcept {
  std::tuple<std::optional<PayloadType>, std::int64_t> ret;
  std::optional<PayloadType>& ret_opt = std::get<0>(ret);
  std::int64_t& initial_version = std::get<1>(ret);
  std::atomic<std::int64_t> final_version;
  do {
    initial_version = this->version.load(std::memory_order_acquire);
    // atomic byte wise copying if accept_UB == false
    // make use of implicit conversion
    ret_opt = this->content;
    final_version.store(this->version,std::memory_order_release);
    // spin if write took place while reading
  } while (initial_version % 2 || (initial_version != final_version));
  ret_opt = initial_version >= prev_version ? ret_opt : std::nullopt;
  return ret;
};

TEMPLATE_PARAMS
SEQ_LOCK_ELEMENT& SEQ_LOCK_ELEMENT::operator=(const SEQ_LOCK_ELEMENT& other) noexcept{
  // read() provides functionality to safely retrieve content and version
  // read() always returns content if called with version argument 0
  const auto read_ret = other.read(0);
  this->content = ContentType(std::get<0>(read_ret).value());
  this->version.store(std::get<1>(read_ret), std::memory_order_relaxed);
  return *this;
};

#undef TEMPLATE_PARAMS
#undef SEQ_LOCK_ELEMENT

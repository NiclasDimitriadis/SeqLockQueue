## SeqLockQueue

Simple C++ implementation of a sequential-lock queue that heavily makes use of templates and leaves the user with the choice to avoid or embrace data races.

- simple, high performance implementation of a seq-lock queue
- implemented as single-producer multiple-consumer queue
- uses a ring buffer of power-of-two size
- allows for wait-free enqueueing
- designed to be cache friendly
- provides options to copy data atomically in chunks or accept undefinded behavior that comes with a data race

#### short explanation of a seq-lock queue
- queue that contains an atomic integral version counter for each of its elements
- there is no concept of a buffer overflow, enqueueing eventually just overwrites previous data
- when enqueueing, the counter is atomically incremented by one before the write occurs and again after
- dequeueing consists of (atomically) logging the version of the element read, speculatively reading the element and then logging the version again
- if the version number has changed in between the two logs, a write has been started (if final version number is odd) or completed (if final version number is even)
- in this the content that was previously read has to be discarded and the reading process is repeated until the initial and final version numbers are identical and even
- version increment thus effectively acts as a spin lock for dequeueing

#### `atomic_arr_copy`
`template<typename T, size_t... indices>`<br>
`requires std::is_trivially_copyable_v<T>`<br>
`&& std::is_default_constructible_v<T>`<br>
`&& std::atomic<char>::is_always_lock_free`<br>
`struct atomic_arr_copy<T, std::integer_sequence<size_t, indices>>`
- should be specialized via:<br>
`template <typename T>`<br>
`using atomic_arr_copy_t`
- class template to wrap an instance type `T` and provides data race free copy assigment and move assignment
- constraints:
-  `std::is_trivially_copyable_v<T>`: ensures that an instance of `T` can be copied by simply copying the memory it occupies
-  `std::is_default_constructible_v<T>`: ensures that `atomic_arr_copy` and therefore also `SeqLockElement` down the line, can be default constructed
-  `std::atomic<std::uint64_t>::is_always_lock_free`: ensures that 8-byte chunks can actually be atomically copied without imposing a software lock
- for the copy operations listed above, the payload is copied atomically copy byte-wise
- while this approach technically avoids a data race, it cannot guarantee the integerity and coherence of the data as a whole and a read of a partially overwritten entry still needs to be discarded
- this problem could only be addressed via a custom copy-assigment operator for `T`, which is impossible to enforce at copile time and on top of that would kill the trivially-copyable constraint
- an instance of `atomic_arr_copy_t<T>` can be default constructed or constructed from an instance of `T`
- `atomic_arr_copy_t<T>` is convertible to `T` via an operator to avoid unnecessary copies

#### `atomic_arr_copy_standin`
`template <typename T>`<br>
`requires std::is_default_constructible_v<T>`<br>
`&& std::is_copy_assignable_v<T>`<br>
`struct atomic_arr_copy_standin`
- specializes to a type that mimics `atomic_arr_copy_t<T>` in terms of interfaces but does not provide atomic copying
- constraints:
-  `std::is_default_constructible_v<T>`: ensures that `atomic_arr_copy_standin` and therefore also `SeqLockElement` down the line, can be default constructed
-  `std::is_copy_assignable_v<T>`: necessary for for copy assignment to work

#### `SeqLockQueue`
`template <typename ContentType_, std::uint32_t length_, bool share_cacheline, bool accept_UB>`<br>
  `requires(std::has_single_bit(length_)) &&`<br>
  `std::is_default_constructible_v<ContentType_> &&`<br>
  `std::is_trivially_copyable_v<ContentType_> &&`<br>
  `std::atomic<std::int64_t>::is_always_lock_free`<br>
`struct SeqLockQueue`
##### template parameters:
- `ContentType_`: type that the seq-lock queue will contain
- type requirements for `ContentType_`:
-  `is_default_constructible`: ensures that the queue can be filled with default constructed elements when its first constructed
-  `is_trivially_copyable`: ensures that a `ContentType_`-object can be copied just by copying its constituent bytes, hence making copying robust towards the race conditions arising when running a seq-lock.
-  `!std::is_const`: ensures new values can be assigned to `ContentType_`-variables when enqueueing new data
- `length`: number of elements that a queue can hold, restricted to powers of two to ensure fastest possible computation of memory location of an element in a ring buffer (via modulus)
- `share_cacheline`: `true` if multiple elements can be located on the same cacheline, `false` if each element is to be placed on a seperate cacheline
- `accept_UB`: if `false`, the queue-type's elements will contain `atomic_arr_copy_t<ContentType_>` which (technically) prevents data races, if `true` `atomic_arr_copy_standin<ContentType_>` will be used instead which embraces data races
##### outline:
At compile time, when the `SeqLockQueue` template is specialized, the desired alignment of the queue's elements is computed, based on the size of `ContentType_` and the value of `share_cacheline`. If `share_cacheline` is `true`, the queue elements' alignment will be the default alignment rounded up to multiples of 64 bytes. If `share_cacheline` is `false`, multiple elements can share a cache line as long as no element would have to span two cachelines. If the default alignment of the element type exceeds 32 bytes, alignment will still be rounded to its 64 byte ceiling. The `Element::SeqLockElement` class-template is then specialized using this cache-friendly alignment.
During construction, the aligned memory is heap allocated for the ring buffer. Subsequently, two `std::span` objects are constructed to access the buffer when enqueueing and dequeueing respectively. The buffer is then populated with default constructed `SeqLockElement` objects. Deallocation of buffer memory happens only during destruction, no further memory is allocated or deallocated during the queue object's lifetime.
Values of type `ContentType_` are enqueued via the `enqueue` method. As `SeqLockQueue` is a single producer queue, `enqueue` is not thread safe.
To read an enqueued value, the use of type member `QueueReader` is encouraged. A `QueueReader` object can be obtained by calling the `get_reader` member-function. An instance of `QueueReader` keeps track of the index of the next queue-entry to read. The version of the last successfully read entry is saved as well to avoid reading the same value twice. The `QueueReader::read_next_entry` method returns a `std::optional<ContentType_>` that contains a value if the queue contained a new value to read. If a write gets in the way of reading, `read_next_entry` (or rather the functionality of `SeqLockElement` that it calls) will busy-spin until the element is ready to be read.
#### `SeqLockElement`
`template <typename ContentType_, std::uint32_t alignment>`<br>
`struct alignas(alignment) SeqLockElement`
Class template that encapsulates the data and functionality of a single element in a seq-lock queue, containing an instance of `ContentType_`, a version counter and the logic to enqueue a new value and to read an enqueued value.
The template is specialized by the type of its content and the element's alignment as discussed above.
`ContentType_` is the appropriate specialization of either `atomic_arr_copy` or `atomic_arr_copy_standin` for the type of queue's content.
The element provides the `insert` and `read` methods used by `SeqLockQueue` und `QueueReader` when enqueueing or reading a value respectively. Only `insert` actually performs any writes to the queue-buffer's memory. `read` is read-only. This one-way flow of information should minimize cache coherence traffic.

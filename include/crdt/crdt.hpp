#ifndef CRDT_HPP
#define CRDT_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace crdt {

/// @brief A grow-only counter (gcounter) is a convergent replicated data type that can be used to implement a counter for a cluster of n nodes.
/// @tparam Nodes The number of nodes to account for
/// @tparam T The arithmetic unit used, defaults to uint32_t
template <size_t Nodes, typename T = uint32_t>
class gcounter {
public:
	static_assert(std::is_arithmetic_v<T>(), "T must be an arithmetic type.");
	using size_type = std::array<T, Nodes>::size_type;
	using value_type = T;
	static constexpr size_type size = Nodes;

	void increment(size_type index) noexcept {
		if (index < size) {
			_payload[index] += 1;
		}
	}

	value_type value() const { return std::accumulate(_payload.begin(), _payload.end(), value_type(0)); }

	void merge(const gcounter& other) {
		for (size_type i = 0; i < size; ++i) {
			_payload[i] = std::max(_payload[i], other._payload[i]);
		}
	}

	bool operator<=(const gcounter& other) const {
		bool is_less_than_or_equals = false;
		for (size_type i = 0; i < size; ++i) {
			is_less_than_or_equals &= (_payload[i] <= other._payload[i]);
		}
		return is_less_than_or_equals;
	}

protected:
	std::array<value_type, size> _payload;
};

/// @brief A positive-negative counter (pncounter) combines two gcounters to support increment and decrement operations.
/// @tparam Nodes The number of nodes to account for
/// @tparam T The arithmetic unit used, defaults to uint32_t
template <size_t Nodes, typename T = uint32_t>
class pncounter {
public:
	static_assert(std::is_arithmetic_v<T>(), "T must be an arithmetic type.");
	using size_type = std::array<T, Nodes>::size_type;
	using value_type = T;
	static constexpr size_type size = Nodes;

	void increment(size_type index) noexcept {
		if (index < size) {
			_increment_counter.increment(index);
		}
	}

	void decrement(size_type index) noexcept {
		if (index < size) {
			_decrement_counter.increment(index);
		}
	}

	value_type value() const {
		return std::accumulate(_increment_counter.begin(), _increment_counter.end(), value_type(0)) -
			   std::accumulate(_decrement_counter.begin(), _decrement_counter.end(), value_type(0));
	}

	void merge(const pncounter& other) {
		_increment_counter.merge(other._increment_counter);
		_decrement_counter.merge(other._decrement_counter);
	}

	bool operator<=(const gcounter& other) const {
		return (_increment_counter <= other._increment_counter) && (_decrement_counter <= other._decrement_counter);
	}

protected:
	gcounter<size, T> _increment_counter;
	gcounter<size, T> _decrement_counter;
};

/// @brief A grow-only set (gset) is a set that only allows adds. An element, once added, cannot be removed. The merger of two G-Sets is their union.
/// @tparam T The type of element
/// @tparam SetType The underlying set-like type to use, defaults to std::unordered_set
template <typename T, typename SetType = std::unordered_set<T>>
class grow_only_set {
public:
    using value_type = T;
	using set_type = SetType;
	using iterator = SetType::iterator;

	grow_only_set()
		: _data({}) {}
	grow_only_set(const grow_only_set& other)
		: _data(other._data) {}
	grow_only_set(grow_only_set&& other) noexcept
		: _data(std::move(other._data)) {}

	void add(T&& _elem) { _data.emplace(std::forward<T>(_elem)); }

	void merge(const grow_only_set& other) { _data.insert(other.begin(), other.end()); }

    // Returns true if the underlying set contains this element
    bool query(const T& elem) {
        return std::find(_data.begin(), _data.end(), elem) != _data.end();
    }

	// Returns true if the underlying set is a superset of otherset
	bool includes(const grow_only_set& otherset) const { std::includes(_data.cbegin(), _data.cend(), otherset._data.cbegin(), otherset._data.cend()); }

	bool operator<=(const grow_only_set& other) const { return includes(other); }

	const iterator& begin() const { return _data.cbegin(); }

	const iterator& end() const { return _data.cend(); }

protected:
	SetType _data;
};

/// @brief Two grow-only sets are combined to create a two-phase set to support remval of elements.
/// Once an element is removed, it cannot be re-added. Removals take precedence over additions.
/// @tparam T 
template <typename T, typename SetType = std::unordered_set<T>>
class two_phase_set {
public:

    void add(T&& elem) {
        _add_set.add(std::forward<T>(elem));
    }

    void remove(const T& elem) {
        if (_add_set.query(elem)) {
            _rem_set.add(elem);
        }
    }

    void merge(const two_phase_set& other) {
        _add_set.merge(other._add_set);
        _rem_set.merge(other._rem_set);
    }

    bool operator<=(const two_phase_set& other) {
        return (_add_set <= other._add_set) && (_rem_set <= other._rem_set);
    }

    // !! expensive operation !! Returns a collection of left-joined set between add and remove.
    SetType value() const {
        // TODO
    }

protected:
    grow_only_set<T, SetType> _add_set;
    grow_only_set<T, SetType> _rem_set;
};

template <typename T>
class observe_remove_set {
public:
protected:
};

} // namespace crdt

#endif // CRDT_HPP

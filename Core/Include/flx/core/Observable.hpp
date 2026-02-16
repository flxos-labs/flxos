#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace flx {

/**
 * @brief Thread-safe observable value with observer pattern.
 *
 * Inspired by LVGL's lv_subject_t but with zero LVGL dependency.
 * Observers are notified when the value changes.
 *
 * @tparam T The type of value to observe (int32_t, std::string, void*, etc.)
 */
template<typename T>
class Observable {
public:

	using Callback = std::function<void(const T& value)>;

	explicit Observable(T initial = T {})
		: m_value(initial), m_prev_value(initial) {}

	/**
	 * Set the value and notify all observers if it changed.
	 */
	void set(const T& value) {
		std::vector<Callback> observers_copy;
		T value_copy;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_value == value) {
				return;
			}
			m_prev_value = m_value;
			m_value = value;
			observers_copy = m_observers;
			value_copy = m_value;
		}
		for (auto& cb: observers_copy) {
			if (cb) {
				cb(value_copy);
			}
		}
	}

	/**
	 * Set the value and always notify observers (even if unchanged).
	 */
	void setAndNotify(const T& value) {
		std::vector<Callback> observers_copy;
		T value_copy;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_prev_value = m_value;
			m_value = value;
			observers_copy = m_observers;
			value_copy = m_value;
		}
		for (auto& cb: observers_copy) {
			if (cb) {
				cb(value_copy);
			}
		}
	}

	/**
	 * Get the current value.
	 */
	T get() const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_value;
	}

	/**
	 * Get the previous value (before last set).
	 */
	T getPrevious() const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_prev_value;
	}

	/**
	 * Subscribe to value changes.
	 * @param cb Callback invoked when value changes.
	 * @return Index of the observer (can be used for unsubscribe).
	 */
	size_t subscribe(Callback cb) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_observers.push_back(cb);
		return m_observers.size() - 1;
	}

	/**
	 * Unsubscribe an observer by index.
	 * @param index Index returned by subscribe().
	 */
	void unsubscribe(size_t index) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (index < m_observers.size()) {
			m_observers[index] = nullptr;
		}
	}

	/**
	 * Manually notify all observers with current value.
	 */
	void notify() {
		std::vector<Callback> observers_copy;
		T value_copy;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			observers_copy = m_observers;
			value_copy = m_value;
		}
		for (auto& cb: observers_copy) {
			if (cb) {
				cb(value_copy);
			}
		}
	}

private:

	T m_value;
	T m_prev_value;
	std::vector<Callback> m_observers {};
	mutable std::mutex m_mutex {};
};

/**
 * @brief Specialization helper for string observables with buffer.
 */
class StringObservable {
public:

	using Callback = std::function<void(const std::string& value)>;

	explicit StringObservable(const char* initial = "")
		: m_value(initial ? initial : ""), m_prev_value(m_value) {}

	void set(const char* value) {
		std::vector<Callback> observers_copy;
		std::string value_copy;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			std::string newVal = value ? value : "";
			if (m_value == newVal) {
				return;
			}
			m_prev_value = m_value;
			m_value = newVal;
			observers_copy = m_observers;
			value_copy = m_value;
		}
		for (auto& cb: observers_copy) {
			if (cb) {
				cb(value_copy);
			}
		}
	}

	void copy(const char* value) { set(value); }

	std::string get() const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_value;
	}

	std::string getPrevious() const {
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_prev_value;
	}

	size_t subscribe(Callback cb) {
		std::lock_guard<std::mutex> lock(m_mutex);
		m_observers.push_back(cb);
		return m_observers.size() - 1;
	}

	void unsubscribe(size_t index) {
		std::lock_guard<std::mutex> lock(m_mutex);
		if (index < m_observers.size()) {
			m_observers[index] = nullptr;
		}
	}

	void notify() {
		std::vector<Callback> observers_copy;
		std::string value_copy;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			observers_copy = m_observers;
			value_copy = m_value;
		}
		for (auto& cb: observers_copy) {
			if (cb) {
				cb(value_copy);
			}
		}
	}

private:

	std::string m_value {};
	std::string m_prev_value {};
	std::vector<Callback> m_observers {};
	mutable std::mutex m_mutex {};
};

} // namespace flx

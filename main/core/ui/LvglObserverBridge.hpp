#pragma once

#include "core/common/Observable.hpp"
#include "lvgl.h"

namespace System {

/**
 * @brief Bidirectional bridge between Observable<T> and LVGL observer pattern.
 *
 * Creates an lv_subject_t that mirrors an Observable<T>.
 * Changes in either direction are synchronized:
 * - Observable changes -> LVGL subject updated
 * - LVGL subject changes -> Observable updated
 */
template<typename T>
class LvglObserverBridge {
public:

	LvglObserverBridge(Observable<T>& observable) : m_observable(observable) {
		// Initialize LVGL subject with current observable value
		lv_subject_init_int(&m_subject, static_cast<int32_t>(observable.get()));

		// Subscribe to Observable changes and update LVGL subject
		observable.subscribe([this](const T& value) {
			if (!m_updating) {
				m_updating = true;
				lv_subject_set_int(&m_subject, static_cast<int32_t>(value));
				m_updating = false;
			}
		});

		// Subscribe to LVGL subject changes and update Observable
		lv_subject_add_observer(&m_subject, [](lv_observer_t* obs, lv_subject_t* s) {
			auto* self = static_cast<LvglObserverBridge*>(lv_observer_get_user_data(obs));
			if (!self->m_updating) {
				self->m_updating = true;
				self->m_observable.set(static_cast<T>(lv_subject_get_int(s)));
				self->m_updating = false;
			} }, this);
	}

	lv_subject_t* getSubject() { return &m_subject; }

private:

	Observable<T>& m_observable;
	lv_subject_t m_subject {};
	bool m_updating = false; // Prevent infinite update loops
};

/**
 * @brief Bidirectional bridge for StringObservable to lv_subject_t.
 */
class LvglStringObserverBridge {
public:

	LvglStringObserverBridge(StringObservable& observable) : m_observable(observable) {
		// Store the initial string
		m_buffer = observable.get();
		lv_subject_init_pointer(&m_subject, (void*)m_buffer.c_str());

		// Subscribe to Observable changes
		observable.subscribe([this](const char* value) {
			if (!m_updating) {
				m_updating = true;
				m_buffer = value ? value : "";
				lv_subject_set_pointer(&m_subject, (void*)m_buffer.c_str());
				m_updating = false;
			}
		});

		// Subscribe to LVGL subject changes and update Observable
		lv_subject_add_observer(&m_subject, [](lv_observer_t* obs, lv_subject_t* s) {
			auto* self = static_cast<LvglStringObserverBridge*>(lv_observer_get_user_data(obs));
			if (!self->m_updating) {
				self->m_updating = true;
				const char* val = static_cast<const char*>(lv_subject_get_pointer(s));
				self->m_observable.set(val ? val : "");
				self->m_updating = false;
			} }, this);
	}

	lv_subject_t* getSubject() { return &m_subject; }

private:

	StringObservable& m_observable;
	lv_subject_t m_subject {};
	std::string m_buffer {}; // Keep string alive
	bool m_updating = false; // Prevent infinite update loops
};

} // namespace System

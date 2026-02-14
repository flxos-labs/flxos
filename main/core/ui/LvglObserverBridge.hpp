#pragma once

#include "core/tasks/gui/GuiTask.hpp"
#include "lvgl.h"
#include "misc/lv_async.h"
#include <flx/core/Observable.hpp>

namespace System {

/**
 * @brief Bidirectional bridge between flx::Observable<T> and LVGL observer pattern.
 *
 * Creates an lv_subject_t that mirrors an flx::Observable<T>.
 * Changes in either direction are synchronized:
 * - flx::Observable changes -> LVGL subject updated
 * - LVGL subject changes -> flx::Observable updated
 */
template<typename T>
class LvglObserverBridge {
public:

	LvglObserverBridge(flx::Observable<T>& observable) : m_observable(observable) {
		// Initialize LVGL subject with current observable value
		m_pending_val = observable.get();
		lv_subject_init_int(&m_subject, static_cast<int32_t>(m_pending_val));

		// Subscribe to flx::Observable changes and update LVGL subject asynchronously
		observable.subscribe([this](const T& value) {
			if (!m_updating) {
				m_pending_val = value;
				lv_async_call(async_cb, this);
			}
		});

		// Subscribe to LVGL subject changes and update flx::Observable
		lv_subject_add_observer(&m_subject, [](lv_observer_t* obs, lv_subject_t* s) {
			auto* self = static_cast<LvglObserverBridge*>(lv_observer_get_user_data(obs));
			if (self) {
				// We are in GUI context (Lock held)
				self->m_updating = true;
				self->m_observable.set(static_cast<T>(lv_subject_get_int(s)));
				self->m_updating = false;
			} }, this);
	}

	lv_subject_t* getSubject() { return &m_subject; }

private:

	static void async_cb(void* user_data) {
		auto* self = static_cast<LvglObserverBridge*>(user_data);
		if (self) {
			// We are in GUI context (Lock held by run loop)
			self->m_updating = true;
			lv_subject_set_int(&self->m_subject, static_cast<int32_t>(self->m_pending_val));
			self->m_updating = false;
		}
	}

	flx::Observable<T>& m_observable;
	lv_subject_t m_subject {};
	bool m_updating = false;
	T m_pending_val {}; // Accessed by both threads, assumed atomic for word-sized types
};

/**
 * @brief Bidirectional bridge for flx::StringObservable to lv_subject_t.
 */
class LvglStringObserverBridge {
public:

	LvglStringObserverBridge(flx::StringObservable& observable) : m_observable(observable) {
		// Store the initial string
		m_buffer = observable.get();
		lv_subject_init_pointer(&m_subject, (void*)m_buffer.c_str());

		// Subscribe to flx::Observable changes
		observable.subscribe([this](const char* value) {
			if (!m_updating) {
				// Allocate data for async transfer
				auto* data = new AsyncUpdateData {this, value ? value : ""};
				lv_async_call(async_cb, data);
			}
		});

		// Subscribe to LVGL subject changes and update flx::Observable
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

	struct AsyncUpdateData {
		LvglStringObserverBridge* self;
		std::string value;
	};

	static void async_cb(void* user_data) {
		auto* data = static_cast<AsyncUpdateData*>(user_data);
		if (data && data->self) {
			auto* self = data->self;
			// GUI Lock is held
			self->m_updating = true;
			self->m_buffer = data->value; // Update local buffer
			lv_subject_set_pointer(&self->m_subject, (void*)self->m_buffer.c_str());
			self->m_updating = false;
		}
		delete data;
	}

	flx::StringObservable& m_observable;
	lv_subject_t m_subject {};
	std::string m_buffer {}; // Keep string alive for LVGL
	bool m_updating = false; // Prevent infinite update loops
};

} // namespace System

#pragma once

#include "LvglObserverBridge.hpp"
#include <type_traits>

namespace System {

/**
 * @brief Initialize an integer bridge and bind a member callback to subject changes.
 * Ensures the callback is invoked in the GUI context (Lock held).
 * 
 * @param bridge unique_ptr<LvglObserverBridge<int32_t>> member
 * @param observable Observable<int32_t> member
 * @param callback Member function taking int32_t
 */
#define INIT_INT_BRIDGE(bridge, observable, callback)                             \
	do {                                                                         \
		bridge = std::make_unique<::System::LvglObserverBridge<int32_t>>(observable); \
		lv_subject_add_observer(bridge->getSubject(), [](lv_observer_t* obs, lv_subject_t* s) { \
			auto* self = static_cast<std::remove_reference_t<decltype(*this)>*>(lv_observer_get_user_data(obs)); \
			self->callback(lv_subject_get_int(s)); }, this);                     \
	} while (0)

/**
 * @brief Initialize a string bridge.
 * 
 * @param bridge unique_ptr<LvglStringObserverBridge> member
 * @param observable StringObservable member
 */
#define INIT_STRING_BRIDGE(bridge, observable) \
	bridge = std::make_unique<::System::LvglStringObserverBridge>(observable)

/**
 * @brief Initialize a bridge without a callback.
 * 
 * @param bridge unique_ptr<LvglObserverBridge<int32_t>> member
 * @param observable Observable<int32_t> member
 */
#define INIT_BRIDGE(bridge, observable) \
	bridge = std::make_unique<::System::LvglObserverBridge<int32_t>>(observable)

} // namespace System

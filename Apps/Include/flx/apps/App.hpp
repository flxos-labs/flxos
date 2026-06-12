#pragma once

#include <flx/apps/AppContext.hpp>
#include <flx/core/Bundle.hpp>
#include <memory>
#include <string>

namespace flx::apps {

class App {
public:

	virtual ~App() = default;

	virtual bool onStart() { return true; }
	virtual bool onResume() { return true; }
	virtual void onPause() {}
	virtual void onStop() {}
	virtual void update() {}

	/**
	 * Called before the app is stopped — save UI/data state for later restoration.
	 * Return a Bundle of state data to persist (or empty to skip).
	 * Saved state is restored on the next launch via onRestoreState().
	 */
	virtual flx::core::Bundle onSaveState() { return {}; }

	/**
	 * Called during onStart() if a previously saved state exists.
	 * Restore your UI and data from this bundle.
	 */
	virtual void onRestoreState(const flx::core::Bundle& savedState) { (void)savedState; }

	/**
	 * Called when the AppManager receives a new Intent for this app while it
	 * is already running in the stack.
	 */
	virtual void onNewIntent(const Intent& intent) {}

	/**
	 * Called when a child app finishes and delivers a result.
	 * Override this to receive results from apps you launched via startAppForResult().
	 */
	virtual void onResult(ResultCode resultCode, const flx::core::Bundle& data) {
		(void)resultCode;
		(void)data;
	}

	virtual std::string getPackageName() const = 0;
	virtual std::string getAppName() const = 0;
	virtual const void* getIcon() const { return nullptr; }
	virtual void createUI(void* parent) {}
	virtual std::string getVersion() const { return "0.1.0"; }
	virtual bool isFlxApp() const { return false; }

	bool isActive() const { return m_isActive; }
	void setActive(bool active) { m_isActive = active; }

	// === Context access ===

	void setContext(AppContext* ctx) { m_context = ctx; }
	AppContext* getContext() const { return m_context; }

protected:

	bool m_isActive = false;
	AppContext* m_context = nullptr;
};

} // namespace flx::apps

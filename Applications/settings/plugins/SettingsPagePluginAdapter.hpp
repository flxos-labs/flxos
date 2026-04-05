#pragma once

#include "settings/plugins/ISettingsPlugin.hpp"
#include <memory>
#include <utility>

namespace System::Apps::Settings {

template <typename TPage>
class SettingsPagePluginAdapter : public ISettingsPlugin {
public:

	void onAttach(lv_obj_t* parent, std::function<void()> onBack) override {
		onDetach();
		if (parent == nullptr) {
			return;
		}

		m_page = std::make_unique<TPage>(parent, std::move(onBack));
	}

	void onDetach() override {
		if (m_page) {
			m_page->destroy();
			m_page.reset();
		}
	}

	void onShow() override {
		if (m_page) {
			m_page->show();
		}
	}

	void onHide() override {
		if (m_page) {
			m_page->hide();
		}
	}

	void onSave() override {}

private:

	std::unique_ptr<TPage> m_page;
};

} // namespace System::Apps::Settings

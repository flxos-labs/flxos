#pragma once

#include "../TaskManager.hpp"
#include <atomic>

namespace System {
class ResourceMonitorTask : public Task {
public:

	static ResourceMonitorTask& getInstance();

	struct Stats {
		size_t freeHeap;
		size_t minFreeHeap;
		size_t freePsram;
		uint32_t uptimeSeconds;
	};

	Stats getLatestStats() const;

protected:

	void run(void* data) override;

private:

	ResourceMonitorTask();
	virtual ~ResourceMonitorTask() = default;
	ResourceMonitorTask(const ResourceMonitorTask&) = delete;
	ResourceMonitorTask& operator=(const ResourceMonitorTask&) = delete;

	std::atomic<size_t> m_freeHeap {0};
	std::atomic<size_t> m_minFreeHeap {0};
	std::atomic<size_t> m_freePsram {0};
	std::atomic<uint32_t> m_uptimeSeconds {0};
};

} // namespace System

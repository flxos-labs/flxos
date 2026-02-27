#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/spi/ISpiBus.hpp>
#include <string_view>

namespace flx::hal::spi {

class EspSpiBus : public DeviceBase<ISpiBus> {
public:
	EspSpiBus(int hostId, int miso, int mosi, int sclk, int maxTransferSz = 4096);
	~EspSpiBus() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override;
	std::string_view getDescription() const override;
	bool start() override;
	bool stop() override;
	Type getType() const override { return ISpiBus::getType(); }

	// ── ISpiBus ───────────────────────────────────────────────────────────
	int getHostId() const override { return m_hostId; }
	bool isShared() const override { return true; }
	bool acquire(uint32_t timeoutMs = 1000) override;
	void release() override;
	uint32_t getTransactionCount() const override { return m_transactionCount; }
	uint32_t getContentionCount() const override { return m_contentionCount; }

private:
	int m_hostId;
	int m_miso;
	int m_mosi;
	int m_sclk;
	int m_maxTransferSz;

	uint32_t m_transactionCount = 0;
	uint32_t m_contentionCount = 0;
	bool m_initialized = false;
};

} // namespace flx::hal::spi

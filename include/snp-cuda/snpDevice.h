#ifndef __SNP_DEVICE_H__
#define __SNP_DEVICE_H__

#include <snp-cuda\snpMacros.h>
#include <snp-cuda\snpErrorCode.h>
#include <snp-cuda\snpOperation.h>

NS_SNP_BEGIN

class snpDeviceImpl;

template<uint16 bitwidth>
class snpDevice
{
private:
	static const uint16 s_cellSize = static_cast<uint16>(static_cast<float>(bitwidth) / (sizeof(uint32) * 8) + 0.5f);

public:
	struct snpBitfield
	{
		uint32 raw[s_cellSize];
	};

	typedef union
	{
		struct
		{
			uint32 raw[s_cellSize * 4];
		};

		struct
		{
			snpBitfield		addressMask;
			snpBitfield		addressData;
			snpBitfield		dataMask;
			snpBitfield		dataData;
		
		} field;
	} snpInstruction;

public:
	snpDevice();
	~snpDevice();

	snpErrorCode configure(uint32 cellsPerPU, uint32 numberOfPU);
	snpErrorCode end();

	inline uint16 getCellSize() const
	{
		return s_cellSize;
	}

	inline uint32 getCellsPerPU() const
	{
		return (m_device != nullptr) ? m_device->getCellsPerPU() : 0;
	}

	inline uint32 getNumberOfPU() const
	{
		return (m_device != nullptr) ? m_device->getNumberOfPU() : 0;
	}

	// Execute isntruction in device, returns True if at least one cell activated
	bool exec(bool singleCell, snpOperation operation, const snpInstruction &instruction, snpErrorCode *error = nullptr);

	// Read data from the 1st cell which activated while the last instruction
	// Returns False if no one cell is selected
	bool read(snpBitfield &bitfield, snpErrorCode *error = nullptr);

private:
	snpDeviceImpl	*m_device;
};

class snpDeviceImpl
{
private:
	static snpDeviceImpl * create(uint16 cellSize, uint32 cellsPerPU, uint32 numberOfPU);
	snpDeviceImpl();
	~snpDeviceImpl();

	bool init(uint16 cellSize, uint32 cellsPerPU, uint32 numberOfPU);

	bool exec(bool singleCell, snpOperation operation, const uint32 * const instruction);
	bool read(uint32 *bitfield);

	inline uint32 getCellsPerPU() const
	{
		return m_cellsPerPU;
	}

	inline uint32 getNumberOfPU() const
	{
		return m_numberOfPU;
	}

	uint32	m_cellSize;
	uint32	m_cellsPerPU;
	uint32	m_numberOfPU;

	uint32	*d_memory;
	int32	*d_output;

	int32	*h_output;
	uint32	*h_cell;
	int32	m_cellIndex;

	template<uint16 bitwidth>
	friend class snpDevice;

	static bool m_exists;
};

template<uint16 bitwidth>
snpDevice<bitwidth>::snpDevice()
	: m_device(nullptr)
{
}

template<uint16 bitwidth>
snpDevice<bitwidth>::~snpDevice()
{
	if (m_device != nullptr)
	{
		delete m_device;
	}
}

template<uint16 bitwidth>
snpErrorCode snpDevice<bitwidth>::configure(uint32 cellsPerPU, uint32 numberOfPU)
{
	if (m_device != nullptr)
	{
		return snpErrorCode::DEVICE_ALREADY_CONFIGURED;
	}

	m_device = snpDeviceImpl::create(s_cellSize, cellsPerPU, numberOfPU);
	return (m_device != nullptr) ? snpErrorCode::SUCCEEDED : snpErrorCode::GPU_INIT_ERROR;
}

template<uint16 bitwidth>
snpErrorCode snpDevice<bitwidth>::end()
{
	if (m_device == nullptr)
	{
		return snpErrorCode::DEVICE_NOT_CONFIGURED;
	}

	delete m_device;
	m_device = nullptr;

	return snpErrorCode::SUCCEEDED;
}

template<uint16 bitwidth>
bool snpDevice<bitwidth>::exec(bool singleCell, snpOperation operation, const snpInstruction &instruction, snpErrorCode *error/* = nullptr*/)
{
	if (m_device != nullptr)
	{
		if (error != nullptr)
			(*error) = snpErrorCode::SUCCEEDED;

		return m_device->exec(singleCell, operation, instruction.raw);
	}
	
	if (error != nullptr)
		(*error) = snpErrorCode::DEVICE_NOT_CONFIGURED;

	return false;
}

template<uint16 bitwidth>
bool snpDevice<bitwidth>::read(snpBitfield &bitfield, snpErrorCode *error/* = nullptr*/)
{
	if (m_device != nullptr)
	{
		if (error != nullptr)
			(*error) = snpErrorCode::SUCCEEDED;

		return m_device->read(bitfield.raw);
	}
	
	if (error != nullptr)
		(*error) = snpErrorCode::DEVICE_NOT_CONFIGURED;

	return false;
}

NS_SNP_END

#endif //__SNP_DEVICE_H__

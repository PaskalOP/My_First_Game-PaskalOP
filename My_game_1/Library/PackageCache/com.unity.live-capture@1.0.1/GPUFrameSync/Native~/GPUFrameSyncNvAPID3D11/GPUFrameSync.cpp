#include <string>
#include <sstream>
#include <fstream>

#include "d3d11.h"
#include "dxgi.h"

#include "GPUFrameSync.h"

namespace GPUFrameSync
{
#ifdef DEBUG_LOG
	void GPUFrameSync::WriteFileDebug(const char* const message, const bool append)
	{
		std::ofstream myfile;

		if (append)
		{
			myfile.open("C:/NVIDIA_GPUFrameSync_DebugFile.txt", std::ios_base::app | std::ios_base::out);
		}
		else
		{
			myfile.open("C:/NVIDIA_GPUFrameSync_DebugFile.txt");
		}

		myfile << message;
		myfile.close();
	}
#endif

    GPUFrameSync::GPUFrameSync()
		: m_GroupId(1)
		, m_BarrierId(1)
		, m_FrameCount(0)
		, m_GSyncSwapGroups(0)
		, m_GSyncBarriers(0)
		, m_GSyncMaster(true)
		, m_GSyncCounter(false)
		, m_IsActive(false)
	{
#ifdef DEBUG_LOG
		WriteFileDebug("---- Initialize File ----\n", false);
#endif
		Prepare();
	}

    GPUFrameSync::~GPUFrameSync()
	{
#ifdef DEBUG_LOG
		WriteFileDebug("---- Destroy File ----\n");
		CloseHandle(m_fileDebug);
#endif
	}

	void GPUFrameSync::Prepare()
	{
		// Prepare NVAPI for use in this application
		NvAPI_Status status = NvAPI_Initialize();

#ifdef DEBUG_LOG
		if (status != NVAPI_OK)
		{
			NvAPI_ShortString errorMessage;
			NvAPI_GetErrorMessage(status, errorMessage);
			WriteFileDebug("* Failed: NvAPI_Initialize, ");
			WriteFileDebug(errorMessage + '\n');
		}
		else
			WriteFileDebug("* Success: NvAPI_Initialize\n");
#endif
	}

	void GPUFrameSync::SetupWorkStation()
	{
		// Register our request to use workstation SwapGroup resources in the driver
		NvU32 gpuCount;
		NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS];
		NvAPI_Status status = NvAPI_EnumPhysicalGPUs(nvGPUHandle, &gpuCount);
		if (NVAPI_OK == status)
		{
			for (unsigned int iGpu = 0; iGpu < gpuCount; iGpu++)
			{
				// send request to enable NVAPI_GPU_WORKSTATION_FEATURE_MASK_SWAPGROUP
				status = NvAPI_GPU_WorkstationFeatureSetup(nvGPUHandle[iGpu], NVAPI_GPU_WORKSTATION_FEATURE_MASK_SWAPGROUP, 0);

#ifdef DEBUG_LOG
				if (status == NvAPI_Status::NVAPI_OK)
					WriteFileDebug("* Success: NvAPI_GPU_WorkstationFeatureSetup\n");
				else
					WriteFileDebug("* Failed: NvAPI_GPU_WorkstationFeatureSetup\n");
#endif
			}
		}
	}

	void GPUFrameSync::DisposeWorkStation()
	{
		// Unregister our request to use workstation SwapGroup resources in the driver
		NvAPI_Status status;
		NvU32 gpuCount;
		NvPhysicalGpuHandle nvGPUHandle[NVAPI_MAX_PHYSICAL_GPUS];
		status = NvAPI_EnumPhysicalGPUs(nvGPUHandle, &gpuCount);
		if (NVAPI_OK == status)
		{
			for (unsigned int iGpu = 0; iGpu < gpuCount; iGpu++)
			{
				// send request to disable NVAPI_GPU_WORKSTATION_FEATURE_MASK_SWAPGROUP
				status = NvAPI_GPU_WorkstationFeatureSetup(nvGPUHandle[iGpu], 0, NVAPI_GPU_WORKSTATION_FEATURE_MASK_SWAPGROUP);

#ifdef DEBUG_LOG
				if (status == NvAPI_Status::NVAPI_OK)
					WriteFileDebug("* Success: NvAPI_GPU_WorkstationFeatureSetup\n");
				else
					WriteFileDebug("* Failed: NvAPI_GPU_WorkstationFeatureSetup\n");
#endif
			}
		}
	}

	bool GPUFrameSync::Initialize(ID3D11Device* const pDevice, IDXGISwapChain* const pSwapChain)
	{
		auto status = NVAPI_OK;

		status = NvAPI_D3D1x_QueryMaxSwapGroup(pDevice, &m_GSyncSwapGroups, &m_GSyncBarriers);

#ifdef DEBUG_LOG
		if (status == NvAPI_Status::NVAPI_OK)
			WriteFileDebug("* Success: NvAPI_D3D1x_QueryMaxSwapGroup\n");
		else
			WriteFileDebug("* Failed: NvAPI_D3D1x_QueryMaxSwapGroup\n");
#endif

		if (m_GSyncSwapGroups > 0)
		{
			if ((m_GroupId >= 0) && (m_GroupId <= m_GSyncSwapGroups))
			{
				status = NvAPI_D3D1x_JoinSwapGroup(pDevice, pSwapChain, m_GroupId, m_GroupId > 0 ? true : false);
#ifdef DEBUG_LOG
				if (status == NvAPI_Status::NVAPI_OK)
				{
					WriteFileDebug("* Success: NvAPI_D3D1x_JoinSwapGroup, NVAPI_OK\n");
				}
				else if (status == NVAPI_ERROR)
				{
					WriteFileDebug("* Failed: NvAPI_D3D1x_JoinSwapGroup, NVAPI_ERROR\n");
				}
				else if (NVAPI_INVALID_ARGUMENT)
				{
					WriteFileDebug("* Failed: NvAPI_D3D1x_JoinSwapGroup, NVAPI_INVALID_ARGUMENT\n");
				}
				else if (NVAPI_API_NOT_INITIALIZED)
				{
					WriteFileDebug("* Failed: NvAPI_D3D1x_JoinSwapGroup, NVAPI_API_NOT_INITIALIZED\n");
				}

				std::ostringstream oss;
				oss << "SwapGroup (" << m_GroupId << ") / (" << m_GSyncSwapGroups << ")\n";
				std::string var = oss.str();
				WriteFileDebug(var.c_str());
#endif

				if (status != NVAPI_OK)
				{
					return false;
				}
			}

			if (m_GSyncBarriers > 0)
			{
				NvU32 frameCount;

				//! heavy
				status = NvAPI_D3D1x_QueryFrameCount(pDevice, &frameCount);

				m_GSyncCounter = (status == NVAPI_OK);

				//! sync node
				if (m_GSyncMaster && m_GSyncCounter)
				{
					status = NvAPI_D3D1x_ResetFrameCount(pDevice);
				}

				if ((m_BarrierId >= 0) && (m_BarrierId <= m_GSyncBarriers) &&
					(m_GroupId >= 0) && (m_GroupId <= m_GSyncSwapGroups))
				{
					status = NvAPI_D3D1x_BindSwapBarrier(pDevice, m_GroupId, m_BarrierId);

#ifdef DEBUG_LOG
					if (status == NvAPI_Status::NVAPI_OK)
					{
						WriteFileDebug("* Success: NvAPI_D3D1x_BindSwapBarrier\n");
					}
					else if (status == NVAPI_ERROR)
					{
						WriteFileDebug("* Failed: NvAPI_D3D1x_BindSwapBarrier, NVAPI_ERROR\n");
					}
					else if (NVAPI_INVALID_ARGUMENT)
					{
						WriteFileDebug("* Failed: NvAPI_D3D1x_BindSwapBarrier, NVAPI_INVALID_ARGUMENT\n");
					}
					else if (NVAPI_API_NOT_INITIALIZED)
					{
						WriteFileDebug("* Failed: NvAPI_D3D1x_BindSwapBarrier, NVAPI_API_NOT_INITIALIZED\n");
					}
#endif
					if (status != NVAPI_OK)
					{
						return false;
					}
				}
			}
			else if (m_BarrierId > 0)
			{
#ifdef DEBUG_LOG
				WriteFileDebug("* Failed: NvAPI_D3D1x_QueryMaxSwapGroup returned 0 barriers\n");
#endif
				m_BarrierId = 0;
				return false;
			}

#ifdef DEBUG_LOG
			std::ostringstream oss;
			oss << "BindSwapBarrier (" << m_BarrierId << ") / (" << m_GSyncBarriers << ")\n";
			std::string var = oss.str();
			WriteFileDebug(var.c_str());
#endif

			status = NvAPI_D3D1x_QuerySwapGroup(pDevice, pSwapChain, &m_GroupId, &m_BarrierId);

#ifdef DEBUG_LOG
			if (status == NvAPI_Status::NVAPI_OK)
				WriteFileDebug("* Success: NvAPI_D3D1x_QuerySwapGroup\n");
			else
				WriteFileDebug("* Failed: NvAPI_D3D1x_QuerySwapGroup\n");
#endif
		}
		else if (m_GroupId > 0)
		{
#ifdef DEBUG_LOG
			WriteFileDebug("* Failed: NvAPI_D3D1x_QueryMaxSwapGroup returned 0\n");
#endif
			m_GroupId = 0;
			return false;
		}
		else
		{
#ifdef DEBUG_LOG
			WriteFileDebug("* Failed: NvAPI_D3D1x_QueryMaxSwapGroup returned 0\n");
#endif
		}

		return (status == NVAPI_OK);
	}

	void GPUFrameSync::Dispose(ID3D11Device* const pDevice, IDXGISwapChain* const pSwapChain)
	{
		NvAPI_Status status;
		if (m_GroupId > 0)
		{
			if (m_BarrierId > 0)
			{
				if (NVAPI_OK == (status = NvAPI_D3D1x_BindSwapBarrier(pDevice, m_GroupId, 0)))
				{
					m_BarrierId = 0;
				}
			}

			if (NVAPI_OK == (status = NvAPI_D3D1x_JoinSwapGroup(pDevice, pSwapChain, 0, 0)))
			{
				m_GroupId = 0;
			}
		}
	}

	NvU32 GPUFrameSync::QueryFrameCount(ID3D11Device* const pDevice)
	{
		NvU32 count = 0;

		if (m_GSyncCounter)
		{
			NvAPI_Status status;
			if (NVAPI_OK == (status = NvAPI_D3D1x_QueryFrameCount(pDevice, &count)))
			{
				m_FrameCount = count;
			}
		}
		else
		{
			++m_FrameCount;
		}

		return m_FrameCount;
	}

	void GPUFrameSync::ResetFrameCount(ID3D11Device* const pDevice)
	{
		if (m_GSyncMaster)
		{
			auto status = NVAPI_OK;
			status = NvAPI_D3D1x_ResetFrameCount(pDevice);
		}
		else
		{
			m_FrameCount = 0;
		}
	}

	bool GPUFrameSync::Render(ID3D11Device* const pDevice,
		                      IDXGISwapChain* const pSwapChain,
		                      const int pVsync,
		                      const int pFlags)
	{
		const auto result = NvAPI_D3D1x_Present(pDevice, pSwapChain, pVsync, pFlags);

		if (result == NVAPI_OK)
		{
			return true;
		}
#ifdef DEBUG_LOG
		WriteFileDebug("* Failed: NvAPI_D3D1x_Present\n");
#endif
		return false;
	}

	void GPUFrameSync::EnableSystem(ID3D11Device* const pDevice,
		IDXGISwapChain* const pSwapChain,
		const bool value)
	{
		m_IsActive = value;
		EnableSwapGroup(pDevice, pSwapChain, value);
		EnableSwapBarrier(pDevice, value);
	}

	void GPUFrameSync::EnableSwapGroup(ID3D11Device* const pDevice,
		                               IDXGISwapChain* const pSwapChain,
		                               const bool value)
	{
#ifdef DEBUG_LOG
		if (value)
			WriteFileDebug("EnableSwapGroup: (true), newSwapGroup ID is 1\n");
		else
			WriteFileDebug("EnableSwapGroup: (false), newSwapGroup ID is 0\n");
#endif

		const NvU32 newSwapGroup = (value) ? 1 : 0;
		if ((newSwapGroup != m_GroupId) && (newSwapGroup <= m_GSyncSwapGroups))
		{
			const auto status = NvAPI_D3D1x_JoinSwapGroup(pDevice, pSwapChain, newSwapGroup, (newSwapGroup > 0));

#ifdef DEBUG_LOG
			if (status == NvAPI_Status::NVAPI_OK)
			{
				WriteFileDebug("* Success: NvAPI_D3D1x_JoinSwapGroup, NVAPI_OK\n");
			}
			else if (status == NVAPI_ERROR)
			{
				WriteFileDebug("* Failed: NvAPI_D3D1x_JoinSwapGroup, NVAPI_ERROR\n");
				WriteFileDebug("Values before Query:\n");

				std::ostringstream ossBefore;
				ossBefore << "m_GroupeId(" << m_GroupId << "), m_BarrierId (" << m_BarrierId << ")\n";
				std::string var = ossBefore.str();
				WriteFileDebug(var.c_str());

				NvAPI_D3D1x_QuerySwapGroup(pDevice, pSwapChain, &m_GroupId, &m_BarrierId);
				WriteFileDebug("Values after Query:\n");

				std::ostringstream oss;
				oss << "m_GroupeId(" << m_GroupId << "), m_BarrierId (" << m_BarrierId << ")\n";
				var = oss.str();
				WriteFileDebug(var.c_str());
			}
			else if (NVAPI_INVALID_ARGUMENT)
			{
				WriteFileDebug("* Failed: NvAPI_D3D1x_JoinSwapGroup, NVAPI_INVALID_ARGUMENT\n");
			}
			else if (NVAPI_API_NOT_INITIALIZED)
			{
				WriteFileDebug("* Failed: NvAPI_D3D1x_JoinSwapGroup, NVAPI_API_NOT_INITIALIZED\n");
			}
#endif

			if (status == NVAPI_OK)
			{
				m_GroupId = newSwapGroup;
			}
		}
	}

	void GPUFrameSync::EnableSwapBarrier(ID3D11Device* const pDevice, const bool value)
	{
		if (m_GroupId == 1)
		{
#ifdef DEBUG_LOG
			if (value)
				WriteFileDebug("EnableSwapBarrier: (true), newSwapBarrier ID is 1\n");
			else
				WriteFileDebug("EnableSwapBarrier: (false), newSwapBarrier ID is 0\n");
#endif

			const NvU32 newSwapBarrier = (value) ? 1 : 0;
			if ((newSwapBarrier != m_BarrierId) && (newSwapBarrier <= m_GSyncBarriers))
			{
				const auto status = NvAPI_D3D1x_BindSwapBarrier(pDevice, m_GroupId, newSwapBarrier);

#ifdef DEBUG_LOG
				if (status == NvAPI_Status::NVAPI_OK)
				{
					WriteFileDebug("* Success: NvAPI_D3D1x_BindSwapBarrier, NVAPI_OK\n");
				}
				else if (status == NVAPI_ERROR)
				{
					WriteFileDebug("* Failed: NvAPI_D3D1x_BindSwapBarrier, NVAPI_ERROR\n");
				}
				else if (NVAPI_INVALID_ARGUMENT)
				{
					WriteFileDebug("* Failed: NvAPI_D3D1x_BindSwapBarrier, NVAPI_INVALID_ARGUMENT\n");
				}
				else if (NVAPI_API_NOT_INITIALIZED)
				{
					WriteFileDebug("* Failed: NvAPI_D3D1x_BindSwapBarrier, NVAPI_API_NOT_INITIALIZED\n");
				}
#endif

				if (status == NVAPI_OK)
				{
					m_BarrierId = newSwapBarrier;
				}
			}
#ifdef DEBUG_LOG
			WriteFileDebug("EnableSwapBarrier: already set, nothing has been called\n");
#endif
		}
#ifdef DEBUG_LOG
		else
		{
			WriteFileDebug("EnableSwapBarrier: (NULL), m_GroupId is diffent than 1\n");
		}
#endif
	}

	void GPUFrameSync::EnableSyncCounter(const bool value)
	{
		m_GSyncCounter = value;
	}
}

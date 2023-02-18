#pragma once 

#pragma warning(disable : 4619 4061 4265 4355 4365 4571 4623 4625 4626 4628 4668 4710 4711 4746 4774 4820 4987 5026 5027 5031 5032 5039 5045)

#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

#define _CRT_RAND_S

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <d3d12.h>
#include <d3d11_1.h>
#include <d3d10.h>

#if defined(NTDDI_WIN10_RS2)
#include <dxgi1_6.h>
#else
#include <dxgi1_5.h>
#endif

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wincodec.h>

#include "d3dx12.h"

#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <vector>
#ifdef __cpp_lib_span 
#include <span>
#endif
#include <map>
#include <unordered_map>
#include <set>
#include <array>
// #ifdef _DEBUG
// #include <dxgidebug.h>
// #endif

#include <stdio.h>
#include <stdlib.h>
#include <numeric>
// #include <pix3.h>
#include <variant>
#include <optional>

//#define DML_TARGET_VERSION_USE_LATEST
#define DML_TARGET_VERSION 0x5100
#define DMLX_USE_ABSEIL 0
#define DMLX_USE_GSL 0
//#define __cpp_lib_span 0
#define DMLX_USE_WIL 0
#include "DirectML.h"
#include "DirectMLX.h"


//#include "DescriptorHeap.h"

#include "../Common/Common.h"
#include "../Common/OnnxParser.h"
#include "Common/Float16Compressor.h"

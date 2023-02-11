#include "ODID3D12RHI.h"

#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"
#include "D3D12State.h"
#include "D3D12Resources.h"
#include "D3D12Viewport.h"
#include "D3D12ConstantBuffer.h"

#include "RHIValidationCommon.h"
#include "GenericPlatform/GenericPlatformFile.h"



DEFINE_LOG_CATEGORY_STATIC(LogODID3D12RHI, Log, All);

#define LOCTEXT_NAMESPACE "FODID3D12RHIModule"


class FODID3D12RHI final : public ODIRHI
{

public:
	FODID3D12RHI(const FNGXRHICreateArguments& Arguments);
	virtual void ExecuteInfer(FRHICommandList& CmdList, const FRHIDLSSArguments& InArguments, FDLSSStateRef InDLSSState) final;
	virtual ~FODID3D12RHI();
private:
	NVSDK_NGX_Result InitializeNewModel(const FNGXRHICreateArguments& InArguments, const wchar_t* InApplicationDataPath, ID3D12Device* InHandle, const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo);

	FD3D12DynamicRHI* D3D12RHI = nullptr;
};

NVSDK_NGX_Result FNGXD3D12RHI::Init_NGX_D3D12(const FNGXRHICreateArguments& InArguments, const wchar_t* InApplicationDataPath, ID3D12Device* InHandle, const NVSDK_NGX_FeatureCommonInfo* InFeatureInfo)
{
	NVSDK_NGX_Result Result = NVSDK_NGX_Result_Fail;
	int32 APIVersion = NVSDK_NGX_VERSION_API_MACRO;
	do 
	{
		if (InArguments.InitializeNGXWithNGXApplicationID())
		{
			Result = NVSDK_NGX_D3D12_Init(InArguments.NGXAppId, InApplicationDataPath, InHandle, InFeatureInfo, static_cast<NVSDK_NGX_Version>(APIVersion));
			UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("NVSDK_NGX_D3D12_Init(AppID= %u, APIVersion = 0x%x, Device=%p) -> (%u %s)"), InArguments.NGXAppId, APIVersion, InHandle, Result, GetNGXResultAsString(Result));
		}
		else
		{
			Result = NVSDK_NGX_D3D12_Init_with_ProjectID(TCHAR_TO_UTF8(*InArguments.UnrealProjectID), NVSDK_NGX_ENGINE_TYPE_UNREAL, TCHAR_TO_UTF8(*InArguments.UnrealEngineVersion), InApplicationDataPath, InHandle, InFeatureInfo, static_cast<NVSDK_NGX_Version>(APIVersion));
			UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("NVSDK_NGX_D3D12_Init_with_ProjectID(ProjectID = %s, EngineVersion=%s, APIVersion = 0x%x, Device=%p) -> (%u %s)"), *InArguments.UnrealProjectID, *InArguments.UnrealEngineVersion, APIVersion, InHandle,  Result, GetNGXResultAsString(Result));
		}

		if (NVSDK_NGX_FAILED(Result))
		{
			NVSDK_NGX_D3D12_Shutdown1(InHandle);
		}
		
		--APIVersion;
	} while (NVSDK_NGX_FAILED(Result) && APIVersion >= NVSDK_NGX_VERSION_API_MACRO_BASE_LINE);

	if (NVSDK_NGX_SUCCEED(Result) && (APIVersion + 1 < NVSDK_NGX_VERSION_API_MACRO_WITH_LOGGING))
	{
		UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("Warning: NVSDK_NGX_D3D12_Init succeeded, but the driver installed on this system is too old the support the NGX logging API. The console variables r.NGX.LogLevel and r.NGX.EnableOtherLoggingSinks will have no effect and NGX logs will only show up in their own log files, and not in UE's log files."));
	}

	return Result;
}



FODID3D12RHI::FODID3D12RHI(const FNGXRHICreateArguments& Arguments)
	: NGXRHI(Arguments)
	, D3D12RHI(static_cast<FD3D12DynamicRHI*>(Arguments.DynamicRHI))

{
	ID3D12Device* Direct3DDevice = D3D12RHI->GetAdapter().GetD3DDevice();

	ensure(D3D12RHI);
	ensure(Direct3DDevice);
	bIsIncompatibleAPICaptureToolActive = IsIncompatibleAPICaptureToolActive(Direct3DDevice);

	const FString NGXLogDir = GetNGXLogDirectory();
	IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*NGXLogDir);

	NVSDK_NGX_Result ResultInit = Init_NGX_D3D12(Arguments, *NGXLogDir, Direct3DDevice, CommonFeatureInfo());
	UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("NVSDK_NGX_D3D12_Init (Log %s) -> (%u %s)"), *NGXLogDir, ResultInit, GetNGXResultAsString(ResultInit));
	
	// store for the higher level code interpret
	DLSSQueryFeature.DLSSInitResult = ResultInit;

	if (NVSDK_NGX_Result_FAIL_OutOfDate == ResultInit)
	{
		DLSSQueryFeature.DriverRequirements.DriverUpdateRequired = true;
	}
	else if (NVSDK_NGX_SUCCEED(ResultInit))
	{
		bNGXInitialized = true;

		NVSDK_NGX_Result ResultGetParameters = NVSDK_NGX_D3D12_GetCapabilityParameters(&DLSSQueryFeature.CapabilityParameters);

		UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("NVSDK_NGX_D3D12_GetCapabilityParameters -> (%u %s)"), ResultGetParameters, GetNGXResultAsString(ResultGetParameters));

		if (NVSDK_NGX_Result_FAIL_OutOfDate == ResultGetParameters)
		{
			DLSSQueryFeature.DriverRequirements.DriverUpdateRequired = true;
		}

		if (NVSDK_NGX_SUCCEED(ResultGetParameters))
		{
			DLSSQueryFeature.QueryDLSSSupport();
		}
	}
}

FODID3D12RHI::~FODID3D12RHI()
{
	UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
	if (bNGXInitialized)
	{
		// Destroy the parameters and features before we call NVSDK_NGX_D3D12_Shutdown1
		ReleaseAllocatedFeatures();
		
		NVSDK_NGX_Result Result;
		if (DLSSQueryFeature.CapabilityParameters != nullptr)
		{
			Result = NVSDK_NGX_D3D12_DestroyParameters(DLSSQueryFeature.CapabilityParameters);
			UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("NVSDK_NGX_D3D12_DestroyParameters -> (%u %s)"), Result, GetNGXResultAsString(Result));
		}
		ID3D12Device* Direct3DDevice = D3D12RHI->GetAdapter().GetD3DDevice();
		Result = NVSDK_NGX_D3D12_Shutdown1(Direct3DDevice);
		UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("NVSDK_NGX_D3D12_Shutdown1 -> (%u %s)"), Result, GetNGXResultAsString(Result));
		bNGXInitialized = false;
	}
	UE_LOG(LogDLSSNGXD3D12RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}

void FODID3D12RHI::ExecuteInfer(FRHICommandList& CmdList, const FRHIDLSSArguments& InArguments, FDLSSStateRef InDLSSState)
{
	check(!IsRunningRHIInSeparateThread() || IsInRHIThread());
	check(IsDLSSAvailable());
	if (!IsDLSSAvailable()) return;

	InArguments.Validate();

	FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(CmdList.GetGPUMask().ToIndex());
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = Device->GetCommandContext().CommandListHandle.GraphicsCommandList();
	if (InDLSSState->RequiresFeatureRecreation(InArguments))
	{
		check(!InDLSSState->DLSSFeature || InDLSSState->HasValidFeature());
		InDLSSState->DLSSFeature = nullptr;
	}

	if (InArguments.bReset)
	{
		check(!InDLSSState->DLSSFeature);
		InDLSSState->DLSSFeature = FindFreeFeature(InArguments);
	}

	if (!InDLSSState->DLSSFeature)
	{
		NVSDK_NGX_Parameter* NewNGXParameterHandle = nullptr;
		NVSDK_NGX_Result Result = NVSDK_NGX_D3D12_AllocateParameters(&NewNGXParameterHandle);
		checkf(NVSDK_NGX_SUCCEED(Result), TEXT("NVSDK_NGX_D3D12_AllocateParameters failed! (%u %s)"), Result, GetNGXResultAsString(Result));

		ApplyCommonNGXParameterSettings(NewNGXParameterHandle, InArguments);

		NVSDK_NGX_DLSS_Create_Params DlssCreateParams = InArguments.GetNGXDLSSCreateParams();
		NVSDK_NGX_Handle* NewNGXFeatureHandle = nullptr;

		const uint32 CreationNodeMask = 1 << InArguments.GPUNode;
		const uint32 VisibilityNodeMask = InArguments.GPUVisibility;

		NVSDK_NGX_Result ResultCreate = NGX_D3D12_CREATE_DLSS_EXT(
			D3DGraphicsCommandList,
			CreationNodeMask,
			VisibilityNodeMask,
			&NewNGXFeatureHandle,
			NewNGXParameterHandle,
			&DlssCreateParams
		);
		checkf(NVSDK_NGX_SUCCEED(ResultCreate), TEXT("NGX_D3D12_CREATE_DLSS_EXT (CreationNodeMask=0x%x VisibilityNodeMask=0x%x) failed! (%u %s), %s"), CreationNodeMask, VisibilityNodeMask, ResultCreate, GetNGXResultAsString(ResultCreate), *InArguments.GetFeatureDesc().GetDebugDescription());
		InDLSSState->DLSSFeature = MakeShared<FD3D12NGXDLSSFeature>(NewNGXFeatureHandle, NewNGXParameterHandle, InArguments.GetFeatureDesc(), FrameCounter);
		RegisterFeature(InDLSSState->DLSSFeature);
	}

	check(InDLSSState->HasValidFeature());

	// execute
	if (Device->GetCommandContext().IsDefaultContext())
	{
		Device->RegisterGPUWork(1);
	}

	NVSDK_NGX_D3D12_DLSS_Eval_Params DlssEvalParams;
	FMemory::Memzero(DlssEvalParams);

	DlssEvalParams.Feature.pInOutput = GetD3D12TextureFromRHITexture(InArguments.OutputColor, InArguments.GPUNode)->GetResource()->GetResource();
	DlssEvalParams.InOutputSubrectBase.X = InArguments.DestRect.Min.X;
	DlssEvalParams.InOutputSubrectBase.Y = InArguments.DestRect.Min.Y;

	DlssEvalParams.InRenderSubrectDimensions.Width = InArguments.SrcRect.Width();
	DlssEvalParams.InRenderSubrectDimensions.Height = InArguments.SrcRect.Height();

	DlssEvalParams.Feature.pInColor = GetD3D12TextureFromRHITexture(InArguments.InputColor, InArguments.GPUNode)->GetResource()->GetResource();
	DlssEvalParams.InColorSubrectBase.X = InArguments.SrcRect.Min.X;
	DlssEvalParams.InColorSubrectBase.Y = InArguments.SrcRect.Min.Y;

	DlssEvalParams.pInDepth = GetD3D12TextureFromRHITexture(InArguments.InputDepth, InArguments.GPUNode)->GetResource()->GetResource();
	DlssEvalParams.InDepthSubrectBase.X = InArguments.SrcRect.Min.X;
	DlssEvalParams.InDepthSubrectBase.Y = InArguments.SrcRect.Min.Y;

	DlssEvalParams.pInMotionVectors = GetD3D12TextureFromRHITexture(InArguments.InputMotionVectors, InArguments.GPUNode)->GetResource()->GetResource();
	// The VelocityCombine pass puts the motion vectors into the top left corner
	DlssEvalParams.InMVSubrectBase.X = 0;
	DlssEvalParams.InMVSubrectBase.Y = 0;

	DlssEvalParams.pInExposureTexture = InArguments.bUseAutoExposure ? nullptr : GetD3D12TextureFromRHITexture(InArguments.InputExposure, InArguments.GPUNode)->GetResource()->GetResource();
	DlssEvalParams.InPreExposure = InArguments.PreExposure;

	DlssEvalParams.Feature.InSharpness = InArguments.Sharpness;
	DlssEvalParams.InJitterOffsetX = InArguments.JitterOffset.X;
	DlssEvalParams.InJitterOffsetY = InArguments.JitterOffset.Y;

	DlssEvalParams.InMVScaleX = InArguments.MotionVectorScale.X;
	DlssEvalParams.InMVScaleY = InArguments.MotionVectorScale.Y;
	DlssEvalParams.InReset = InArguments.bReset;

	DlssEvalParams.InFrameTimeDeltaInMsec = InArguments.DeltaTime;

	NVSDK_NGX_Result ResultEvaluate = NGX_D3D12_EVALUATE_DLSS_EXT(
		D3DGraphicsCommandList,
		InDLSSState->DLSSFeature->Feature,
		InDLSSState->DLSSFeature->Parameter,
		&DlssEvalParams
	);
	checkf(NVSDK_NGX_SUCCEED(ResultEvaluate), TEXT("NGX_D3D12_EVALUATE_DLSS_EXT failed! (%u %s), %s"), ResultEvaluate, GetNGXResultAsString(ResultEvaluate), *InDLSSState->DLSSFeature->Desc.GetDebugDescription());
	InDLSSState->DLSSFeature->Tick(FrameCounter);

	Device->GetCommandContext().StateCache.ForceSetComputeRootSignature();
	Device->GetCommandContext().StateCache.GetDescriptorCache()->SetCurrentCommandList(Device->GetCommandContext().CommandListHandle);
	
}

/** IModuleInterface implementation */

void FNGXD3D12RHIModule::StartupModule()
{
	// NGXRHI module should be loaded to ensure logging state is initialized
	FModuleManager::LoadModuleChecked<INGXRHIModule>(TEXT("NGXRHI"));
}

void FNGXD3D12RHIModule::ShutdownModule()
{
}

TUniquePtr<NGXRHI> FODID3D12RHIModule::CreateNGXRHI(const FNGXRHICreateArguments& Arguments)
{
	TUniquePtr<NGXRHI> Result(new FNGXD3D12RHI(Arguments));
	return Result;
}

IMPLEMENT_MODULE(FODID3D12RHIModule, ODID3D12RHI)

#undef LOCTEXT_NAMESPACE





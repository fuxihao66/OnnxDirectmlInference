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
void FODID3D12RHI::CreateDMLResources(){
	// initialize once
	if (m_dmlDevice == nullptr) {
		if (_Debug)
			DMLCreateDevice(m_d3dDevice.Get(), DML_CREATE_DEVICE_FLAG_DEBUG, IID_PPV_ARGS(&m_dmlDevice));
		else
			DMLCreateDevice(m_d3dDevice.Get(), DML_CREATE_DEVICE_FLAG_NONE, IID_PPV_ARGS(&m_dmlDevice));


		DML_FEATURE_QUERY_TENSOR_DATA_TYPE_SUPPORT fp16Query = { DML_TENSOR_DATA_TYPE_FLOAT16 };
		DML_FEATURE_DATA_TENSOR_DATA_TYPE_SUPPORT fp16Supported = {};
		m_dmlDevice->CheckFeatureSupport(DML_FEATURE_TENSOR_DATA_TYPE_SUPPORT, sizeof(fp16Query), &fp16Query, sizeof(fp16Supported), &fp16Supported);

		if (!fp16Supported.IsSupported)
		{
			throw std::exception("Current driver doesn't support FP16, which is required.");
		}
		m_dmlDevice->CreateCommandRecorder(IID_PPV_ARGS(&m_dmlCommandRecorder));
	}
	m_dmlDescriptorHeap = std::make_unique<DescriptorHeapWrapper>(
		m_d3dDevice.Get(),
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		MAX_DESCRIPTOR_COUNT);
	m_currentDescriptorTopIndex = 0;
}

// called in Network
void FODID3D12RHI::ParseAndUploadModelData(const std::wstring& path_to_onnx, const std::string& modelName) {
	m_modelNameToResourceInfo[modelName] = ModelInfo();

	auto& currOnnxInfo = m_modelNameToResourceInfo[modelName];

	// start to parse onnx file
	std::map<std::string, ONNX_PARSER::TensorInfo> inputMap;
	std::map<std::string, ONNX_PARSER::TensorInfo> outputMap;
	std::map<std::string, ONNX_PARSER::InitializerTensorInfo> graphInitializers;
	std::map<std::string, ONNX_PARSER::Op> graphNodes;
	std::vector<ONNX_PARSER::BindingInfo> weightsBinding;
	std::vector<char> dmlWeights;
	unsigned int opsetVersion;

	{
		ONNX_PARSER::OnnxParser* parser = new ONNX_PARSER::OnnxParser(path_to_onnx);
		graphInitializers = parser->GetGraphInitializers(); // error
		outputMap = parser->GetOutputs();
		inputMap = parser->GetInputs();
		weightsBinding = parser->GetBindings();
		dmlWeights = parser->GetWeights();
		graphNodes = parser->GetGraphNodes();
		opsetVersion = parser->GetOpsetVersion();
		delete(parser);
	}

	unsigned int modelInputNum = inputMap.size();
	unsigned int modelOutputNum = outputMap.size();

	currOnnxInfo.modelInputNum = modelInputNum;
	currOnnxInfo.modelOutputNum = modelOutputNum;

	unsigned int currentInputIndex = 0;

	// create graph
	{
		DML_TENSOR_DATA_TYPE dataType = DML_TENSOR_DATA_TYPE_FLOAT16;
		DML_TENSOR_FLAGS flags = DML_TENSOR_FLAG_OWNED_BY_DML;

		dml::TensorPolicy policy = dml::TensorPolicy::Default();// TODO: use NHWC on NV GPU?

		dml::Graph graph(m_dmlDevice.Get(), policy);

		// TODO: model input and output naming constraint?
		std::map<std::string, dml::Expression> expressionMap;
		std::vector<dml::Expression> outputExpression;

		int inputIndex = 0;
		for (auto& inputPair : inputMap) {
			auto& inputInfo = inputPair.second;
			dml::TensorDimensions modelInputShape;

			for (int i = 0; i < inputInfo.dims; i++) {
				modelInputShape.push_back(inputInfo.shapes[i]);
			}
			auto modelInput = dml::InputTensor(graph, inputIndex, dml::TensorDesc(static_cast<DML_TENSOR_DATA_TYPE>(inputInfo.tensorType), modelInputShape, policy));
			expressionMap[inputPair.first] = modelInput;
			inputIndex += 1;
		}

		for (auto& initializerPair : graphInitializers) {
			auto& initializerInfo = initializerPair.second;

			dml::TensorDimensions modelWeightShape;

			for (int i = 0; i < initializerInfo.dims; i++) {
				modelWeightShape.push_back(initializerInfo.shapes[i]);
			}

			expressionMap[initializerPair.first] = dml::InputTensor(graph, initializerInfo.index + modelInputNum,
				dml::TensorDesc(static_cast<DML_TENSOR_DATA_TYPE>(initializerInfo.tensorType), flags, modelWeightShape, policy));

			currentInputIndex = std::max(initializerInfo.index + modelInputNum + 1, currentInputIndex);
		}

		auto TopologicalSort = [](std::map<std::string, ONNX_PARSER::Op>& graph, std::vector<std::string>& sortedKey) {

			struct TempGraphNode {
				std::string graphKey;
				unsigned int dependencyNum;
				bool valid;
				TempGraphNode() : dependencyNum(0), valid(true) {}
			};
			unsigned int nodeNum = graph.size();
			sortedKey.resize(nodeNum);
			std::vector<TempGraphNode> tempGraph(nodeNum);
			std::vector<std::vector<bool>> tempGraphEdge(nodeNum, std::vector<bool>(nodeNum, false));
			unsigned int index = 0;
			for (auto& graphNode : graph) {
				auto& op = graphNode.second;

				tempGraph[op.opIndex].graphKey = graphNode.first;
				for (const std::string& dependencyName : op.inputNames) {
					if (graph.count(dependencyName)) { // depend on other graph node
						tempGraph[op.opIndex].dependencyNum += 1;

						auto& dependencyNode = graph[dependencyName];
						tempGraphEdge[dependencyNode.opIndex][op.opIndex] = true;
					}
				}
			}

			index = 0; // O(N^2), assuming no circle
			for (int i = 0; i < nodeNum; i++) {
				for (int j = 0; j < nodeNum; j++) {
					if (tempGraph[j].dependencyNum == 0 && tempGraph[j].valid) {
						sortedKey[index] = tempGraph[j].graphKey;
						tempGraph[j].valid = false;

						for (int k = 0; k < nodeNum; k++) {
							if (tempGraphEdge[j][k]) {
								tempGraph[k].dependencyNum -= 1;
							}
						}

						break;
					}
				}
				index += 1;
			}
		};


		// weightsBinding, &dmlWeights, weightBytes
		std::vector<std::string> sortedGraphKeys;
		// because we use dmlx to compile whole network to a graph, so we need to deal with operator dependency
		TopologicalSort(graphNodes, sortedGraphKeys);
		// TODO: create dml operator based on onnx operator type
		for (auto& graphNodeKey : sortedGraphKeys) {
			auto& opNode = graphNodes[graphNodeKey];
			if (opNode.opType == "Shape") { // not supported by DML, only STATIC GRAPH is supported right now
				auto& inputExpresion = expressionMap[opNode.inputNames[0]];

				dml::TensorDimensions inputShape = inputExpresion.GetOutputDesc().sizes;

				graphInitializers[graphNodeKey] = ONNX_PARSER::InitializerTensorInfo(graphNodeKey, 1, ONNX_PARSER::TensorType::UINT32, currentInputIndex);
				graphInitializers[graphNodeKey].SetShape(0, inputShape.size());

				auto shapeByteSize = inputShape.size() * sizeof(UINT32);
				auto alignedByteSize = ONNX_PARSER::GetAlignedBytes(shapeByteSize);
				auto prevStride = dmlWeights.size();
				
				weightsBinding.push_back(ONNX_PARSER::BindingInfo(prevStride, shapeByteSize));
				dmlWeights.resize(prevStride + alignedByteSize);
				memcpy(dmlWeights.data() + prevStride, inputShape.data(), shapeByteSize);
				expressionMap[graphNodeKey] = dml::InputTensor(graph, currentInputIndex,
					dml::TensorDesc(static_cast<DML_TENSOR_DATA_TYPE>(ONNX_PARSER::TensorType::UINT32), flags, dml::TensorDimensions{ static_cast<uint32_t>(inputShape.size()) }, policy));
				currentInputIndex += 1;
			}
			else {
				auto expression = CreateExpression(expressionMap, graphInitializers, opNode, graph, opsetVersion);
				expressionMap[graphNodeKey] = expression;
			}

		}

		for (auto it = graphInitializers.begin(); it != graphInitializers.end(); it++) {
			weightsBinding[it->second.index].SetShouldBind(it->second.referredByDml);
		}


		currOnnxInfo.weightsBinding = weightsBinding;


		// TODO: only support single output
		if (modelOutputNum != 1)
			throw std::exception("Only single output is supported.");

		for (auto& outputPair : outputMap) {
			auto& output = outputPair.second;
			outputExpression.push_back(expressionMap[output.name]);
		}

		DML_EXECUTION_FLAGS executionFlags = DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION;

		currOnnxInfo.dmlGraph = graph.Compile(executionFlags, std::array<dml::Expression, 1>{ outputExpression[0] });
	}

	if (dmlWeights.size() > 0){ // TODO: dont need to be on default heap, DML supports resource on upload heap if tensor is owned by DML
		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dmlWeights.size(), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		CD3DX12_RANGE readRange(0, 0);
		m_d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(currOnnxInfo.modelOperatorWeights.ReleaseAndGetAddressOf()));


		D3D12_SUBRESOURCE_DATA weightsData = {};
		weightsData.pData = dmlWeights.data();
		UINT64 uploadSize = GetRequiredIntermediateSize(
			currOnnxInfo.modelOperatorWeights.Get(),
			0,
			1);
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

		// Create a temporary buffer
		currOnnxInfo.scratchResource = nullptr;
		m_d3dDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, // D3D12_CLEAR_VALUE* pOptimizedClearValue
			IID_PPV_ARGS(currOnnxInfo.scratchResource.ReleaseAndGetAddressOf()));

		// Submit resource copy to command list
		UpdateSubresources(m_commandList.Get(), currOnnxInfo.modelOperatorWeights.Get(), currOnnxInfo.scratchResource.Get(), 0, 0, 1, &weightsData);
	}
}
void FODID3D12RHI::InitializeNewModel(const std::string& modelName){
	auto& currOnnxInfo = m_modelNameToResourceInfo[modelName];
	auto& weightsBinding = currOnnxInfo.weightsBinding;
	auto modelInputNum = currOnnxInfo.modelInputNum;


	m_dmlDevice->CreateOperatorInitializer(1, currOnnxInfo.dmlGraph.GetAddressOf(), IID_PPV_ARGS(&currOnnxInfo.dmlOpInitializer));

	DML_BINDING_PROPERTIES initBindingProps = currOnnxInfo.dmlOpInitializer->GetBindingProperties();
	DML_BINDING_PROPERTIES executeBindingProps = currOnnxInfo.dmlGraph->GetBindingProperties();



	// Operator initialization dispatches will use this heap right away
	ID3D12DescriptorHeap* pHeaps[] = { m_dmlDescriptorHeap->Heap() };
	m_commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

	// Create any persistent resources required for the operators.
	if (executeBindingProps.PersistentResourceSize > 0)
	{
		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
			executeBindingProps.PersistentResourceSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		m_d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&currOnnxInfo.modelPersistentResource));
	}

	// Temporary resource for execution
	if (executeBindingProps.TemporaryResourceSize > 0)
	{
		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
			executeBindingProps.TemporaryResourceSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		m_d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&currOnnxInfo.modelTemporaryResource));
	}

	// If the execute temporary resource isn't big enough for initialization, create a bigger buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> initTemporaryResource;
	if (initBindingProps.TemporaryResourceSize > executeBindingProps.TemporaryResourceSize)
	{
		D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(
			initBindingProps.TemporaryResourceSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		m_d3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&initTemporaryResource));
	}
	else if (initBindingProps.TemporaryResourceSize > 0)
	{
		initTemporaryResource = currOnnxInfo.modelTemporaryResource;
	}

	Microsoft::WRL::ComPtr<IDMLBindingTable> initBindingTable;
	assert(initBindingProps.PersistentResourceSize == 0);

	DML_BINDING_TABLE_DESC tableDesc =
	{
		currOnnxInfo.dmlOpInitializer.Get(),
		//m_dmlDescriptorHeap->GetCpuHandle(currOnnxInfo.descriptorCPUOffset), // TODO: 
		//m_dmlDescriptorHeap->GetGpuHandle(currOnnxInfo.descriptorGPUOffset),
		m_dmlDescriptorHeap->GetCpuHandle(m_currentDescriptorTopIndex),
		m_dmlDescriptorHeap->GetGpuHandle(m_currentDescriptorTopIndex),
		initBindingProps.RequiredDescriptorCount
	};

	m_currentDescriptorTopIndex += initBindingProps.RequiredDescriptorCount;

	m_dmlDevice->CreateBindingTable(&tableDesc, IID_PPV_ARGS(&initBindingTable));



	// TODO:
	currOnnxInfo.inputBindings.resize(weightsBinding.size() + modelInputNum);
	
	std::vector<DML_BUFFER_BINDING> bufferBindings(weightsBinding.size() + modelInputNum);
	for (int i = modelInputNum; i < bufferBindings.size(); i++) {

		auto& bindingInfo = weightsBinding[i - modelInputNum];
		if (bindingInfo.GetShouldBind())
			bufferBindings[i] = { currOnnxInfo.modelOperatorWeights.Get(), bindingInfo.stride, bindingInfo.byteSize };
		currOnnxInfo.inputBindings[i] = DML_BINDING_DESC{ DML_BINDING_TYPE_NONE, nullptr };
	}

	DML_BUFFER_ARRAY_BINDING initInputBinding = { bufferBindings.size(), bufferBindings.data() };
	initBindingTable->BindInputs(1, &DML_BINDING_DESC{ DML_BINDING_TYPE_BUFFER_ARRAY, &initInputBinding });

	if (initTemporaryResource)
	{
		DML_BUFFER_BINDING binding = { initTemporaryResource.Get(), 0, initTemporaryResource->GetDesc().Width };
		initBindingTable->BindTemporaryResource(&DML_BINDING_DESC{ DML_BINDING_TYPE_BUFFER, &binding });
	}

	// If the operator requires a persistent resource, it must be bound as output for the initializer.
	if (currOnnxInfo.modelPersistentResource)
	{
		DML_BUFFER_BINDING binding = { currOnnxInfo.modelPersistentResource.Get(), 0, currOnnxInfo.modelPersistentResource->GetDesc().Width };
		initBindingTable->BindOutputs(1, &DML_BINDING_DESC{ DML_BINDING_TYPE_BUFFER, &binding });
	}

	// Record the initialization
	m_dmlCommandRecorder->RecordDispatch(m_commandList.Get(), currOnnxInfo.dmlOpInitializer.Get(), initBindingTable.Get());

	

	tableDesc.Dispatchable = currOnnxInfo.dmlGraph.Get();
	tableDesc.SizeInDescriptors = executeBindingProps.RequiredDescriptorCount;
	m_dmlDevice->CreateBindingTable(&tableDesc, IID_PPV_ARGS(&currOnnxInfo.dmlBindingTable));

	// ForceCPUSync(); // TODO: sync is required
	
}
void FODID3D12RHI::BindResources(){
	if (currOnnxInfo.PersAndTempResourceIsBinded){
		return;
	}
	if (currOnnxInfo.modelPersistentResource)
	{
		DML_BUFFER_BINDING binding = { currOnnxInfo.modelPersistentResource.Get(), 0, currOnnxInfo.modelPersistentResource->GetDesc().Width };
		currOnnxInfo.dmlBindingTable->BindPersistentResource(&DML_BINDING_DESC{ DML_BINDING_TYPE_BUFFER, &binding });
	}

	if (currOnnxInfo.modelTemporaryResource)
	{
		DML_BUFFER_BINDING binding = { currOnnxInfo.modelTemporaryResource.Get(), 0, currOnnxInfo.modelTemporaryResource->GetDesc().Width };
		currOnnxInfo.dmlBindingTable->BindTemporaryResource(&DML_BINDING_DESC{ DML_BINDING_TYPE_BUFFER, &binding });
	}
	currOnnxInfo.PersAndTempResourceIsBinded = true;
}
void FODID3D12RHI::ExecuteInfer(FRHICommandList& CmdList, const FRHIDLSSArguments& InArguments, FDLSSStateRef InDLSSState)
{
	check(!IsRunningRHIInSeparateThread() || IsInRHIThread());

	// const std::map<std::string, FRHITextureRef> InArguments.ModelInputs
	
	ID3D12DescriptorHeap* PreviousHeaps[2] =
	{
		StateCache.GetDescriptorCache()->GetCurrentViewHeap()->GetHeap(),
		StateCache.GetDescriptorCache()->GetCurrentSamplerHeap()->GetHeap(),
	};

	FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(CmdList.GetGPUMask().ToIndex());
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = Device->GetCommandContext().CommandListHandle.GraphicsCommandList();

	auto & modelInputs = InArguments.ModelInputs;
	auto & modelOutput = GetD3D12TextureFromRHITexture(InArguments.ModelOutput, InArguments.GPUNode)->GetResource()->GetResource();


	ID3D12DescriptorHeap* pHeaps[] = { m_dmlDescriptorHeap->Heap() };
	D3DGraphicsCommandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);
	
	auto& currOnnxInfo = m_modelNameToResourceInfo[modelName];

	auto modelInputNum = currOnnxInfo.modelInputNum;
	auto modelOutputNum = currOnnxInfo.modelOutputNum;
	auto dmlGraph = currOnnxInfo.dmlGraph;
	auto dmlBindingTable = currOnnxInfo.dmlBindingTable;

	int inputIndex = 0;
	std::vector<DML_BUFFER_BINDING> bufferBindings(modelInputs.size());
	for (auto& input : modelInputs) { // because std::map is sorted

		auto resourcePointer = GetD3D12TextureFromRHITexture(input.second, InArguments.GPUNode)->GetResource()->GetResource();
		bufferBindings[inputIndex] = DML_BUFFER_BINDING{ resourcePointer }; // TODO: buffer size might be larger than tensor size (because buffer is 16 byte aligned)
		currOnnxInfo.inputBindings[inputIndex] = { DML_BINDING_TYPE_BUFFER, &bufferBindings[inputIndex] };

		inputIndex += 1;
	}
	
	dmlBindingTable->BindInputs(currOnnxInfo.inputBindings.size(), currOnnxInfo.inputBindings.data());

	DML_BUFFER_BINDING outputBinding = { modelOutput, 0, modelOutput->GetDesc().Width }; // TODO: buffer size might be larger than tensor size (because buffer is 16 byte aligned)
	dmlBindingTable->BindOutputs(1, &DML_BINDING_DESC{ DML_BINDING_TYPE_BUFFER, &outputBinding });

	m_dmlCommandRecorder->RecordDispatch(D3DGraphicsCommandList.Get(), dmlGraph.Get(), dmlBindingTable.Get());
	// execute

	Device->GetCommandContext().CommandListHandle.AddUAVBarrier();


	if (Device->GetCommandContext().IsDefaultContext())
	{
		Device->RegisterGPUWork(1);
	}

	D3DGraphicsCommandList->SetDescriptorHeaps(2, PreviousHeaps);

	Device->GetCommandContext().StateCache.ForceSetComputeRootSignature();
	// Device->GetCommandContext().StateCache.GetDescriptorCache()->SetCurrentCommandList(Device->GetCommandContext().CommandListHandle);
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





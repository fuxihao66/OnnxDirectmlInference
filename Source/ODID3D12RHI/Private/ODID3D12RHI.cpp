#include "ODID3D12RHI.h"

#include "D3D12RHIPrivate.h"
#include "D3D12Util.h"
#include "D3D12State.h"
#include "D3D12Resources.h"
#include "D3D12Viewport.h"
#include "D3D12ConstantBuffer.h"
#include "D3D12CommandContext.h"
#include "RHIValidationCommon.h"
#include "ID3D12DynamicRHI.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Interfaces/IPluginManager.h"

#include "OnnxParser.h"
#include "OnnxDMLOperatorMapping.h"

#include <unordered_map>

//#pragma comment(lib, "d3d12.lib")


DEFINE_LOG_CATEGORY_STATIC(LogODID3D12RHI, Log, All);

#define LOCTEXT_NAMESPACE "FODID3D12RHIModule"
#define _Debug 0

struct ModelInfo {
	bool 												PersAndTempResourceIsBinded;
	unsigned int                                        modelInputNum;
	unsigned int                                        modelOutputNum;
	std::vector<ONNX_PARSER::BindingInfo>               weightsBinding;
	/*unsigned int                                        descriptorCPUOffset;
	unsigned int                                        descriptorGPUOffset;*/
	std::vector<DML_BINDING_DESC>                       inputBindings;
	Microsoft::WRL::ComPtr<IDMLCompiledOperator>        dmlGraph;
	Microsoft::WRL::ComPtr<IDMLOperatorInitializer>     dmlOpInitializer;
	Microsoft::WRL::ComPtr<ID3D12Resource>              modelPersistentResource;
	Microsoft::WRL::ComPtr<ID3D12Resource>              modelTemporaryResource;
	Microsoft::WRL::ComPtr<ID3D12Resource>              modelOperatorWeights;
	Microsoft::WRL::ComPtr<ID3D12Resource>              scratchResource; // used for upload weights
	Microsoft::WRL::ComPtr<IDMLBindingTable>            dmlBindingTable;
	ModelInfo() : PersAndTempResourceIsBinded(false), modelInputNum(0), modelOutputNum(0), dmlGraph(nullptr), dmlOpInitializer(nullptr),
		modelPersistentResource(nullptr), modelTemporaryResource(nullptr), modelOperatorWeights(nullptr), dmlBindingTable(nullptr)
	{}
	void Destroy(){
		dmlGraph = nullptr;
		dmlOpInitializer = nullptr;
		modelPersistentResource = nullptr;
		modelTemporaryResource = nullptr;
		modelOperatorWeights = nullptr;
		scratchResource = nullptr;
		dmlBindingTable = nullptr;
	}
};

class DescriptorHeapWrapper 
{
private:
	void Create(
		ID3D12Device* pDevice,
		const D3D12_DESCRIPTOR_HEAP_DESC* pDesc)
	{
		assert(pDesc != nullptr);

		m_desc = *pDesc;
		m_increment = pDevice->GetDescriptorHandleIncrementSize(pDesc->Type);

		if (pDesc->NumDescriptors == 0)
		{
			m_pHeap.Reset();
			m_hCPU.ptr = 0;
			m_hGPU.ptr = 0;
		}
		else
		{
			pDevice->CreateDescriptorHeap(
				pDesc,
				IID_PPV_ARGS(m_pHeap.ReleaseAndGetAddressOf()));

			m_hCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();

			if (pDesc->Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
				m_hGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();

		}
	}

public:
	~DescriptorHeapWrapper(){
		m_pHeap = nullptr;
	}
	DescriptorHeapWrapper(
		_In_ ID3D12Device* device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags,
		size_t count) :
		m_desc{},
		m_hCPU{},
		m_hGPU{},
		m_increment(0)
	{
		if (count > UINT32_MAX)
			throw std::exception("Too many descriptors");

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Flags = flags;
		desc.NumDescriptors = static_cast<UINT>(count);
		desc.Type = type;
		Create(device, &desc);
	}
	ID3D12DescriptorHeap* Heap() {
		return m_pHeap.Get();
	}
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(_In_ size_t index) const
	{
		assert(m_pHeap != nullptr);
		if (index >= m_desc.NumDescriptors)
		{
			throw std::out_of_range("D3DX12_CPU_DESCRIPTOR_HANDLE");
		}

		D3D12_CPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = static_cast<SIZE_T>(m_hCPU.ptr + UINT64(index) * UINT64(m_increment));
		return handle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(_In_ size_t index) const
	{
		assert(m_pHeap != nullptr);
		if (index >= m_desc.NumDescriptors)
		{
			throw std::out_of_range("D3DX12_GPU_DESCRIPTOR_HANDLE");
		}
		assert(m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

		D3D12_GPU_DESCRIPTOR_HANDLE handle;
		handle.ptr = m_hGPU.ptr + UINT64(index) * UINT64(m_increment);
		return handle;
	}
private:
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_pHeap;
	D3D12_DESCRIPTOR_HEAP_DESC                      m_desc;
	D3D12_CPU_DESCRIPTOR_HANDLE                     m_hCPU;
	D3D12_GPU_DESCRIPTOR_HANDLE                     m_hGPU;
	uint32_t                                        m_increment;
};
class FODID3D12RHI final : public ODIRHI
{
private:
	Microsoft::WRL::ComPtr<IDMLDevice>              m_dmlDevice;
	Microsoft::WRL::ComPtr<IDMLCommandRecorder>     m_dmlCommandRecorder;
	std::unique_ptr<DescriptorHeapWrapper>        	m_dmlDescriptorHeap;
	
	std::unordered_map<std::string, ModelInfo>      m_modelNameToResourceInfo; // model info (for supporting different models)
	UINT                                            m_currentDescriptorTopIndex;
public:
	FODID3D12RHI(const FODIRHICreateArguments& Arguments);
	virtual ODI_Result ParseAndUploadModelData(FRHICommandList& CmdList, const std::wstring& path_to_onnx, const std::string& model_name);
	virtual ODI_Result InitializeNewModel(FRHICommandList& CmdList, const std::string& model_name);
	virtual ODI_Result BindResources(const std::string& model_name);
	virtual ODI_Result ExecuteInference(FRHICommandList& CmdList, const FODIRHIInferArguments& InArguments) final;
	virtual ~FODID3D12RHI();
private:
	ODI_Result CreateDMLResources();

	ID3D12Device* m_d3dDevice = nullptr;
	ID3D12DynamicRHI* D3D12RHI = nullptr;
};


FODID3D12RHI::FODID3D12RHI(const FODIRHICreateArguments& Arguments)
	: ODIRHI(Arguments)
	, D3D12RHI(CastDynamicRHI<ID3D12DynamicRHI>(Arguments.DynamicRHI))
	//, D3D12RHI(static_cast<FD3D12DynamicRHI*>(Arguments.DynamicRHI))
{
	//ID3D12Device* Direct3DDevice = D3D12RHI->GetAdapter().GetD3DDevice();
	ID3D12Device* Direct3DDevice = D3D12RHI->RHIGetDevice(0);


	ensure(D3D12RHI);
	ensure(Direct3DDevice);

	// const FString NGXLogDir = GetNGXLogDirectory();
	// IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*NGXLogDir);
	m_d3dDevice = Direct3DDevice;
	ODI_Result ResultInit = CreateDMLResources();
	UE_LOG(LogODID3D12RHI, Log, TEXT("ODI_D3D12_Init"));
	
}

FODID3D12RHI::~FODID3D12RHI()
{
	for (auto it = m_modelNameToResourceInfo.begin(); it != m_modelNameToResourceInfo.end(); it++)
		it->second.Destroy();
	m_dmlDevice = nullptr;
	m_dmlCommandRecorder = nullptr;
	m_dmlDescriptorHeap = nullptr;
	UE_LOG(LogODID3D12RHI, Log, TEXT("%s Enter"), ANSI_TO_TCHAR(__FUNCTION__));
	
	UE_LOG(LogODID3D12RHI, Log, TEXT("%s Leave"), ANSI_TO_TCHAR(__FUNCTION__));
}
ODI_Result FODID3D12RHI::CreateDMLResources(){
	// initialize once
	if (m_dmlDevice == nullptr) {
		if (_Debug)
			DMLCreateDevice(m_d3dDevice, DML_CREATE_DEVICE_FLAG_DEBUG, IID_PPV_ARGS(&m_dmlDevice));
		else
			DMLCreateDevice(m_d3dDevice, DML_CREATE_DEVICE_FLAG_NONE, IID_PPV_ARGS(&m_dmlDevice));


		DML_FEATURE_QUERY_TENSOR_DATA_TYPE_SUPPORT fp16Query = { DML_TENSOR_DATA_TYPE_FLOAT16 };
		DML_FEATURE_DATA_TENSOR_DATA_TYPE_SUPPORT fp16Supported = {};
		m_dmlDevice->CheckFeatureSupport(DML_FEATURE_TENSOR_DATA_TYPE_SUPPORT, sizeof(fp16Query), &fp16Query, sizeof(fp16Supported), &fp16Supported);

		if (!fp16Supported.IsSupported)
		{
			UE_LOG(LogODID3D12RHI, Log, TEXT("Current driver doesn't support FP16, which is required."));
			return ODI_Result::ODI_Result_Fail;
		}
		m_dmlDevice->CreateCommandRecorder(IID_PPV_ARGS(&m_dmlCommandRecorder));
	}
	m_dmlDescriptorHeap = std::make_unique<DescriptorHeapWrapper>(
		m_d3dDevice,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		MAX_DESCRIPTOR_COUNT);
	m_currentDescriptorTopIndex = 0;

	return ODI_Result::ODI_Result_Success;
}

// called in Network
ODI_Result FODID3D12RHI::ParseAndUploadModelData(FRHICommandList& CmdList, const std::wstring& path_to_onnx, const std::string& model_name) {
	/*FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(CmdList.GetGPUMask().ToIndex());
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = Device->GetDefaultCommandContext().GraphicsCommandList().Get();*/
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = D3D12RHI->RHIGetGraphicsCommandList(0);

	// New API
	/*const uint32 DeviceIndex = D3D12RHI->RHIGetResourceDeviceIndex(InArguments.InputColor);
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = D3D12RHI->RHIGetGraphicsCommandList(DeviceIndex);*/

	m_modelNameToResourceInfo[model_name] = ModelInfo();

	auto& currOnnxInfo = m_modelNameToResourceInfo[model_name];

	// start to parse onnx file
	std::map<std::string, ONNX_PARSER::TensorInfo> inputMap;
	std::map<std::string, ONNX_PARSER::TensorInfo> outputMap;
	std::map<std::string, ONNX_PARSER::InitializerTensorInfo> graphInitializers;
	std::map<std::string, ONNX_PARSER::Op> graphNodes;
	std::vector<ONNX_PARSER::BindingInfo> weightsBinding;
	std::vector<char> dmlWeights;
	unsigned int opsetVersion;

	{
		ONNX_PARSER::OnnxParser* parser = new ONNX_PARSER::OnnxParser(path_to_onnx.c_str());
		graphInitializers = parser->GetGraphInitializers(); // error
		outputMap = parser->GetOutputs();
		inputMap = parser->GetInputs();
		weightsBinding = parser->GetBindings();
		dmlWeights = parser->GetWeights();
		graphNodes = parser->GetGraphNodes();
		opsetVersion = parser->GetOpsetVersion();
		delete(parser);
	}
	//{
	//	ONNX_PARSER::OnnxParser parser(path_to_onnx.c_str());
	//	graphInitializers = parser.GetGraphInitializers(); // error
	//	outputMap = parser.GetOutputs();
	//	inputMap = parser.GetInputs();
	//	weightsBinding = parser.GetBindings();
	//	dmlWeights = parser.GetWeights();
	//	graphNodes = parser.GetGraphNodes();
	//	opsetVersion = parser.GetOpsetVersion();
	//}

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

			for (size_t i = 0; i < inputInfo.dims; i++) {
				modelInputShape.push_back(inputInfo.shapes[i]);
			}
			auto modelInput = dml::InputTensor(graph, inputIndex, dml::TensorDesc(static_cast<DML_TENSOR_DATA_TYPE>(inputInfo.tensorType), modelInputShape, policy));
			expressionMap[inputPair.first] = modelInput;
			inputIndex += 1;
		}

		for (auto& initializerPair : graphInitializers) {
			auto& initializerInfo = initializerPair.second;

			dml::TensorDimensions modelWeightShape;

			for (size_t i = 0; i < initializerInfo.dims; i++) {
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
			for (size_t i = 0; i < nodeNum; i++) {
				for (size_t j = 0; j < nodeNum; j++) {
					if (tempGraph[j].dependencyNum == 0 && tempGraph[j].valid) {
						sortedKey[index] = tempGraph[j].graphKey;
						tempGraph[j].valid = false;

						for (size_t k = 0; k < nodeNum; k++) {
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
				auto expression = ODI::CreateExpression(expressionMap, graphInitializers, opNode, graph, opsetVersion);
				expressionMap[graphNodeKey] = expression;
			}

		}

		for (auto it = graphInitializers.begin(); it != graphInitializers.end(); it++) {
			weightsBinding[it->second.index].SetShouldBind(it->second.referredByDml);
		}


		currOnnxInfo.weightsBinding = weightsBinding;


		// TODO: only support single output
		if (modelOutputNum != 1){
			UE_LOG(LogODID3D12RHI, Log, TEXT("Only single output is supported."));
			// throw std::exception("Only single output is supported.");
			return ODI_Result::ODI_Result_Fail;
		}
		

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
		currOnnxInfo.modelOperatorWeights->SetName(L"NetworkWights");

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
		UpdateSubresources(D3DGraphicsCommandList, currOnnxInfo.modelOperatorWeights.Get(), currOnnxInfo.scratchResource.Get(), 0, 0, 1, &weightsData);
	
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(currOnnxInfo.modelOperatorWeights.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		D3DGraphicsCommandList->ResourceBarrier(1, &barrier);
	}
	return ODI_Result::ODI_Result_Success;
}
ODI_Result FODID3D12RHI::InitializeNewModel(FRHICommandList& CmdList, const std::string& model_name){
	/*ID3D12DescriptorHeap* PreviousHeaps[2] =
	{
		StateCache.GetDescriptorCache()->GetCurrentViewHeap()->GetHeap(),
		StateCache.GetDescriptorCache()->GetCurrentSamplerHeap()->GetHeap(),
	};*/
	
	/*FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(CmdList.GetGPUMask().ToIndex());
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = Device->GetDefaultCommandContext().GraphicsCommandList().Get();*/
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = D3D12RHI->RHIGetGraphicsCommandList(0);



	auto& currOnnxInfo = m_modelNameToResourceInfo[model_name];
	auto& weightsBinding = currOnnxInfo.weightsBinding;
	auto modelInputNum = currOnnxInfo.modelInputNum;

	m_dmlDevice->CreateOperatorInitializer(1, currOnnxInfo.dmlGraph.GetAddressOf(), IID_PPV_ARGS(&currOnnxInfo.dmlOpInitializer));

	DML_BINDING_PROPERTIES initBindingProps = currOnnxInfo.dmlOpInitializer->GetBindingProperties();
	DML_BINDING_PROPERTIES executeBindingProps = currOnnxInfo.dmlGraph->GetBindingProperties();



	// Operator initialization dispatches will use this heap right away
	ID3D12DescriptorHeap* pHeaps[] = { m_dmlDescriptorHeap->Heap() };
	D3DGraphicsCommandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

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
	m_dmlCommandRecorder->RecordDispatch(D3DGraphicsCommandList, currOnnxInfo.dmlOpInitializer.Get(), initBindingTable.Get());

	

	tableDesc.Dispatchable = currOnnxInfo.dmlGraph.Get();
	tableDesc.SizeInDescriptors = executeBindingProps.RequiredDescriptorCount;
	m_dmlDevice->CreateBindingTable(&tableDesc, IID_PPV_ARGS(&currOnnxInfo.dmlBindingTable));

	//D3DGraphicsCommandList->SetDescriptorHeaps(2, PreviousHeaps);
	/*Device->GetDefaultCommandContext().StateCache.GetDescriptorCache()->SetDescriptorHeaps(true);
	Device->GetDefaultCommandContext().StateCache.ForceSetComputeRootSignature();*/

	D3D12RHI->RHIFinishExternalComputeWork(0, D3DGraphicsCommandList); // New API

	// ForceCPUSync(); 
	// TODO: sync is required
	

	return ODI_Result::ODI_Result_Success;
}
ODI_Result FODID3D12RHI::BindResources(const std::string& model_name){
	auto & currOnnxInfo = m_modelNameToResourceInfo[model_name];
	if (currOnnxInfo.PersAndTempResourceIsBinded){
		return ODI_Result::ODI_Result_Success;
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

	return ODI_Result::ODI_Result_Success;
}
ODI_Result FODID3D12RHI::ExecuteInference(FRHICommandList& CmdList, const FODIRHIInferArguments& InArguments)
{
	check(!IsRunningRHIInSeparateThread() || IsInRHIThread());

	// const std::map<std::string, FRHITextureRef> InArguments.ModelInputs
	
	/*ID3D12DescriptorHeap* PreviousHeaps[2] =
	{
		StateCache.GetDescriptorCache()->GetCurrentViewHeap()->GetHeap(),
		StateCache.GetDescriptorCache()->GetCurrentSamplerHeap()->GetHeap(),
	};*/

	/*FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(CmdList.GetGPUMask().ToIndex());
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = Device->GetCommandContext().CommandListHandle.GraphicsCommandList();*/
	/*FD3D12Device* Device = D3D12RHI->GetAdapter().GetDevice(CmdList.GetGPUMask().ToIndex());
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = Device->GetDefaultCommandContext().GraphicsCommandList().Get();*/
	ID3D12GraphicsCommandList* D3DGraphicsCommandList = D3D12RHI->RHIGetGraphicsCommandList(0);


	auto & modelInputs = InArguments.InputBuffers;
	auto modelOutputResourcePointer = D3D12RHI->RHIGetResource(InArguments.OutputBuffer); // version >= 5.1


	ID3D12DescriptorHeap* pHeaps[] = { m_dmlDescriptorHeap->Heap() };
	D3DGraphicsCommandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);
	
	auto& currOnnxInfo = m_modelNameToResourceInfo[InArguments.ModelName];

	auto modelInputNum = currOnnxInfo.modelInputNum;
	auto modelOutputNum = currOnnxInfo.modelOutputNum;
	auto dmlGraph = currOnnxInfo.dmlGraph;
	auto dmlBindingTable = currOnnxInfo.dmlBindingTable;

	int inputIndex = 0;
	std::vector<DML_BUFFER_BINDING> bufferBindings(modelInputs.size());
	for (auto& input : modelInputs) { // because std::map is sorted

		auto resourcePointer = D3D12RHI->RHIGetResource(input.second); //, InArguments.GPUNode)->GetResource()->GetResource();
		bufferBindings[inputIndex] = DML_BUFFER_BINDING{ resourcePointer }; // TODO: buffer size might be larger than tensor size (because buffer is 16 byte aligned)
		currOnnxInfo.inputBindings[inputIndex] = { DML_BINDING_TYPE_BUFFER, &bufferBindings[inputIndex] };

		inputIndex += 1;
	}
	
	dmlBindingTable->BindInputs(currOnnxInfo.inputBindings.size(), currOnnxInfo.inputBindings.data());

	DML_BUFFER_BINDING outputBinding = { modelOutputResourcePointer, 0, modelOutputResourcePointer->GetDesc().Width }; // TODO: buffer size might be larger than tensor size (because buffer is 16 byte aligned)
	dmlBindingTable->BindOutputs(1, &DML_BINDING_DESC{ DML_BINDING_TYPE_BUFFER, &outputBinding });

	m_dmlCommandRecorder->RecordDispatch(D3DGraphicsCommandList, dmlGraph.Get(), dmlBindingTable.Get());
	// execute

	//Device->GetDefaultCommandContext().AddUAVBarrier();
	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
	D3DGraphicsCommandList->ResourceBarrier(1, &barrier);

	/*if (Device->GetDefaultCommandContext().IsDefaultContext())
	{
		Device->RegisterGPUWork(1);
	}*/

	//D3DGraphicsCommandList->SetDescriptorHeaps(2, PreviousHeaps);
	/*Device->GetDefaultCommandContext().StateCache.GetDescriptorCache()->SetDescriptorHeaps(true);
	Device->GetDefaultCommandContext().StateCache.ForceSetComputeRootSignature();*/

	D3D12RHI->RHIFinishExternalComputeWork(0, D3DGraphicsCommandList); // New API

	// Device->GetCommandContext().StateCache.GetDescriptorCache()->SetCurrentCommandList(Device->GetCommandContext().CommandListHandle);
	return ODI_Result::ODI_Result_Success;
}

/** IModuleInterface implementation */

void FODID3D12RHIModule::StartupModule()
{
	FString BaseDir = IPluginManager::Get().FindPlugin("ODI")->GetBaseDir();
	FString DmlBinariesRoot = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/DirectML_1_9_1/bin/Win64/"));
#if _Debug
	DirectMLLibraryHandle = FPlatformProcess::GetDllHandle(*(DmlBinariesRoot + "DirectML.Debug.dll"));
#else
	DirectMLLibraryHandle = FPlatformProcess::GetDllHandle(*(DmlBinariesRoot + "DirectML.dll"));
#endif
	/*FString OnnxParserBinariesRoot = FPaths::Combine(*BaseDir, TEXT("Source/ODID3D12RHI/OnnxParser/"));
	OnnxParserLibraryHandle = FPlatformProcess::GetDllHandle(*(OnnxParserBinariesRoot + "OnnxParser.dll"));*/


	// ODIRHI module should be loaded to ensure logging state is initialized
	FModuleManager::LoadModuleChecked<IODIRHIModule>(TEXT("ODIRHI"));
}

void FODID3D12RHIModule::ShutdownModule()
{
	FPlatformProcess::FreeDllHandle(DirectMLLibraryHandle);
	DirectMLLibraryHandle = nullptr;

	/*FPlatformProcess::FreeDllHandle(OnnxParserLibraryHandle);
	OnnxParserLibraryHandle = nullptr;*/
}

TUniquePtr<ODIRHI> FODID3D12RHIModule::CreateODIRHI(const FODIRHICreateArguments& Arguments)
{
	TUniquePtr<ODIRHI> Result(new FODID3D12RHI(Arguments));
	return Result;
}

IMPLEMENT_MODULE(FODID3D12RHIModule, ODID3D12RHI)

#undef LOCTEXT_NAMESPACE





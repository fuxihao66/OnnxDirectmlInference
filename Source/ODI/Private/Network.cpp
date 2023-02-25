

#include "Network.h"
#include "ODI.h"

#include <map>
#include <locale>
#include <codecvt>

BEGIN_SHADER_PARAMETER_STRUCT(FBlankParameters, )
END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FInferenceParameters, )
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelInput0)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelInput1)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelInput2)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelInput3)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelInput4)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelInput5)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelInput6)
SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, ModelOutput)
END_SHADER_PARAMETER_STRUCT()

#define PARAMETER_MODELINPUT_SLOT(Parameter, index) \
Parameter->ModelInput##index;


Network::Network(const std::wstring& onnx_file_path, const std::string& model_name){
    FODIModule* LocalODIModuleRef = &FModuleManager::LoadModuleChecked<FODIModule>("ODI"); // load module
	m_ODIRHIExtensions = LocalODIModuleRef->GetODIRHIRef();

    m_onnx_file_path = onnx_file_path;

	if (model_name == "") {
		using convert_type = std::codecvt_utf8<wchar_t>;
		std::wstring_convert<convert_type, wchar_t> converter;

		//use converter (.to_bytes: wstr->str, .from_bytes: str->wstr)
		m_model_name = converter.to_bytes(m_onnx_file_path);
	}
    else
        m_model_name = model_name;
}



bool Network::CreateModelAndUploadData(FRDGBuilder& GraphBuilder){
	FBlankParameters* PassParameters = GraphBuilder.AllocParameters<FBlankParameters>();

    GraphBuilder.AddPass(
        RDG_EVENT_NAME("Create Model"),
        PassParameters,
        ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
        [LocalODIRHIExtensions = m_ODIRHIExtensions, model_name = m_model_name, onnx_file_path = m_onnx_file_path](FRHICommandListImmediate& RHICmdList)
    {

        RHICmdList.EnqueueLambda(
            [LocalODIRHIExtensions, model_name, onnx_file_path](FRHICommandListImmediate& Cmd) mutable
        {

            // LocalODIModuleRef->CreateAndInitializeModel(Cmd, DLSSArguments);
			LocalODIRHIExtensions->ParseAndUploadModelData(Cmd, onnx_file_path, model_name);
			LocalODIRHIExtensions->InitializeNewModel(Cmd, model_name);
            // TODO: synchronize

        });
        FRHICommandListExecutor::GetImmediateCommandList().BlockUntilGPUIdle(); 
    });

}

bool Network::ExecuteInference(FRDGBuilder& GraphBuilder, std::map<std::string, FRDGBufferRef> InputBuffers, FRDGBufferRef OutputBuffer){
	FInferenceParameters* PassParameters = GraphBuilder.AllocParameters<FInferenceParameters>();


	PARAMETER_MODELINPUT_SLOT(PassParameters, 0);
	PARAMETER_MODELINPUT_SLOT(PassParameters, 1);
	PARAMETER_MODELINPUT_SLOT(PassParameters, 2);
	PARAMETER_MODELINPUT_SLOT(PassParameters, 3);
	PARAMETER_MODELINPUT_SLOT(PassParameters, 4);
	PARAMETER_MODELINPUT_SLOT(PassParameters, 5);

	int inputIndex = 0;
	for (auto it = InputBuffers.begin(); it != InputBuffers.end(); it++) {
		BIND_INPUT_TO_PARAMETER(PassParameters, inputIndex, GraphBuilder.CreateUAV(it->second));
		MARK_RESOURCE_AS_USED(PassParameters, inputIndex);
		inputIndex += 1;
	}
	PassParameters->ModelOutput = GraphBuilder.CreateUAV(OutputBuffer);
	PassParameters->ModelOutput->MarkResourceAsUsed();
	
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("Execute OnnxDML Inference"),
        PassParameters,
        ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
        [LocalODIRHIExtensions = m_ODIRHIExtensions, model_name = m_model_name, PassParameters, InputBuffers, OutputBuffer](FRHICommandListImmediate& RHICmdList)
    {
        FODIRHIInferArguments Arguments;
		

        Arguments.ModelName = model_name;
        Arguments.OutputBuffer = OutputBuffer->GetRHI();

        for (auto it = InputBuffers.begin(); it != InputBuffers.end(); it++){
            Arguments.InputBuffers[it->first] = it->second->GetRHI();
			RHICmdList.TransitionResource(ERHIAccess::UAVMask, it->second);
        }

		FD3D12DynamicRHI::TransitionResource(InCommandListHandle, Resource, D3D12_RESOURCE_STATE_TBD, DesiredState, Subresource, FD3D12DynamicRHI::ETransitionMode::Validate);


        RHICmdList.EnqueueLambda(
            [LocalODIRHIExtensions, Arguments](FRHICommandListImmediate& Cmd) mutable
        {
            Arguments.GPUNode = Cmd.GetGPUMask().ToIndex();
            Arguments.GPUVisibility = Cmd.GetGPUMask().GetNative();

            // LocalODIModuleRef->BindResourceAndExecuteInference(Cmd, DLSSArguments);

			LocalODIRHIExtensions->BindResources(Arguments.ModelName);
			LocalODIRHIExtensions->ExecuteInference(Cmd, Arguments);
        });
    });
}
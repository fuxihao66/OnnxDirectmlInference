
Network::Network(const std::wstring& onnx_file_path, const std::string& model_name){
    FODIModule* LocalODIModuleRef = &FModuleManager::LoadModuleChecked<FODIModule>("ODI"); // load module
	m_ODIRHIExtensions = LocalODIModuleRef->GetODIRHIRef();

    m_onnx_file_path = onnx_file_path;

    if (model_name == "")
        m_model_name = m_onnx_file_path;
    else
        m_model_name = model_name;
}



bool Network::CreateModelAndUploadData(FRDGBuilder& GraphBuilder){
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("Create Model for %s", TEXT(m_model_name)),
        PassParameters,
        ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
        [m_ODIRHIExtensions, PassParameters, m_onnx_file_path, m_model_name](FRHICommandListImmediate& RHICmdList)
    {

        RHICmdList.EnqueueLambda(
            [m_ODIRHIExtensions, DLSSArguments, DLSSState](FRHICommandListImmediate& Cmd) mutable
        {

            // LocalODIModuleRef->CreateAndInitializeModel(Cmd, DLSSArguments);
            m_ODIRHIExtensions->ParseAndUploadModelData(Cmd, onnx_file_path, m_model_name);
            m_ODIRHIExtensions->InitializeNewModel(Cmd, m_model_name);
            // TODO: synchronize
        });
    });

}

bool Network::ExecuteInference(FRDGBuilder& GraphBuilder, std::map<std::string, FRDGBufferRef> InputBuffers, FRDGBufferRef OutputBuffer){
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("Execute OnnxDML Inference"),
        PassParameters,
        ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
        [m_ODIRHIExtensions, m_model_name, PassParameters, InputBuffers, OutputBuffer](FRHICommandListImmediate& RHICmdList)
    {
        FODIRHIInferArguments Arguments;
        
        // output images
        check(PassParameters->SceneColorOutput);
        PassParameters->SceneColorOutput->MarkResourceAsUsed();

        Arguments.ModelName = m_model_name;
        
        Arguments.Output = PassParameters->SceneColorOutput->GetRHI();

        for (auto it = InputBuffers.begin(); it != InputBuffers.end(); it++){
            Arguments.Inputs[it->first] = it->second->GetRHI();
        }

        RHICmdList.TransitionResource(ERHIAccess::UAVMask, Arguments.OutputColor);
        RHICmdList.TransitionResource(ERHIAccess::UAVMask, Arguments.OutputColor);
        RHICmdList.TransitionResource(ERHIAccess::UAVMask, Arguments.OutputColor);

        RHICmdList.EnqueueLambda(
            [m_ODIRHIExtensions, Arguments](FRHICommandListImmediate& Cmd) mutable
        {
            Arguments.GPUNode = Cmd.GetGPUMask().ToIndex();
            Arguments.GPUVisibility = Cmd.GetGPUMask().GetNative();

            // LocalODIModuleRef->BindResourceAndExecuteInference(Cmd, DLSSArguments);

            m_ODIRHIExtensions->BindResources(Arguments.ModelName);
            m_ODIRHIExtensions->ExecuteInference(Cmd, Arguments);
        });
    });
}
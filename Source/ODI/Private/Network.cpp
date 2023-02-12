
bool Network::CreateModelAndUploadData(GraphBuilder, onnx_file_path, modelName){
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("DLSS %s%s %dx%d -> %dx%d",
            PassName,
            Sharpness != 0.0f ? TEXT(" Sharpen") : TEXT(""),
            SrcRect.Width(), SrcRect.Height(),
            DestRect.Width(), DestRect.Height()),
        PassParameters,
        ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
        [LocalNGXRHIExtensions, PassParameters, Inputs, bCameraCut, JitterOffset, DeltaWorldTime, PreExposure, Sharpness, NGXDLAAPreset, NGXDLSSPreset, NGXPerfQuality, DLSSState, bUseAutoExposure, bReleaseMemoryOnDelete](FRHICommandListImmediate& RHICmdList)
    {
        FRHIDLSSArguments DLSSArguments;
        
        // output images
        check(PassParameters->SceneColorOutput);
        PassParameters->SceneColorOutput->MarkResourceAsUsed();
        DLSSArguments.OutputColor = PassParameters->SceneColorOutput->GetRHI();

        RHICmdList.TransitionResource(ERHIAccess::UAVMask, DLSSArguments.OutputColor);
        RHICmdList.EnqueueLambda(
            [LocalNGXRHIExtensions, DLSSArguments, DLSSState](FRHICommandListImmediate& Cmd) mutable
        {

            const uint32 FeatureCreationNode = CVarNGXDLSSFeatureCreationNode.GetValueOnRenderThread();
            const uint32 FeatureVisibilityMask = CVarNGXDLSSFeatureVisibilityMask.GetValueOnRenderThread();

            DLSSArguments.GPUNode = FeatureCreationNode == -1 ? Cmd.GetGPUMask().ToIndex() : FMath::Clamp(FeatureCreationNode, 0u, GNumExplicitGPUsForRendering - 1);
            DLSSArguments.GPUVisibility = FeatureVisibilityMask == -1 ? Cmd.GetGPUMask().GetNative() : (Cmd.GetGPUMask().All().GetNative() & FeatureVisibilityMask) ;

            LocalODIRHIExtensions->ParseAndUploadModelData(Cmd, DLSSArguments, DLSSState);
            LocalODIRHIExtensions->ParseAndUploadModelData(InitializeNewModel);
        });
    });

    // TODO: force sync 
}

bool Network::ExecuteInfer(GraphBuilder, bufferMap){
    GraphBuilder.AddPass(
        RDG_EVENT_NAME("DLSS %s%s %dx%d -> %dx%d",
            PassName,
            Sharpness != 0.0f ? TEXT(" Sharpen") : TEXT(""),
            SrcRect.Width(), SrcRect.Height(),
            DestRect.Width(), DestRect.Height()),
        PassParameters,
        ERDGPassFlags::Compute | ERDGPassFlags::Raster | ERDGPassFlags::SkipRenderPass,
        [LocalNGXRHIExtensions, PassParameters, Inputs, bCameraCut, JitterOffset, DeltaWorldTime, PreExposure, Sharpness, NGXDLAAPreset, NGXDLSSPreset, NGXPerfQuality, DLSSState, bUseAutoExposure, bReleaseMemoryOnDelete](FRHICommandListImmediate& RHICmdList)
    {
        FRHIDLSSArguments DLSSArguments;
        
        // output images
        check(PassParameters->SceneColorOutput);
        PassParameters->SceneColorOutput->MarkResourceAsUsed();
        DLSSArguments.OutputColor = PassParameters->SceneColorOutput->GetRHI();

        RHICmdList.TransitionResource(ERHIAccess::UAVMask, DLSSArguments.OutputColor);
        RHICmdList.EnqueueLambda(
            [LocalNGXRHIExtensions, DLSSArguments, DLSSState](FRHICommandListImmediate& Cmd) mutable
        {
            LocalODIRHIExtensions->BindResources();

            const uint32 FeatureCreationNode = CVarNGXDLSSFeatureCreationNode.GetValueOnRenderThread();
            const uint32 FeatureVisibilityMask = CVarNGXDLSSFeatureVisibilityMask.GetValueOnRenderThread();

            DLSSArguments.GPUNode = FeatureCreationNode == -1 ? Cmd.GetGPUMask().ToIndex() : FMath::Clamp(FeatureCreationNode, 0u, GNumExplicitGPUsForRendering - 1);
            DLSSArguments.GPUVisibility = FeatureVisibilityMask == -1 ? Cmd.GetGPUMask().GetNative() : (Cmd.GetGPUMask().All().GetNative() & FeatureVisibilityMask) ;

            LocalODIRHIExtensions->ExecuteInfer(Cmd, DLSSArguments, DLSSState);
        });
    });
}
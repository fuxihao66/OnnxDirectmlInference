#pragma once
//#include "ScreenPass.h"
//#include "PostProcess/PostProcessing.h"
//#include "PostProcess/PostProcessInput.h"
#include "CoreMinimal.h"
//#include "PostProcess/PostProcessVisualizeBuffer.h"

FRDGBufferRef TEXTUREBUFFERCOPY_API DispatchTexture2Tensor(FRDGBuilder& GraphBuilder,// const FViewInfo& View,
	const uint32 TextureWidth, const uint32 TextureHeight,
	const uint32 BufferWidth, const uint32 BufferHeight,
	FRDGTextureRef InputTexture);

FRDGTextureRef TEXTUREBUFFERCOPY_API DispatchTensor2Texture(FRDGBuilder& GraphBuilder,// const FViewInfo& View,
	const uint32 TextureWidth, const uint32 TextureHeight,
	const uint32 BufferWidth, const uint32 BufferHeight,
	FRDGBufferRef InputTensor, const FRDGTextureDesc& OutputTextureDesc);
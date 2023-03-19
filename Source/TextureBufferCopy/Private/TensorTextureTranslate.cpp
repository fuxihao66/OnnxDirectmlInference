#include "TensorTextureTranslate.h"

//#include "SceneTextureParameters.h"
#include "PixelShaderUtils.h"
#include "RawIndexBuffer.h"
#include "SceneUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "Shader.h"
#include "MeshMaterialShader.h"
//#include "PostProcess/PostProcessing.h"


class FTensor2TextureCS : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	DECLARE_GLOBAL_SHADER(FTensor2TextureCS);
	SHADER_USE_PARAMETER_STRUCT(FTensor2TextureCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<half>, gInputTensor)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, gOutputTexture)
		SHADER_PARAMETER(uint32, TextureWidth)
		SHADER_PARAMETER(uint32, TextureHeight)
		SHADER_PARAMETER(uint32, BufferWidth)
		SHADER_PARAMETER(uint32, BufferHeight)


	END_SHADER_PARAMETER_STRUCT()
};

class FTexture2TensorCS : public FGlobalShader
{
public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	DECLARE_GLOBAL_SHADER(FTexture2TensorCS);
	SHADER_USE_PARAMETER_STRUCT(FTexture2TensorCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, gInputTexture)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<half>, gOutputTensor)
		SHADER_PARAMETER(uint32, TextureWidth)
		SHADER_PARAMETER(uint32, TextureHeight)
		SHADER_PARAMETER(uint32, BufferWidth)
		SHADER_PARAMETER(uint32, BufferHeight)
		SHADER_PARAMETER_SAMPLER(SamplerState, gSampler)
	END_SHADER_PARAMETER_STRUCT()
};


IMPLEMENT_GLOBAL_SHADER(FTensor2TextureCS, "/ShadersUtility/Shaders/tensor2Texture.usf", "tensorToImage", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FTexture2TensorCS, "/ShadersUtility/Shaders/texture2Tensor.usf", "imageToTensor", SF_Compute);


FRDGBufferRef DispatchTexture2Tensor(FRDGBuilder& GraphBuilder,// const FViewInfo& View,
	const uint32 TextureWidth, const uint32 TextureHeight, 
	const uint32 BufferWidth, const uint32 BufferHeight,
	FRDGTextureRef InputTexture) {

	
	FRDGBufferRef OutputTensor;
	{

		FRDGBufferDesc BufferDesc = FRDGBufferDesc::CreateBufferDesc(sizeof(uint16_t), BufferWidth * BufferWidth * 3); // half
		OutputTensor = GraphBuilder.CreateBuffer(BufferDesc, TEXT("OutputTensor"));
	}
	{// pass one

		FTexture2TensorCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FTexture2TensorCS::FParameters>();

		PassParameters->gInputTexture = InputTexture;
		PassParameters->TextureWidth = TextureWidth;
		PassParameters->TextureHeight = TextureHeight;
		PassParameters->BufferWidth = BufferWidth;
		PassParameters->BufferHeight = BufferHeight;
		// PassParameters->View = View.ViewUniformBuffer;
		// PassParameters->InputInfo = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(InputTexture->Desc.Extent, View.ViewRect));

		PassParameters->gOutputTensor = GraphBuilder.CreateUAV(OutputTensor, PF_R16F);

		FRHISamplerState* Sampler = TStaticSamplerState<SF_Bilinear>::GetRHI();
		PassParameters->gSampler = Sampler;
		//TShaderMapRef<FTexture2TensorCS> ComputeShader(View.ShaderMap);
		TShaderMapRef<FTexture2TensorCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		//ComputeShader.GetComputeShader()->SetShaderName(FString(TEXT("texture2Tensor")));
		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("TextureToTensor"),
			ComputeShader,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(FIntPoint(BufferWidth, BufferHeight), 32));
	}

	return OutputTensor;
}

FRDGTextureRef DispatchTensor2Texture(FRDGBuilder& GraphBuilder,// const FViewInfo& View,
	const uint32 TextureWidth, const uint32 TextureHeight, 
	const uint32 BufferWidth, const uint32 BufferHeight,
	FRDGBufferRef InputTensor, const FRDGTextureDesc& OutputTextureDesc) {

	FRDGTextureRef OutputTexture;
	{
		auto TextureDesc = OutputTextureDesc;
		TextureDesc.Flags = TexCreate_ShaderResource | TexCreate_UAV; // allow uav
		OutputTexture = GraphBuilder.CreateTexture(TextureDesc, TEXT("OutputTexture"), ERDGTextureFlags::MultiFrame);
	}
	{// pass one

		FTensor2TextureCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FTensor2TextureCS::FParameters>();

		PassParameters->gInputTensor = GraphBuilder.CreateSRV(InputTensor, PF_R16F);

		// PassParameters->View = View.ViewUniformBuffer;
		// PassParameters->InputInfo = GetScreenPassTextureViewportParameters(FScreenPassTextureViewport(SceneDepthTexture->Desc.Extent, View.ViewRect));
		PassParameters->TextureWidth = TextureWidth;
		PassParameters->TextureHeight = TextureHeight;
		PassParameters->BufferWidth = BufferWidth;
		PassParameters->BufferHeight = BufferHeight;
		PassParameters->gOutputTexture = GraphBuilder.CreateUAV(OutputTexture);

		TShaderMapRef<FTensor2TextureCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
		//ComputeShader.GetComputeShader()->SetShaderName(FString(TEXT("tensor2texture")));

		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("TensorToTexture"),
			ComputeShader,
			PassParameters,
			FComputeShaderUtils::GetGroupCount(FIntPoint(TextureWidth, TextureHeight), 32));
	}

	return OutputTexture;
}


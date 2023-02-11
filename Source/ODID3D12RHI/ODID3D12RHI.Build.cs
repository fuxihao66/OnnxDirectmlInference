
using UnrealBuildTool;
using System.IO;
public class NGXD3D12RHI : ModuleRules
{
	public NGXD3D12RHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);

		PrivateIncludePaths.AddRange(
		new string[] {
				Path.Combine(EngineDirectory,"Source/Runtime/D3D12RHI/Private"),
				Path.Combine(EngineDirectory,"Source/Runtime/D3D12RHI/Private/Windows"),
		}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
					"Core",
					"Engine",
					"RenderCore",
					"RHI",
					"D3D12RHI",
			}
			);

		if (ReadOnlyBuildVersion.Current.MajorVersion == 5)
		{
			PrivateDependencyModuleNames.Add("RHICore");
		}
		// those come from the D3D12RHI
		AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
		// AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
		// AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
		// AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelMetricsDiscovery");
		// AddEngineThirdPartyPrivateStaticDependencies(Target, "IntelExtensionsFramework");
	}
}

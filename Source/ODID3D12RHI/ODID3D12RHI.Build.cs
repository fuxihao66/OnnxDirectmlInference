
using UnrealBuildTool;
using System.IO;
public class ODID3D12RHI : ModuleRules
{
	public ODID3D12RHI(ReadOnlyTargetRules Target) : base(Target)
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
				"DirectML_1_9_1",
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
					"ODIRHI",
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

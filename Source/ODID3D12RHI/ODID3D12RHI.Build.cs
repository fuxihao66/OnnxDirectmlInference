
using UnrealBuildTool;
using System.IO;
public class ODID3D12RHI : ModuleRules
{
	public ODID3D12RHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableExceptions = true;
		bUseUnity = false;
		CppStandard = CppStandardVersion.Cpp17;
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
		PublicSystemLibraries.AddRange(new string[] { "shlwapi.lib", "runtimeobject.lib" });


		PrivateIncludePaths.AddRange(
		new string[] {
				Path.Combine(EngineDirectory,"Source/Runtime/D3D12RHI/Private"),
				Path.Combine(EngineDirectory,"Source/Runtime/D3D12RHI/Private/Windows"),
				Path.Combine(Target.WindowsPlatform.WindowsSdkDir,        
												"Include", 
												Target.WindowsPlatform.WindowsSdkVersion, 
												"cppwinrt")
		}
		);

		string LibDirPath = Path.Combine(ModuleDirectory, "OnnxParser");

		PublicAdditionalLibraries.Add(Path.Combine(LibDirPath, "OnnxParser.lib"));
		PublicDelayLoadDLLs.Add("OnnxParser.dll");
		string DLLFullPath = Path.Combine(LibDirPath, "OnnxParser.dll");
		RuntimeDependencies.Add(DLLFullPath);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"DirectML_1_9_1",
				"Projects"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
					"Core",
					"CoreUObject",
					"EngineSettings",
					"Engine",
					"RenderCore",
					"Renderer",
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


using UnrealBuildTool;
using System.IO;
public class OnnxParser : ModuleRules
{
	public OnnxParser(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnableExceptions = true;
		bUseUnity = false;
		CppStandard = CppStandardVersion.Cpp17;
		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
		PublicSystemLibraries.AddRange(new string[] {  });


		PrivateIncludePaths.AddRange(
		new string[] {
				
		}
		);

		// string LibDirPath = Path.Combine(ModuleDirectory, "OnnxParser");

		// PublicAdditionalLibraries.Add(Path.Combine(LibDirPath, "OnnxParser.lib"));
		// PublicDelayLoadDLLs.Add("OnnxParser.dll");
		// string DLLFullPath = Path.Combine(LibDirPath, "OnnxParser.dll");
		// RuntimeDependencies.Add(DLLFullPath);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				//"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Protobuf"
			}
			);
		PublicDefinitions.Add("GOOGLE_PROTOBUF_INTERNAL_DONATE_STEAL_INLINE=0");
	}
}

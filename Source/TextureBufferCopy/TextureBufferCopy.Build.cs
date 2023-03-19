

using UnrealBuildTool;
using System.IO;
public class TextureBufferCopy : ModuleRules
{
	public TextureBufferCopy(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


		PublicIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(ModuleDirectory, "..")
			}
		);
		PrivateIncludePaths.AddRange(
			new string[] {
			}
			);
		PublicDependencyModuleNames.AddRange
			(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine"
			}
		);

		PrivateDependencyModuleNames.AddRange
			(
			new string[] {
				"Core",
				"Engine",
				"Projects",
				"RenderCore",
				"RHI"
			}
		);

		
		if (ReadOnlyBuildVersion.Current.MajorVersion == 5)
		{
			PrivateDependencyModuleNames.Add("RHICore");
		}
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}



using UnrealBuildTool;
using System.IO;
public class ODIRHI : ModuleRules
{
	public ODIRHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		

		PublicIncludePaths.AddRange(
			new string[] {
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
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
			}
			);
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
			}
			);
	}
}

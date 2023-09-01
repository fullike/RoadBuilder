// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

using System.IO;
namespace UnrealBuildTool.Rules
{
	public class RoadBuilder : ModuleRules
	{
		public RoadBuilder(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
                    "CoreUObject",
					"DeveloperSettings",
                    "Engine",
					"GeometryAlgorithms",
					"GeometryCore",
                    "GeoReferencing",
                    "HTTP",
                    "MeshConversion",
					"MeshDescription",
					"PCG",
					"ProceduralMeshComponent",
					"StaticMeshDescription",
					"XmlParser",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			if (Target.bBuildEditor)
			{
				PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"DesktopPlatform",
						"Slate",
						"SlateCore",
						"UnrealEd",
					}
					);
			}

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}

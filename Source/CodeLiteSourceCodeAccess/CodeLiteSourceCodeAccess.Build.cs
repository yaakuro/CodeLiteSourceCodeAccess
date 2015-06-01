// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class CodeLiteSourceCodeAccess : ModuleRules
	{
		public CodeLiteSourceCodeAccess(TargetInfo Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"SourceCodeAccess",
					"DesktopPlatform",
				}
			);

			if (UEBuildConfiguration.bBuildEditor)
			{
				PrivateDependencyModuleNames.Add("HotReload");
			}
			if ( (Target.Platform == UnrealTargetPlatform.Linux) )
			{
				// TODO For now not needed.
			//	PublicIncludePaths.AddRange(new string[] { "/usr/include/dbus-1.0", "/usr/lib/x86_64-linux-gnu/dbus-1.0/include" });
			//	PublicAdditionalLibraries.Add("dbus-1");
			} 
			else if ( (Target.Platform == UnrealTargetPlatform.Mac) )
			{
			}
			else if ( (Target.Platform == UnrealTargetPlatform.Win64) || 
					  (Target.Platform == UnrealTargetPlatform.Win32) )
			{
			}
		}
	}
}

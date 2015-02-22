// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;
using System;

public class voxel1 : ModuleRules
{
	private void AddMyInclude(string path)
	{
		string ModulePath = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
		string IncDir = Path.GetFullPath(Path.Combine(ModulePath, "../../", path));
		PrivateIncludePaths.Add(IncDir);
	}

	private void AddMyLibrary(string path)
	{
		string ModulePath = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
		string LibPath = Path.GetFullPath(Path.Combine(ModulePath, "../../", path));
		PublicAdditionalLibraries.Add(LibPath);
	}

	public voxel1(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "RHI", "RenderCore", "ShaderCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");
		// if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
		// {
		//		if (UEBuildConfiguration.bCompileSteamOSS == true)
		//		{
		//			DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
		//		}
		// }

		LoadOpenVDB(Target);
	}

	public void LoadOpenVDB(TargetInfo Target) 
	{
		if (Target.Platform == UnrealTargetPlatform.Mac) 
		{
			AddMyInclude("Backend/include");
			AddMyLibrary("Backend/build/release/libbackend.a");
			AddMyLibrary("ThirdParty/libs/libHalf.dylib");
			AddMyLibrary("ThirdParty/libs/libtbb.dylib");
			AddMyLibrary("ThirdParty/libs/libopenvdb.dylib");
		}
	}
}

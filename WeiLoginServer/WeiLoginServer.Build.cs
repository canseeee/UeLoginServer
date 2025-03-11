// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WeiLoginServer : ModuleRules
{
	public WeiLoginServer(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePathModuleNames.Add("Launch");
		PrivateDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.Add("Projects");
		//独立程序
		PrivateDependencyModuleNames.Add("ApplicationCore");
		// to link with CoreUObject module:
		PrivateDependencyModuleNames.Add("CoreUObject");
		//请求和响应
		PrivateDependencyModuleNames.Add("HTTP");
		//服务器
		PrivateDependencyModuleNames.Add("HTTPServer");
		//Json
		PrivateDependencyModuleNames.Add("Json");
		PrivateDependencyModuleNames.Add("JsonUtilities");
		//MySQL

		PrivateDependencyModuleNames.Add("SimpleMySQL");
		

		// to enable LLM tracing:
		// GlobalDefinitions.Add("LLM_ENABLED_IN_CONFIG=1");
		// GlobalDefinitions.Add("UE_MEMORY_TAGS_TRACE_ENABLED=1");

	}
}

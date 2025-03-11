// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeiLoginServer.h"
#include "Log/LogWeiLoginServer.h"
#include "RequiredProgramMainCPPInclude.h"
#include "LoginServerObject/HttpServerObject.h"



IMPLEMENT_APPLICATION(WeiLoginServer, "WeiLoginServer");

INT32_MAIN_INT32_ARGC_TCHAR_ARGV()
{
	FTaskTagScope Scope(ETaskTag::EGameThread);  //这行代码创建了一个FTaskTagScope对象，该对象用于标记接下来的代码块是在游戏线程上执行的。
	/*
	ON_SCOPE_EXIT是一个宏，它允许你指定一段代码，这段代码将在当前作用域结束时自动执行。这里它被用来在程序退出前执行一系列清理操作，
	如更新内存统计信息、请求引擎退出、调用应用预退出和退出函数，以及卸载模块。
	*/
	ON_SCOPE_EXIT
	{ 
		LLM(FLowLevelMemTracker::Get().UpdateStatsPerFrame());
		RequestEngineExit(TEXT("Exiting"));
		FEngineLoop::AppPreExit();
		FModuleManager::Get().UnloadModulesAtShutdown();
		UE_LOG(LogWeiLoginServer, Display, TEXT("LoginServer AppExit"));
		FEngineLoop::AppExit();
	};
	//这行代码调用GEngineLoop（全局引擎循环对象）的PreInit方法来进行引擎的预初始化。如果PreInit返回非零值，则立即返回该值，表示程序初始化失败。
	if (int32 Ret = GEngineLoop.PreInit(ArgC, ArgV))
	{
		return Ret;
	}
	UE_LOG(LogWeiLoginServer, Display, TEXT("Hello World"));
	UE_LOG(LogWeiLoginServer, Display, TEXT("LoginServer Init"));

	//1.初始化配置
	FWeiMySqlConfigManage* MySqlConfigManage =FWeiMySqlConfigManage::Get();
	MySqlConfigManage->Init();

	//2.创建服务器的实例
	UHttpServerObject* LoginServer=nullptr;
	LoginServer=UHttpServerObject::GetWeiLoginServerObject();

	//3.启动服务器
	LoginServer->InitWeiLoginServerObject();
	FHttpServerModule& ServerModule=FHttpServerModule::Get();
	//4.服务器开始收发数据
	

	//记录程序开始时间
	double LastTime=FPlatformTime::Seconds();
	while (!IsEngineExitRequested())
	{
		//让程序休眠一下 避免一直工作，上线的程序必须去掉
		FPlatformProcess::Sleep(0.05f);
		//拿到当前帧的时间点
		double Now= FPlatformTime::Seconds();
		//计算两帧差值
		float DeltaSenconds = Now - LastTime;
		LastTime = Now;

		ServerModule.Tick(DeltaSenconds);
	}


	//5.程序结束销毁
	LoginServer->DestoryWeiLoginServerObject();
	LoginServer=nullptr;
	MySqlConfigManage->Destroy();
	MySqlConfigManage=nullptr;
	UE_LOG(LogWeiLoginServer, Display, TEXT("LoginServer Destory"));
	


	
	
	return 0;
}
// to run automation tests in BlankProgram:
// * set `bForceCompileDevelopmentAutomationTests` to `true` in `BlankProgramTarget` constructor in "BlankProgram.Target.cs"
// add `FAutomationTestFramework::Get().StartTestByName(TEXT("The name of the test class passed to IMPLEMENT_SIMPLE_AUTOMATION_TEST or TEST_CASE_NAMED"), 0);` right here

// to link with "CoreUObject" module:
// * uncomment `PrivateDependencyModuleNames.Add("CoreUObject");` in `BlankProgram` constructor in "BlankProgram.Build.cs"
// * set `bCompileAgainstCoreUObject` to `true` in `BlankProgramTarget` constructor in "BlankProgram.Target.cs"

// to enable tracing:
// * uncomment `AppendStringToPublicDefinition("UE_TRACE_ENABLED", "1");` in `BlankProgram` constructor in "BlankProgram.Build.cs"
// * uncomment `GlobalDefinitions.Add("UE_TRACE_ENABLED=1");` in `BlankProgramTarget` constructor in "BlankProgram.Target.cs"
// you may need to enable compilation of a particular trace channel, e.g. for "task" traces:
// * add `GlobalDefinitions.Add("UE_TASK_TRACE_ENABLED=1");` in `BlankProgramTarget` constructor in "BlankProgram.Target.cs"
// you'll still need to enable this trace channel on cmd-line like `-trace=task,default`

// to enable LLM tracing, uncomment the following in `BlankProgram` constructor in "BlankProgram.Build.cs":
// GlobalDefinitions.Add("LLM_ENABLED_IN_CONFIG=1");
// GlobalDefinitions.Add("UE_MEMORY_TAGS_TRACE_ENABLED=1");
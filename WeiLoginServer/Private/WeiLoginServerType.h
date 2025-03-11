#pragma once

#include "CoreMinimal.h"
#include "Core/SimpleMysqlLinkType.h"

struct FWeiSqlConfig
{
	FWeiSqlConfig()
		:User("root")
		,Host("127.0.0.1")
		,Pawd("123456")
		,Post(3306)
		,DB("hello")
	{
		ClientFlags.Add(ESimpleClientFlags::Client_Multi_Statements);
		ClientFlags.Add(ESimpleClientFlags::Client_Multi_Results);
	}
	//数据库账号
	FString User;
	//数据库IP
	FString Host;
	//密码
	FString Pawd;
	//库名字
	FString DB;
	//端口
	int32 Post;

	TArray<ESimpleClientFlags> ClientFlags;
};
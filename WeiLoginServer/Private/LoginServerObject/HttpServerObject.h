#pragma once

#include "CoreMinimal.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpResultCallback.h"
#include "HttpServerRequest.h"
#include <Dom/JsonObject.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializable.h>
#include "SimpleMySQLibrary.h"
#include "MySqlManage/WeiMySqlConfigManage.h"
#include "Core/SimpleMysqlLinkType.h"
#include "HttpServerObject.generated.h"


class USimpleMysqlObject;
//单例模式
UCLASS()
class UHttpServerObject:public UObject
{
	GENERATED_BODY()

public:
	UHttpServerObject();

public:
	static UHttpServerObject* GetWeiLoginServerObject();

	static void DestoryWeiLoginServerObject();

	void InitWeiLoginServerObject();

protected:
	// 数据库初始化 创建连接   无createMysqlObject函数，无法实现
	void InitMySqlConnect();

	// 封装
	bool PostSql(const FString& InSQL);
	bool GetSql(const FString& InSQL, TArray<FSimpleMysqlResult>& Results);
protected:
	//using FHttpRequestHandler = TDelegate<bool(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)>;

	//收到消息后会调用的函数
	bool ProcessHttpRequest(const FHttpServerRequest& Request,const FHttpResultCallback& OnComplete);
	
	//用于处理Json字符串
	FString HandleProcessHttpJson(FString InJsonStr);

	//注册请求
	FString HandleProcessRegisterJson(FString InJsonStr);
	//登录请求
	FString HandleProcessLoginJson(FString InJsonStr);

private:
	//实例
	static UHttpServerObject* Instance;
	//管理服务器的单例
	FHttpServerModule* Server=nullptr;
	//用来绑定监听端口
	TSharedPtr<IHttpRouter> Router;
	//用来传递绑定的
	FHttpRequestHandler RequestHandler;
	//对于Router的hanlde 用来管理生命周期
	FHttpRouteHandle* RouterHandler;
private:
	// MySql的读写，读写发往不同的数据库
	USimpleMysqlObject* MysqlObjectRead=nullptr;
	USimpleMysqlObject* MysqlObjectWrite=nullptr;
};
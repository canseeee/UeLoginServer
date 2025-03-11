#pragma once

#include "WeiLoginServerType.h"

class FWeiMySqlConfigManage
{
public:
	// 单例 获取自己
	static FWeiMySqlConfigManage* Get();
	// 销毁自己
	static void Destroy();
	//初始化
	void Init(const FString& InPath=FPaths::ProjectDir()/TEXT("MysqlConfig.ini"));

	// 获取配置信息
	const FWeiSqlConfig& GetInfo() const;
private:
	// 全局唯一
	static FWeiMySqlConfigManage* WeiMySqlConfigManage;

	// Mysql结构体类型，专门描述结构体的
	// 配置信息，如果本地没有就创建缺省，如果本地有就去读取本地到这个对象里面来，方便后续使用
	struct FWeiSqlConfig MySqlConfigInfo;
};
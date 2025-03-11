#include "WeiMySqlConfigManage.h"

FWeiMySqlConfigManage* FWeiMySqlConfigManage::WeiMySqlConfigManage=nullptr;
FWeiMySqlConfigManage* FWeiMySqlConfigManage::Get()
{
	if (!WeiMySqlConfigManage)
	{
		WeiMySqlConfigManage = new FWeiMySqlConfigManage();
	}
	return WeiMySqlConfigManage;
}

void FWeiMySqlConfigManage::Destroy()
{
	if (WeiMySqlConfigManage)
	{
		delete WeiMySqlConfigManage;
		WeiMySqlConfigManage = nullptr;
	}
}

void FWeiMySqlConfigManage::Init(const FString& InPath)
{
	// 字符串每行内容
	TArray<FString> Content;

	// 从本地磁盘读取文件，读到字符串数组
	FFileHelper::LoadFileToStringArray(Content,*InPath);

	// 如果能找到数据
	if (Content.Num() > 0)
	{
		TMap<FString,FString> InConfigInfo;
		for (auto& Tmp:Content)
		{
			
			// 说明它是第一行【MySqlConfigManage】
			if (Tmp.Contains("[")&& Tmp.Contains("]"))
			{
				Tmp.RemoveFromStart("[");
				Tmp.RemoveFromEnd("]");

				InConfigInfo.Add("ConfigHead",Tmp);
			}
			else
			{
				FString Type,Data;
				Tmp.Split(TEXT("="),&Type,&Data);
				InConfigInfo.Add(Type,Data);
			}
		}
		MySqlConfigInfo.User=InConfigInfo["User"];
		MySqlConfigInfo.Host=InConfigInfo["Host"];
		MySqlConfigInfo.Pawd=InConfigInfo["Pawd"];
		MySqlConfigInfo.DB=InConfigInfo["DB"];
		// 
		MySqlConfigInfo.Post = 3306; //FCString::Atoi(*(InConfigInfo["Post"]));
	
	}
	else
	{
		// 如果没有找到数据，就需要创建这个文件，方便下次使用
		// 只在项目第一次启动时创建
		Content.Add(TEXT("[WeiMySqlConfigManage]"));
		Content.Add(FString::Printf(TEXT("User=%s"), *MySqlConfigInfo.User));
		Content.Add(FString::Printf(TEXT("Host=%s"), *MySqlConfigInfo.Host));
		Content.Add(FString::Printf(TEXT("Pawd=%s"), *MySqlConfigInfo.Pawd));
		Content.Add(FString::Printf(TEXT("DB=%s"), *MySqlConfigInfo.DB));
		Content.Add(FString::Printf(TEXT("Port=%i"), MySqlConfigInfo.Post));

		// 存到本地磁盘
		FFileHelper::SaveStringArrayToFile(Content,*InPath);
	}
}

const FWeiSqlConfig& FWeiMySqlConfigManage::GetInfo() const
{
	return MySqlConfigInfo;
}

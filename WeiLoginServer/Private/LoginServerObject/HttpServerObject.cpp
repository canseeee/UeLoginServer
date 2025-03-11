#include "HttpServerObject.h"
#include "../Log/LogWeiLoginServer.h"
#include "HttpPath.h"
#include "IHttpRouter.h"
#include "MySqlManage/WeiMySqlConfigManage.h"

UHttpServerObject* UHttpServerObject::Instance=nullptr;

UHttpServerObject::UHttpServerObject()
{
	
}

UHttpServerObject* UHttpServerObject::GetWeiLoginServerObject()
{
	if (Instance==nullptr)
	{
		Instance=NewObject<UHttpServerObject>();
		//一定要添加到根上 不然会被垃圾回收
		Instance->AddToRoot();
	}
	return Instance;
}

void UHttpServerObject::DestoryWeiLoginServerObject()
{
	if (Instance!=nullptr)
	{
		Instance->RemoveFromRoot();
		Instance=nullptr;
	}
}

void UHttpServerObject::InitWeiLoginServerObject()
{
	UE_LOG(LogWeiLoginServer, Display, TEXT("Init WeiLoginServerObject"));

	//1.初始化数据库
	InitMySqlConnect();
	//2.初始化webServer
	Server=&FHttpServerModule::Get();

	//指定访问路径
	FHttpPath HttpPath(TEXT("/Login"));

	//指定端口
	Router=Server->GetHttpRouter(8000);

	//Router->BindRoute会将端口绑定访问路径与发送的json模式和-一个委托-，我们将这个委托绑定到我们自己的函数上
	RequestHandler.BindUObject(this,&UHttpServerObject::ProcessHttpRequest);
	Router->BindRoute(HttpPath,EHttpServerRequestVerbs::VERB_POST|EHttpServerRequestVerbs::VERB_GET,RequestHandler);
	//3.启动监听
	Server->StartAllListeners();
}

void UHttpServerObject::InitMySqlConnect()
{
	// sql的配置管理类必须原先初始化,要不然config会报空指针
	MysqlObjectRead=USimpleMySQLLibrary::CreateMysqlObject(nullptr,FWeiMySqlConfigManage::Get()->GetInfo().User,
		FWeiMySqlConfigManage::Get()->GetInfo().Host,
		FWeiMySqlConfigManage::Get()->GetInfo().Pawd,
		FWeiMySqlConfigManage::Get()->GetInfo().DB,
		FWeiMySqlConfigManage::Get()->GetInfo().Post,
		FWeiMySqlConfigManage::Get()->GetInfo().ClientFlags);

	MysqlObjectWrite=USimpleMySQLLibrary::CreateMysqlObject(nullptr,FWeiMySqlConfigManage::Get()->GetInfo().User,
	FWeiMySqlConfigManage::Get()->GetInfo().Host,
	FWeiMySqlConfigManage::Get()->GetInfo().Pawd,
	FWeiMySqlConfigManage::Get()->GetInfo().DB,
	FWeiMySqlConfigManage::Get()->GetInfo().Post,
	FWeiMySqlConfigManage::Get()->GetInfo().ClientFlags);

	FString CreateUserDataTable=TEXT("create table if not EXISTS `users` (`id` int UNSIGNED auto_increment,`user_name` VARCHAR(100) NOT NULL,`user_pass` VARCHAR(100) NOT NULL,`phone` VARCHAR(100) NOT NULL,`email` VARCHAR(100) NOT NULL,`money` VARCHAR(100) NOT NULL,PRIMARY KEY(`id`)) ENGINE=INNODB DEFAULT CHARSET = utf8mb4;");

	if (PostSql(CreateUserDataTable))
	{
		UE_LOG(LogWeiLoginServer, Display, TEXT("CreateUserDataTable Succeed"));
	}
	else
	{
		UE_LOG(LogWeiLoginServer, Error, TEXT("CreateUserDataTable Error"));
	}
}

bool UHttpServerObject::PostSql(const FString& InSQL)
{
	if (!InSQL.IsEmpty())
	{
		if (MysqlObjectWrite)
		{
			// 报错信息
			FString ErrMsg;
			//呼叫数据库执行Sql语句
			USimpleMySQLLibrary::QueryLink(MysqlObjectWrite,InSQL,ErrMsg);
			if (ErrMsg.IsEmpty())
			{
				return true;
			}
			else
			{
				//日志分离
				UE_LOG(LogWeiLoginServer,Error,TEXT("UHttpServerObject::PostSql:%s"),*ErrMsg);
			}
		}
	}
	return false;
}

bool UHttpServerObject::GetSql(const FString& InSQL,TArray<FSimpleMysqlResult>& Results)
{
	if (!InSQL.IsEmpty())
	{
		if (MysqlObjectRead)
		{
			FSimpleMysqlDebugResult Debug;
			//是否打印日志
			Debug.bPrintToLog=true;
			FString ErrMsg;
			USimpleMySQLLibrary::QueryLinkResult(MysqlObjectRead,
				InSQL,
				Results,
				ErrMsg,
				EMysqlQuerySaveType::STORE_RESULT,
				Debug);
			if (ErrMsg.IsEmpty())
			{
				return true;
			}
			else
			{
				UE_LOG(LogWeiLoginServer,Error,TEXT("UHttpServerObject::GetSql:%s"),*ErrMsg);
			}
		}
	}
	return false;
}

//当端口接收到数据时说明有客户端发消息过来了，会调用这个函数
bool UHttpServerObject::ProcessHttpRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	UE_LOG(LogWeiLoginServer, Display, TEXT("UHttpServerObject::ProcessHttpRequest"));

	UHttpServerObject* WeiServer = UHttpServerObject::GetWeiLoginServerObject();

	//get读取数据 没用到
	for (auto param : Request.QueryParams) {
		UE_LOG(LogWeiLoginServer, Display, TEXT("%s->%s"), *param.Key, *param.Value);
	}

	//post读取content数据  先显式转换成const char*，再隐式转成UTF8的类，再转换成FString
	FUTF8ToTCHAR Convert(reinterpret_cast<const char*>(Request.Body.GetData()), Request.Body.Num());
	FString InJsonStr(Convert);
	FString OutJsonStr = TEXT("");
	UE_LOG(LogWeiLoginServer, Display, TEXT("UHttpServerObject::ProcessHttpRequest:InJsonStr=[%s]"), *InJsonStr);
	if (!(InJsonStr.IsEmpty()))
	{
		OutJsonStr=WeiServer->HandleProcessHttpJson(InJsonStr);
	}
	else {
		//未找到匹配的服务器执行模式
		FString Code = TEXT("404");
		FString Message = TEXT("JsonContent Get Error");
		OutJsonStr = FString::Printf(TEXT("{\"Code\":\"%s\",\"Message\":\"%s\"}"), *Code, *Message);
		UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::ProcessHttpRequest:JsonContent Get Error"))
	}



	//回复
	OnComplete(FHttpServerResponse::Create(OutJsonStr,TEXT("application/json")));
	return true;
}

FString UHttpServerObject::HandleProcessHttpJson(FString InJsonStr)
{
	//创建一个Json对象
	TSharedPtr<FJsonObject> ResponseObj;
	//创建一个读取流
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InJsonStr);
	//反序列化
	FJsonSerializer::Deserialize(Reader, ResponseObj);
	
	
	FString InterViewType = *ResponseObj->GetStringField("InterviewType");
	
	//返回的字符串
	FString ResponseBody = TEXT("");

	//查看是对应于哪种类型的发送报文
	if (InterViewType.Equals(TEXT("RegisterIn"))) {
		ResponseBody = HandleProcessRegisterJson(InJsonStr);
	}
	else if (InterViewType.Equals(TEXT("SignIn")))
	{
		ResponseBody = HandleProcessLoginJson(InJsonStr);
	}
	else {
		//未找到匹配的服务器执行模式
		FString Code = TEXT("404");
		FString Message = TEXT("Http Get Error");
		ResponseBody = FString::Printf(TEXT("{\"Code\":\"%s\",\"Message\":\"%s\"}"), *Code, *Message);
		UE_LOG(LogWeiLoginServer, Display, TEXT("UHttpServerObject::HandleProcessHttpJson:JsonContent Get Error"))
	}
	
	return ResponseBody;
}

FString UHttpServerObject::HandleProcessRegisterJson(FString InJsonStr)
{
	UE_LOG(LogWeiLoginServer, Display, TEXT("UHttpServerObject::HandleProcessRegisterJson"));

	TSharedPtr<FJsonObject> HttpContentObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InJsonStr);
	FJsonSerializer::Deserialize(Reader, HttpContentObj);

	FString UserName = *HttpContentObj->GetStringField("UserName");
	FString Password = *HttpContentObj->GetStringField("Password");
	FString PhoneNum=*HttpContentObj->GetStringField("PhoneNum");
	FString Email = *HttpContentObj->GetStringField("Email");
	FString Guid = *HttpContentObj->GetStringField("Guid");

	/*
	 * 正则：
	 * 名称判断
	 * 密码判定
	 * 手机号判断
	 * 邮箱判定
	 */
	// 1.查询是否重名，是否验证通过可以使用
	bool bVerifyUserName = true;
	FString SQL = TEXT("SELECT user_name FROM users ;");
	TArray<FSimpleMysqlResult> UserNameResults;
	if (GetSql(SQL, UserNameResults))
	{
		for (auto& Tmp: UserNameResults)
		{
			if (UserName==*Tmp.Rows.Find("user_name"))
			{
				UE_LOG(LogWeiLoginServer,Warning,TEXT("UHttpServerObject::HandleProcessRegisterJson: repetitive name : [%s]"),*UserName);
				bVerifyUserName = false;
				break;
			}

			
		}
	}
	else
	{
		bVerifyUserName=false;
	}

	// 2.真正进行注册
	//避免sql注入
	bool bRegister=false;
	if (bVerifyUserName)
	{

		// 生成 SQL 插入语句
		FString ToSQL = FString::Printf(
			TEXT("INSERT INTO users (User_Name, user_pass, phone, Email) VALUES ('%s', '%s', '%s', '%s');"),
			*UserName, *Password, *PhoneNum, *Email
		);

		// 调用 PostSql 函数执行 SQL 语句
		bool bSuccess = PostSql(ToSQL);

		if (bSuccess)
		{
			UE_LOG(LogWeiLoginServer, Log, TEXT("用户数据插入成功: UserName=%s, PhoneNum=%s"), *UserName, *PhoneNum);
			bRegister=true;
		}
		else
		{
			UE_LOG(LogWeiLoginServer, Error, TEXT("用户数据插入失败: UserName=%s, PhoneNum=%s"), *UserName, *PhoneNum);
		}
		
		/*UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessRegisterJson : RegisterSQL [%s] Uninque name"),*UserName);
		FString RegisterSQL = TEXT("insert into users (users.user_name,users.user_pass,users.phone,users.email,users.money) value(?,?,?,?,0);");

		FString ErrMesg;
		//FSimpleMysqlQueryStmt是value、type结构体，用于将值打到字符串上面
		TArray<FSimpleMysqlQueryStmt> QueryStmtsParams;

		//加入到字符串里
		FSimpleMysqlQueryStmt UserNameStmt;
		UserNameStmt.VariableType=EMysqlVariableType::MYSQL_VARCHAR;
		UserNameStmt.Value=*UserName;
		QueryStmtsParams.Add(UserNameStmt);

		FSimpleMysqlQueryStmt PasswordStmt;
		PasswordStmt.VariableType=EMysqlVariableType::MYSQL_VARCHAR;
		PasswordStmt.Value=*Password;
		QueryStmtsParams.Add(PasswordStmt);

		FSimpleMysqlQueryStmt PhoneNumStmt;
		UserNameStmt.VariableType=EMysqlVariableType::MYSQL_VARCHAR;
		UserNameStmt.Value=*PhoneNum;
		QueryStmtsParams.Add(UserNameStmt);
		
		FSimpleMysqlQueryStmt EmailStmt;
		EmailStmt.VariableType=EMysqlVariableType::MYSQL_VARCHAR;
		EmailStmt.Value=*Email;
		QueryStmtsParams.Add(EmailStmt);
		FSimpleMysqlQueryStmtResult SimpleMysqlQueryStmtResult;

		//传入Mysql对象，传入SQL语句，传入参数，传入报错信息
		//函数处理注册SQL，判断是否会被SQL注入攻击
		bool bRegisterLinkStmt=USimpleMySQLLibrary::QueryLinkStmt(MysqlObjectWrite,RegisterSQL,QueryStmtsParams,SimpleMysqlQueryStmtResult,ErrMesg);

		if (!bRegisterLinkStmt)
		{
			UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessRegisterJson : RegisterSQL failed :[%s] "),*ErrMesg);
		}
		else
		{
			UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessRegisterJson : RegisterSQL succeed "));
			bRegister=true;
		}*/

		
		
	}

	
	// 3.查询是否注册成功拿到返回的ID
	FString UserID=TEXT("-1");
	if (bRegister)
	{
		//查询一下刚才注册的用户id，其实可以不查，这个地方多了一条sql，可以自动返回的主键
		FString UserIDSQL=FString::Printf(TEXT("SELECT ID,user_name FROM users ;"));
		TArray<FSimpleMysqlResult> UserIDResults;
		if (GetSql(UserIDSQL, UserIDResults))
		{
			if (UserIDResults.Num() > 0)
			{
				for (auto & Tmp: UserIDResults)
				{
					if (FString * UserNameDataBase=Tmp.Rows.Find(TEXT("user_name")))
					{
						if (*UserName == *UserNameDataBase)
						{
							if (FString*ID=Tmp.Rows.Find(TEXT("id")))
							{
								UserID=*ID;
							}
						}
					}
				}
			}
		}
	}

	// 拼一个json
	FString ResponseBody=TEXT("");
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseBody);
	
	FString InterviewType=TEXT("RegisterResponse");
	FString RegisterResult=TEXT("");
	TSharedRef<FJsonObject> RequestObj=MakeShared<FJsonObject>();
	if (bRegister)
	{
		UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessRegisterJson :  [%s] Response to successfully"),*UserName);
		RegisterResult=TEXT("Succeed");
	}
	else
	{
		UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessRegisterJson :  [%s] Response to Unsuccessfully"),*UserName);
		RegisterResult=TEXT("Failed");

	}
	RequestObj->SetStringField("RegisterResult",RegisterResult);
	RequestObj->SetStringField("ID",UserID);
	RequestObj->SetStringField("UserName",UserName);
	RequestObj->SetStringField("InterviewType",InterviewType);
	RequestObj->SetStringField("Guid",Guid);
	FJsonSerializer::Serialize(RequestObj,Writer);

	UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessRegisterJson :  End=========================================="));

	
	/*FString Code = TEXT("200");
	FString Message = TEXT("Register In Yes");
	ResponseBody = FString::Printf(TEXT("{\"Code\":\"%s\",\"Message\":\"%s\"}"), *Code, *Message);*/
	return ResponseBody;
}

FString UHttpServerObject::HandleProcessLoginJson(FString InJsonStr)
{
	UE_LOG(LogWeiLoginServer, Display, TEXT("UHttpServerObject::HandleProcessLoginJson"));

	TSharedPtr<FJsonObject> HttpContentObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InJsonStr);
	FJsonSerializer::Deserialize(Reader, HttpContentObj);


	FString UserName = *HttpContentObj->GetStringField("UserName");
	FString Password = *HttpContentObj->GetStringField("Password");
	FString Guid = *HttpContentObj->GetStringField("Guid");

	UE_LOG(LogWeiLoginServer, Display, TEXT("UHttpServerObject::HandleProcessLoginJson:[%s] begin to vaild login information"),*UserName);

	// 查询数据库里是否有这个数据
	// 拼接方式容易受到sql注入攻击
	// FString SQL = FString::Printf(TEXT("select id,user_name,user_pass from users where user_name='%s' and user_pass='%s';"), *UserName, *Password);

	FString SQL = TEXT("select id,user_name,user_pass from users;");
	bool bLogin=false;
	int32 UserIDDataBase=-1;
	TArray<FSimpleMysqlResult> Results;
	// 拿数据
	if (GetSql(SQL,Results))
	{
		for (auto& TmpResult : Results) {
			if (FString* UserNameDataBase = TmpResult.Rows.Find(TEXT("user_name"))) {
				if (UserNameDataBase->Equals(*UserName)) {
					if (FString* UserPassDataBase = TmpResult.Rows.Find(TEXT("user_pass"))) {
						if (UserPassDataBase->Equals(*Password)) {
							if (FString* IDString = TmpResult.Rows.Find(TEXT("id"))) {
								UserIDDataBase = FCString::Atoi(**IDString);
							}

							bLogin = true;
						}
					}
					//找到了账户信息
					break;
				}
			}
		}

		
	}

	

	// 返回的字符串
	FString ResponseBody = TEXT("");
	// 创建一个写入流
	TSharedRef<TJsonWriter<>> Writer=TJsonWriterFactory<>::Create(&ResponseBody);
	// 准备好要去写入的对象
	TSharedRef<FJsonObject> RequestObj=MakeShared<FJsonObject>();

	// 准备写入的数据
	FString InterviewType=TEXT("LoginResponse");
	FString LoginResult=TEXT("");
	// 完成数据的写入到json对象
	RequestObj->SetStringField("InterviewType",InterviewType);
	RequestObj->SetStringField("UserName",UserName);
	RequestObj->SetStringField("ID",FString::FromInt(UserIDDataBase));
	RequestObj->SetStringField("Guid",Guid);
	if (bLogin)
	{
		UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessLoginJson:[%s] Success"),*UserName);
		LoginResult=TEXT("Succeed");
		RequestObj->SetStringField("LoginResult",LoginResult);
	}
	else
	{
		UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessLoginJson:[%s] UnSuccess"),*UserName);
		LoginResult=TEXT("Failed");
		RequestObj->SetStringField("LoginResult",LoginResult);
	}
	UE_LOG(LogWeiLoginServer,Display,TEXT("UHttpServerObject::HandleProcessLoginJson:[%s] end-=======================-"),*UserName);
	// 完成数据的写入到字符串
	FJsonSerializer::Serialize(RequestObj,Writer);
	return ResponseBody;
}


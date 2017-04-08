//本程序是战绩追踪的一部分。接收命令行指令来查询战地1数据并将数据保存到指定文件。
//使用TRN的API来获取战地1数据:http://docs.trnbattlefield.apiary.io/#introduction/parameters/game
//使用微软 C++ REST SDK 来发起 HTTP 请求并解码收到的JSON数据:http://microsoft.github.io/cpprestsdk/

//以下是命令行参数说明，每一行是一个参数的可选项
//ID=要查询的ID。
//GAME=游戏名，填"tunguska"。
//STATS=查询类型，可选:BasicStats(基本统计)、DetailedStats(详细统计)、DogTagImg(狗牌图片URL)、Weapons(武器统计)、Vehicles(载具统计)、KitRanks(兵种等级)。
//SAVETO=保存结果的文件名。
//TRANSLATEFILE=翻译文件名(填不填无所谓)。
//命令行示例:BFT_M ID=liuziangexit GAME=tunguska STATS=BasicStats SAVETO=1.txt(查询liuziangexit的战地1基本信息并保存到1.txt)

#pragma once
//地址和其他
#define BaseURI L"https://battlefieldtracker.com/bf1/api"
#define APIKEY L"40615011-4ab1-4da2-a7a9-7a8559ead70e"
#define LZAURI L"https://liuziangexit.com/BFT"
#define ServerAddrFilename L"ServerAddr.html"//服务器和本地都用这个文件名
#define TimeOut 30000
#define RecvLength 65535
#define BFT_Server_Response_Result_OK "OK"
#define BFT_Server_Response_Result_NO "NO"
#define ThisVersion "2000"
#define BFT_EXTERN_C  extern "C"  _declspec(dllexport)
//URI参数
#define Platform L"platform"
#define Name L"displayName"
#define Game L"game"
#define PC L"3"
#define BF1 L"tunguska"
#define BF4 L"bf4"
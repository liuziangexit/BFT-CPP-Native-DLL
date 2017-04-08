#pragma once
#include <iostream>
#include <string>
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "DC_STR.h"
#include "DC_File.h"
#include "DC_ERROR.h"
#include "define.h"

using namespace utility;
using namespace web::http;
using namespace web::http::client;
using namespace web;

http_client_config _config;
std::unique_ptr<http_client> BF_Client;
http_request BF_request;

//初始化
void init() {//初始化HTTP请求
	_config.set_timeout(utility::seconds(5));

	BF_request.headers().add(L"TRN-Api-Key", APIKEY);

	BF_Client.reset(new http_client(BaseURI, _config));
}

//GET请求
http_response HttpGet(http_client *_client, const std::wstring& _uri, http_request _request) {//发送 HTTP GET 请求
	_request.set_request_uri(_uri);
	return _client->request(_request).get();
}

//保存response的body到文件，仅支持文本(而不是二进制)
bool WriteBodyToFile(const http_response& res, const std::string& filename) {
	try {
		if (DC::File::write(filename, res.extract_utf8string().get()) == 1)
			return true;
	}
	catch (...) {}
	return false;
}

//判断response是否成功
inline bool judge_response(const http_response& response) {//判断 HTTP 响应码
	if (response.status_code() >= 200 && response.status_code() < 300) return true;
	return false;
}

//判断获取数据是否成功
bool judge_successful(const web::json::value& input) {//判断JSON是否合法
	auto rf = [] {return false; };
	try {
		auto rv = input.as_object();
		if (rv.at(L"successful").as_bool())
			return true;
		else
			return rf();
	}
	catch (...) {
		return rf();
	}
}

//合成资源实际地址
inline std::string GetImgURL(const std::string& base, const std::string& uri) {//提取JSON中的图片URL
	return DC::STR::replace(uri, DC::STR::find(uri, "[BB_PREFIX]"), base);
}

//查询基类
class StatsBase {
public:
	virtual ~StatsBase() = default;

	void DownloadStats(const std::string& ID, const std::wstring& game) {
		uri_builder _uri;
		SetPath(_uri);
		_uri.append_query(Platform, PC);
		_uri.append_query(Name, ID.c_str());
		_uri.append_query(Game, game.c_str());
		http_response rv;
		json::value result;
		try {
			rv = HttpGet(BF_Client.get(), _uri.to_string(), BF_request);
		}
		catch (const std::exception& ex) {
			throw DC::DC_ERROR("", ex.what(), 0);
		}
		catch (...) {
			throw DC::DC_ERROR("", "无法获取数据", 0);
		}
		
		if (judge_response(rv) != true) {
			throw DC::DC_ERROR("", "HTTP状态" + DC::STR::toString(rv.status_code()), 0);
		}
		try {
			result = rv.extract_json().get();
		}
		catch (...) {
			throw DC::DC_ERROR("", "无法获取数据", 0);//在这里大部分时候都是因为网络不好引起的，所以写网络错误而不是解析失败
		}
		Stats = result;
	}
	
	void Write(const std::string& filename) {
		if (!DC::File::write(filename, this->Get())) throw DC::DC_ERROR("", "无法写入到文件", 0);
	}

	virtual std::string Get() = 0;

protected:
	virtual void SetPath(uri_builder& input) = 0;

	json::value Stats;
};

//翻译基类
class Traslate {
public:
	virtual ~Traslate() = default;

protected:
	void ReadTranslateFile(const std::string& filename) {
		std::string file, line;
		TranslateResource.clear();
		try {
			file = DC::File::read(filename);
		}
		catch (...) {
			return;
		}		

		for (auto p : file) {
			if (p != '\n') { line += p; continue; }
			TranslateResource.push_back(line);
			line = "";
		}
	}

	std::string Translate(const std::string& input) {//翻译文件必须以'\n'结尾
		for (auto p = TranslateResource.begin(); p != TranslateResource.end(); p++) {
			if (input == *p) {
				if (p + 1 != TranslateResource.end()) return *(p + 1); else break;
			}
		}
		return input;
	}

private:
	std::vector<std::string> TranslateResource;
};

//基本信息查询
class BasicStats:public StatsBase {
public:
	virtual std::string Get()override {
		auto all = std::move(Stats);
		auto result = all[L"result"].as_object();
		std::string WriteThis;
		WriteThis += DC::STR::toString(result.at(L"rank").as_object().at(L"number").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"spm").as_double()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"wins").as_double() / result.at(L"losses").as_double()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"timePlayed").as_integer() / 3600) += "\n";
		WriteThis += DC::STR::toString(result.at(L"kpm").as_double()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"kills").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"deaths").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"kills").as_double() / result.at(L"deaths").as_double()) += "\n";
		WriteThis += std::move(GetImgURL(DC::STR::toString(all.at(L"bbPrefix").as_string()), DC::STR::toString(result[L"rank"].as_object().at(L"imageUrl").as_string()))) + "\n";
		WriteThis += DC::STR::toString(result.at(L"wins").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"losses").as_integer()) += "\n";
		return WriteThis;
	}

protected:
	virtual void SetPath(uri_builder& input) {
		input.append_path(L"Stats/BasicStats");
	}
};

//详细信息查询
class DetailedStats :public StatsBase {
public:
	virtual std::string Get()override {
		auto GetMostKillVehicle = [](const json::array& vehicleStats)->std::wstring {
			std::wstring name;
			int32_t biggest = 0;
			try {
				for (auto i : vehicleStats) {
					if (i.as_object().at(L"killsAs").as_integer() > biggest) {
						biggest = i.as_object().at(L"killsAs").as_integer();
						name = i.as_object().at(L"name").as_string();
					}
				}
				return name + L"(" + DC::STR::toType<std::wstring>(DC::STR::toString(biggest).c_str()) + L"kills)";
			}
			catch (...) {
				return L"NULL";
			}
		};
		auto GetKitStats = [](const json::array& kitStats, const std::wstring& name, const std::wstring& what) {
			for (auto p : kitStats) {
				if (p.at(L"name").as_string() == name) {
					return p.at(what).as_integer();
				}
			}
			throw DC::DC_ERROR("", "GetKitStats", 0);
		};
		auto GetVehicleKills = [](const json::array& vehicleStats, const std::wstring& name) {
			for (auto p : vehicleStats) {
				if (p.at(L"name").as_string() == name) {
					return p.at(L"killsAs").as_integer();
				}
			}
			return 0;
		};
		//Assault Support Medic Scout
		//Armoured Land Vehicles,Stationary Weapons,Air Vehicles,Horse,Transport Land Vehicles
		auto all = std::move(Stats);
		auto result = all[L"result"].as_object();
		std::string WriteThis;

		WriteThis += DC::STR::toString(result.at(L"squadScore").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"awardScore").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"bonusScore").as_integer()) += "\n";

		WriteThis += DC::STR::toString(result.at(L"kdr").as_double()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"saviorKills").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"killAssists").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"headShots").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"accuracyRatio").as_double()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"basicStats").at(L"wins").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"basicStats").at(L"losses").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"basicStats").at(L"skill").as_double()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"flagsCaptured").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"flagsDefended").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"repairs").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"revives").as_integer()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"roundsPlayed").as_integer()) += "\n";
		//某些ID没有favMAP
		if (result.at(L"roundHistory").as_object().at(L"favouriteMap").is_string()) {
			WriteThis += DC::STR::toString(result.at(L"roundHistory").as_object().at(L"favouriteMap").as_string()) += "\n";
		}
		else {
			WriteThis += "-NULL\n";
		}
		WriteThis += DC::STR::toString(result.at(L"favoriteClass").as_string()) += "\n";
		WriteThis += DC::STR::toString(result.at(L"longestHeadShot").as_double()) += "\n";
		WriteThis += DC::STR::toString(GetMostKillVehicle(result.at(L"vehicleStats").as_array())) += "\n";
		WriteThis += DC::STR::toString(result.at(L"highestKillStreak").as_integer()) += "\n";


		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Assault", L"score")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Assault", L"kills")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Medic", L"score")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Medic", L"kills")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Support", L"score")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Support", L"kills")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Scout", L"score")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Scout", L"kills")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Tanker", L"score")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Tanker", L"kills")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Cavalry", L"score")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Cavalry", L"kills")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Pilot", L"score")) += "\n";
		WriteThis += DC::STR::toString(GetKitStats(result.at(L"kitStats").as_array(), L"Pilot", L"kills")) += "\n";

		WriteThis += DC::STR::toString(GetVehicleKills(result.at(L"vehicleStats").as_array(), L"Air Vehicles")) += "\n";
		WriteThis += DC::STR::toString(GetVehicleKills(result.at(L"vehicleStats").as_array(), L"Stationary Weapons")) += "\n";
		WriteThis += DC::STR::toString(GetVehicleKills(result.at(L"vehicleStats").as_array(), L"Armoured Land Vehicles")) += "\n";
		WriteThis += DC::STR::toString(GetVehicleKills(result.at(L"vehicleStats").as_array(), L"Transport Land Vehicles")) += "\n";
		WriteThis += DC::STR::toString(GetVehicleKills(result.at(L"vehicleStats").as_array(), L"Horse"));

		return WriteThis;
	}

protected:
	virtual void SetPath(uri_builder& input) {
		input.append_path(L"Stats/DetailedStats");
	}
};

//狗牌信息查询
class DogTag :public StatsBase {
public:
	virtual std::string Get()override {
		auto res = Stats.as_object().at(L"result").as_array();
		auto Base = DC::STR::toString(Stats.as_object().at(L"bbPrefix").as_string());
		auto DogTag = DC::STR::toString(res[0].as_object()[L"imageUrl"].as_string());
		auto DogTagBack = DC::STR::toString(res[1].as_object()[L"imageUrl"].as_string());
		std::string WriteThis;
		WriteThis += GetImgURL(Base, DogTag) + "\n";
		WriteThis += GetImgURL(Base, DogTagBack);

		return WriteThis;
	}

protected:
	virtual void SetPath(uri_builder& input) {
		input.append_path(L"Loadout/GetEquippedDogtags");
	}
};

//武器信息查询
class WeaponStats :public StatsBase,public Traslate {
public:
	WeaponStats(const std::string& inputTraslateFilename) {
		ReadTranslateFile(inputTraslateFilename);
	}

	virtual std::string Get()override {
		if (!judge_successful(Stats)) return std::string();
		auto result = Stats.as_object().at(L"result").as_array();
		auto baseaddr = DC::STR::toString(Stats.as_object().at(L"bbPrefix").as_string());
		struct weapon {
			weapon(const std::string& inputimageURL, const std::string& inputname, const std::string& inputdescription,
				const std::string& inputkills, const std::string& inputaccuracy, const std::string& inputheadshots, const time_t& inputtime, const std::string& inputdps) {
				imageURL = inputimageURL;
				name = inputname;
				description = inputdescription;
				kills = inputkills;
				accuracy = inputaccuracy;
				headshots = inputheadshots;
				time = DC::STR::toString(inputtime / 3600);
				dps = inputdps;

				if (DC::STR::toType<int32_t>(inputkills) == 0 || inputtime == 0)
					kpm = "0";
				else {
					kpm = DC::STR::toString(((float)DC::STR::toType<int32_t>(inputkills) / (float)(inputtime / 60)));
					if (kpm == "inf") kpm = "0";
					if (kpm.size() > 6) kpm.resize(6);
				}
			}

			std::string imageURL, name, description, kills, accuracy, headshots, time, kpm, dps;//图片网址，名字，描述，击杀数，准度，爆头数，使用时间，dps
		};
		std::vector<weapon> WriteList;

		//读取并翻译
		for (auto p : result) {
			auto WeaponList = p.as_object().at(L"weapons").as_array();
			for (auto p2 : WeaponList) {
				auto GetDes = [&] {
					try {
						auto rv = DC::STR::toString(p2.as_object().at(L"description").as_string());
						return rv;
					}
					catch (...) {}
					return std::string("");
				};
				try {
					WriteList.push_back(weapon(GetImgURL(baseaddr, DC::STR::toString(p2.as_object().at(L"hires").as_object().at(L"Small").as_string())),
						Translate(DC::STR::toString(p2.as_object().at(L"name").as_string())),
						Translate(GetDes()),
						DC::STR::toString(p2.as_object().at(L"stats").as_object().at(L"values").as_object().at(L"kills").as_integer()),
						DC::STR::toString(p2.as_object().at(L"stats").as_object().at(L"values").as_object().at(L"accuracy").as_double()),
						DC::STR::toString(p2.as_object().at(L"stats").as_object().at(L"values").as_object().at(L"headshots").as_integer()),
						p2.as_object().at(L"stats").as_object().at(L"values").as_object().at(L"seconds").as_double(),
						DC::STR::toString((int)(p2.as_object().at(L"info").as_object().at(L"rateOfFire").as_integer()*p2.as_object().at(L"info").as_object().at(L"statDamage").as_double()))
					));
				}
				catch (...) {
				}
			}
		}

		//写入
		std::string WriteThis;
		for (auto p : WriteList) {
			WriteThis += p.name += "\n";
			WriteThis += p.imageURL += "\n";
			WriteThis += p.description += "\n";
			WriteThis += p.kills += "\n";
			WriteThis += p.headshots += "\n";
			WriteThis += p.accuracy += "\n";
			WriteThis += p.time += "\n";
			WriteThis += p.kpm += "\n";
		}

		return WriteThis;
	}

protected:
	virtual void SetPath(uri_builder& input) {
		input.append_path(L"Progression/GetWeapons");
	}
};

//载具信息查询
class VehicleStats :public StatsBase, public Traslate {
public:
	VehicleStats(const std::string& inputTraslateFilename) {
		ReadTranslateFile(inputTraslateFilename);
	}

	virtual std::string Get()override {
		if (!judge_successful(Stats)) return std::string("");
		auto result = Stats.as_object().at(L"result").as_array();
		auto baseaddr = DC::STR::toString(Stats.as_object().at(L"bbPrefix").as_string());
		struct vehicle {
			vehicle(std::string inputimageURL, std::string inputname, std::string inputdescription,
				std::string inputkills, time_t inputtime) {
				imageURL = inputimageURL;
				name = inputname;
				description = inputdescription;
				kills = inputkills;
				time = DC::STR::toString(inputtime / 3600);
				if (DC::STR::toType<int32_t>(inputkills) == 0 || inputtime == 0)
					kpm = "0";
				else {
					kpm = DC::STR::toString(((float)DC::STR::toType<int32_t>(inputkills) / (float)(inputtime / 60)));
					if (kpm == "inf") kpm = "0";
					if (kpm.size() > 6) kpm.resize(6);
				}
			}

			std::string imageURL, name, description, kills, time, kpm;//图片网址，名字，描述，击杀数，使用时间
		};
		std::vector<vehicle> WriteList;

		//读取并翻译
		for (auto p : result) {
			auto VehicleList = p.as_object().at(L"vehicles").as_array();
			for (auto p2 : VehicleList) {
				//某些载具没有描述，用GetDes来防止没有描述跳过
				auto GetDes = [&] {
					try {
						auto rv = DC::STR::toString(p2.as_object().at(L"description").as_string());
						return rv;
					}
					catch (...) {}
					return std::string("");
				};
				try {
					WriteList.push_back(vehicle(GetImgURL(baseaddr, DC::STR::toString(p2.as_object().at(L"imageUrl").as_string())),
						Translate(DC::STR::toString(p2.as_object().at(L"name").as_string())),
						Translate(GetDes()),
						DC::STR::toString(p2.as_object().at(L"stats").as_object().at(L"values").as_object().at(L"kills").as_integer()),
						p2.as_object().at(L"stats").as_object().at(L"values").as_object().at(L"seconds").as_double()
					));
				}
				catch (...) {}
			}
		}

		//写入
		std::string WriteThis;
		for (auto p : WriteList) {
			WriteThis += p.name += "\n";
			WriteThis += p.imageURL += "\n";
			WriteThis += p.description += "\n";
			WriteThis += p.kills += "\n";
			WriteThis += p.time += "\n";
			WriteThis += p.kpm += "\n";
		}

		return WriteThis;
	}

protected:
	virtual void SetPath(uri_builder& input) {
		input.append_path(L"Progression/GetVehicles");
	}
};

//兵种等级查询
class KitRanks :public StatsBase {
public:
	virtual std::string Get()override {
		auto result = Stats[L"result"].as_object();
		std::string WriteThis;
		WriteThis += DC::STR::toString(result.at(L"assault").as_object().at(L"rank").as_integer()) + "\n";
		WriteThis += DC::STR::toString(result.at(L"medic").as_object().at(L"rank").as_integer()) + "\n";
		WriteThis += DC::STR::toString(result.at(L"support").as_object().at(L"rank").as_integer()) + "\n";
		WriteThis += DC::STR::toString(result.at(L"scout").as_object().at(L"rank").as_integer()) + "\n";
		WriteThis += DC::STR::toString(result.at(L"tanker").as_object().at(L"rank").as_integer()) + "\n";
		WriteThis += DC::STR::toString(result.at(L"cavalry").as_object().at(L"rank").as_integer()) + "\n";
		WriteThis += DC::STR::toString(result.at(L"pilot").as_object().at(L"rank").as_integer()) + "\n";

		return WriteThis;
	}

protected:
	virtual void SetPath(uri_builder& input) {
		input.append_path(L"Progression/GetKitRanksMap");
	}
};
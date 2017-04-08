#include "BFT.h"
#include "liuzianglib.h"
#include "DC_type.h"

std::vector<DC::KeyValuePair> BFT_CPP_GetArgs(const wchar_t input[]) {
	std::string temp;
	std::string inputStr(DC::STR::toString(input));
	std::vector<DC::KeyValuePair> rv;
	DC::pos_type index = -1;

	while (true) {
		index++;
		if (input[index] == '\0') {
			rv.push_back(std::move(temp));
			break;
		}
		if (input[index] == ' ') {
			rv.push_back(std::move(temp));
			continue;
		}
		temp.push_back(input[index]);
	}

	return rv;
}

inline std::string BFT_CPP_GetValue(const std::vector<DC::KeyValuePair>& input, const std::string& name) {
	for (const auto& p : input) {
		if (DC::STR::toLower(p.GetName()) == DC::STR::toLower(name)) return p.GetValue();
	}
	return "";
}

BFT_EXTERN_C void BFT_CPP_CALL(const wchar_t input[], wchar_t output[], int outputSize) {
	auto args = BFT_CPP_GetArgs(input);
	std::string returnvalue;

	init();
	std::unique_ptr<StatsBase> Stats;
	if (BFT_CPP_GetValue(args, "stats") == "BasicStats") { Stats.reset(new BasicStats); }
	if (BFT_CPP_GetValue(args, "stats") == "DetailedStats") { Stats.reset(new DetailedStats); }
	if (BFT_CPP_GetValue(args, "stats") == "DogTagImg") { Stats.reset(new DogTag); }
	if (BFT_CPP_GetValue(args, "stats") == "Weapons") { Stats.reset(new WeaponStats(BFT_CPP_GetValue(args, "translatefile"))); }
	if (BFT_CPP_GetValue(args, "stats") == "Vehicles") { Stats.reset(new VehicleStats(BFT_CPP_GetValue(args, "translatefile"))); }
	if (BFT_CPP_GetValue(args, "stats") == "KitRanks") { Stats.reset(new KitRanks); }
	//如果没有Stats实例(即用户的Stats输入错误)则退出
	if (Stats.get() == nullptr) return;

	try {
		Stats->DownloadStats(BFT_CPP_GetValue(args, "id"), DC::STR::toType<std::wstring>(BFT_CPP_GetValue(args, "game")));
		returnvalue = Stats->Get();
	}
	catch (const DC::DC_ERROR& err) {
		returnvalue = "错误:" + err.GetDescription();
	}
	catch (...) {
		returnvalue = "错误:未捕获的异常";
	}

	auto toWide = DC::STR::toType<std::wstring>(returnvalue);
	if (!(toWide.size() >= outputSize)) memcpy(output, toWide.c_str(), sizeof(wchar_t) * toWide.size());
}
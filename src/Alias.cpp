#include "Alias.hpp"

std::unordered_map<std::string,std::vector<std::string>>Alias::replace_map;


void Alias::init(){
	replace_map["ls"]={"ls","--color=auto"};
	replace_map["l"]={"ls","--color=auto"};
}
std::vector<std::string>Alias::replace(const std::vector<std::string>&argv){
	if(argv.empty())return argv;
	auto token=replace_map.find(argv[0]);
	if(token==replace_map.end()){
		return argv;
	}else{
		std::vector<std::string>args;
		for(auto&arg:replace_map[argv[0]]){
			args.push_back(arg);
		}
		for(int i=1;i<argv.size();++i){
			args.push_back(argv[i]);
		}
		return args;
	}
}
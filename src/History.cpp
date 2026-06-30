#include "History.hpp"

/// 静态成员变量定义（分配实际存储空间）
std::vector<std::string> History::history;

void History::push(const std::string& line) {
	history.push_back(line);
}

std::vector<std::string> History::get_history() {
	return history;
}
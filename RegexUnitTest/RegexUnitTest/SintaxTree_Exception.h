#pragma once
#include<exception>
#include <iostream>
namespace regex {
	class SintaxTree_Exception : public std::exception
	{
	private:
		char incorrect_char;
	public:
		SintaxTree_Exception(char ch) :exception("unexpected symbol or incorrect operation usage: "), incorrect_char(ch) {};
		SintaxTree_Exception(std::string ch) :exception(ch.c_str()), incorrect_char('\0') {};
		friend std::ostream& operator <<(std::ostream& out, SintaxTree_Exception& st) {
			out << st.what() << st.incorrect_char << std::endl;
			return out;
		}
	};
}
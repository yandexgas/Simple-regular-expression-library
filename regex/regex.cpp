// regex.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#pragma once
//#include "DeterminedFinalAutomat.h"
//#include"SintaxTree.h"
#include "Regular_expression.h"
//namespace regex {
	//std::string ClosePriority::sign{")"};
	//std::string OpenPriority::sign{ "(" };
//	//std::string ScriningNode::sign{ "&" };
//	std::string OrNode::sign{ "|" };
//	std::string PositiveClosureNode::sign{ "+" };
//	std::string OptionNode::sign{ "?" };
//	std::string AnyNode::sign{ "." };
//	std::string RepeatNode::sign{ "{" };
//	std::string NamedGroup::sign{ "(<" };
//	std::string LinkNamedGroup::sign{ "<" };

	//std::int8_t ScriningNode::priority{ 5 };
	//std::int8_t NamedGroup::priority{ 4 };
	//std::int8_t OpenPriority::priority{ 0 };
	//std::int8_t ClosePriority::priority{ 0 };
	//std::int8_t PositiveClosureNode::priority{ 3 };
	//std::int8_t OptionNode::priority{ 3 };
	//std::int8_t RepeatNode::priority{ 3 };
	//std::int8_t OrNode::priority{ 1};
	//std::int8_t ConcatNode::priority{ 2 };
	//std::int8_t AnyNode::priority{ 0 };
	//std::int8_t LinkNamedGroup::priority{ 0 };
	//std::int8_t Leaf::priority{ 0 };

//	std::int16_t LinkNamedGroup::links{ 0 };
//	std::shared_ptr<std::unordered_map<std::string, std::string>> NamedGroup::groups{ std::make_shared<std::unordered_map<std::string,std::string>>() };
//	std::shared_ptr<std::unordered_map<std::string, std::string>> LinkNamedGroup::groups = { NamedGroup::groups };

//}


// В этой функции просто выполняются некоторые простые тесты библиотеки, чуть позже будут отдельные unit-тесты, для осуществления регрессионного тестирования
int main()
{
	/*std::string str = "abc(ab+|cd&??)&.(ssn{1,2})b!";
	for (int i = 0; i < 1; i++) {
		regex::SintaxTree st("a?");
		regex::DeterminedFinalAutomat dfa(st.stealNfa(), st.getAlphabet());
		st.clear();
		regex::SintaxTree st3(".+?");
		regex::DeterminedFinalAutomat dfa3(st3.stealNfa(), st3.getAlphabet());
		regex::DeterminedFinalAutomat dfa4;
		dfa4 =std::move(dfa3 * dfa);
		bool rs = false;
		rs = dfa4.checkString("");
		rs = dfa4.checkString("a");
		rs = dfa4.checkString("abd");
		rs = dfa.checkString("acacihpem");
		rs = dfa.checkString("cacihpem");
		rs = dfa.checkString("ihpem");
		rs = dfa.checkString("@ihpem");
	auto tt = dfa.regexRecover();
		std::cout << tt;
		regex::SintaxTree st2(tt);

		regex::DeterminedFinalAutomat dfa1(st2.stealNfa(), st2.getAlphabet());
		rs = dfa1.checkString("abcabbbd?.ssnb!");
		rs = dfa1.checkString("111");
		rs = dfa1.checkString("aa");
		rs = dfa1.checkString("");
		for (int i = 0; i < 10000000; i++) {
			rs = dfa.checkString("abcabbbd?.ssnb!");

			rs = dfa.checkString("c@");
			rs = dfa.checkString("bcabcac@");
			rs = dfa.checkString("");
			//rs = dfa.checkString("nfs://abcaaaaaaaaaaaaaaa/123415/&");
			//rs = dfa.checkString("nfs://a/123415/&");
			//rs = dfa.checkString("nfs://aa/123415/&1");
			//rs = dfa.checkString("nfs://a/123415/&1");
			//rs = dfa.checkString("nfs:///123415/&1");
		}
	}
	std::cout << "1";*/
	bool rs = false;
	try {
		rgx::Regular_expression r("a|");
	}
	catch (regex::SintaxTree_Exception e) {
		std::cout << e;
	}
	
	for (int i = 0; i < 1111111; i++) {
		rgx::Regular_expression st("(<a>a|b(<d>c|d))");
		rs = rgx::checkString("ac", st);
		rs = rgx::checkString("bc", st);
		rs = rgx::checkString("b", st);
		rs = rgx::checkString("a", st);
		rs = rgx::checkString("(<name>faf", "&(<name>faf)");
		rgx::Regular_expression stt("&(<name>faf)");
		rgx::Regular_expression r("mephi&&|@&.?ru+");
		rgx::Regular_expression r3("(<a>(ac)|(bd))1?");
		//Тут дополнение языка
		
		rgx::Regular_expression r1(r.make_language_addition());
		rs = rgx::checkString("mephi&ru", r1);
		rs = rgx::checkString("mephru", r1);
		rs = rgx::checkString("mephi&.ru", r1);
		rs = rgx::checkString("mephi&ruuuu", r1);
		rs = rgx::checkString("mephi&", r1);
		rs = rgx::checkString("@.ruuu", r1);
		rs = rgx::checkString("@ruuu", r1);
		rs = rgx::checkString("@druuu", r1);
		////
		//Инверсия
		rgx::Regular_expression r2(r.make_language_inversion());
		rs = rgx::checkString("mephi&ru", r2);
		rs = rgx::checkString("uuur.@ihpem", r2);
		rs = rgx::checkString("ur&ihpem", r2);
		rs = rgx::checkString("uuur@", r2);
		rs = rgx::checkString("&ihpem", r2);
		rgx::Regular_expression r6("b&.?r{2,4}");
		rgx::Regular_expression r7(r6.make_language_inversion());
		rs = rgx::checkString("rr.b", r7);
		rs = rgx::checkString("rrrrb", r7);
		rs = rgx::checkString("rrrb", r7);
		//findAll
		auto c = rgx::findAll("sacbd1ac1f", r3,true);
		rgx::Regular_expression r4("((<a>a)|(<b>b))<a><b>");
		rs = rgx::checkString("bbb", r4);
		rgx::Regular_expression r5("(<name>.{2,8})@(<mail>(m|a|i|l|g){2,8})&.com #(<k>abr|<mail>)");
		rs = rgx::checkString("yan.gas@gmail.com #gmail", r5);
		rs = rgx::checkString("yan.gas@gmail.com #abr", r5);
	}
}


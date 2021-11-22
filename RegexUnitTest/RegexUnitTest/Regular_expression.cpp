#include "Regular_expression.h"
using namespace regex;
std::string ClosePriority::sign{ ")" };
std::string OpenPriority::sign{ "(" };
std::string ScriningNode::sign{ "&" };
std::string OrNode::sign{ "|" };
std::string PositiveClosureNode::sign{ "+" };
std::string OptionNode::sign{ "?" };
std::string AnyNode::sign{ "." };
std::string RepeatNode::sign{ "{" };
std::string NamedGroup::sign{ "(<" };
std::string LinkNamedGroup::sign{ "<" };
std::int16_t LinkNamedGroup::links{ 0 };

std::shared_ptr<std::unordered_map<std::string, std::string>> NamedGroup::groups{ std::make_shared<std::unordered_map<std::string,std::string>>() };
std::shared_ptr<std::unordered_map<std::string, std::string>> LinkNamedGroup::groups = { NamedGroup::groups };
namespace rgx {
	
	Regular_expression::Regular_expression(std::string str,bool inverse):regular_expression_dfa_() {
			SintaxTree nfa(str, inverse);
			DeterminedFinalAutomat dfa(nfa.stealNfa(), nfa.getAlphabet());
			regular_expression_dfa_ = dfa;
	}

	Regular_expression::Regular_expression(std::string str, int) {
		SintaxTree nfa(str);
		DeterminedFinalAutomat dfa(nfa.stealNfa(), nfa.getAlphabet(), false);
		regular_expression_dfa_ = dfa;
	}

	Regular_expression::Regular_expression(Regular_expression&& obj) noexcept {
		regular_expression_dfa_ = std::move(obj.regular_expression_dfa_);
	}

	std::string Regular_expression::restore_expression() const {
		return regular_expression_dfa_.regexRecover();
	}

	Regular_expression Regular_expression::make_language_inversion() const noexcept{
		Regular_expression res(restore_expression(), true);
		return res;
	}

	Regular_expression Regular_expression::make_language_addition() const noexcept {
		SintaxTree full(".+?");
		DeterminedFinalAutomat full_lang(full.stealNfa(), full.getAlphabet());
		DeterminedFinalAutomat addition;
		addition= std::move(full_lang * regular_expression_dfa_);
		return addition;
	}

	Regular_expression& Regular_expression::operator=(Regular_expression& obj) {
		if (this == &obj)
			return *this;
		regular_expression_dfa_ = obj.regular_expression_dfa_;
		return *this;
	}

	Regular_expression& Regular_expression::operator=(Regular_expression&& obj) noexcept {
		regular_expression_dfa_ = std::move(obj.regular_expression_dfa_);
		return *this;
	}

	bool checkString(std::string str, std::string regular) {
		Regular_expression reg(regular, 1);
		return reg.regular_expression_dfa_.checkString(str);
	}

	bool checkString(std::string str, std::string regular, RgxResult& rs) {
		Regular_expression reg(regular, 1);
		return checkString(str, reg, rs);
		/*bool res = reg.regular_expression_dfa_.checkString(str);
		if (res)
			rs.goodSubstr.push_back(str);
		if (!rs.namedGroups) {
			std::vector<std::unordered_map<std::string, std::string>> tmpres;
			tmpres.push_back(regex::NamedGroup::getTable());
			rs.namedGroups = std::move(tmpres);
		}
		else
			rs.namedGroups.value().push_back(regex::NamedGroup::getTable());
		return res;*/
	}

	bool checkString(std::string str, Regular_expression& regular) {
		return regular.regular_expression_dfa_.checkString(str);
	}
	bool checkString(std::string str, Regular_expression& regular, RgxResult& rs) {
		bool res = regular.regular_expression_dfa_.checkString(str);
		rs.goodSubstr.clear();
		if (rs.namedGroups)
			rs.namedGroups.value().clear();
		if (res)
			rs.goodSubstr.push_back(str);
		if (!rs.namedGroups) {
			std::vector<std::unordered_map<std::string, std::string>> tmpres;
			tmpres.push_back(regex::NamedGroup::getTable());
			rs.namedGroups = std::move(tmpres);
		}
		else
			rs.namedGroups.value().push_back(regex::NamedGroup::getTable());
		return res;
	}

	RgxResult findAll(std::string str, std::string regular, bool links) {
		std::vector<std::unordered_map<std::string, std::string>> groups;
		RgxResult rs;
		Regular_expression reg(regular, 1);
		rs.goodSubstr = reg.regular_expression_dfa_.findAll(str, links ? &groups : nullptr);
		if (links) {
			rs.namedGroups = groups;
		}
		return rs;
	}
	RgxResult findAll(std::string str, Regular_expression& regular, bool links) {
		std::vector<std::unordered_map<std::string, std::string>> groups;
		RgxResult rs;
		rs.goodSubstr = regular.regular_expression_dfa_.findAll(str, links ? &groups : nullptr);
		if (links) {
			rs.namedGroups = groups;
		}
		return rs;
	}
}
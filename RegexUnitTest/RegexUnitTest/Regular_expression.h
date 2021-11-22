#pragma once
#include "DeterminedFinalAutomat.h"
#include"SintaxTree.h"
namespace rgx {
	using namespace regex;
	class Regular_expression;
//  ������� ��������� ���������� ������� ������, �������� ������ �����, ���� ������������ ��� ���������, ��������� �������� � ���������� ��� ���������� ���������
// ����������� ������ �� ������ ����� �������
	struct RgxResult
	{
		std::vector<std::string> goodSubstr;
		std::optional<std::vector<std::unordered_map<std::string, std::string>>> namedGroups;
	};
// ������� ��������� ��� ������ - ������������� � ���������� ���������.
// ���������� true ���� ������ �������� �������, � false ���� �� ��������
// ��� ��������� ���������� ������������� sintax_tree_exception
	bool checkString(std::string, std::string);
// ��������� ������ ������, � ������ RgxResult ���������� �������� ������, ���� ��� ������ ��������
// � ��� �� �������� �� ���� ����� ������� ����������� ��������� (���� ��� ����).
	bool checkString(std::string, std::string, RgxResult&);
// ������� ��������� ������ ������ �� ������� ���������� ������� ����������� ���������
	bool checkString(std::string, Regular_expression&);
// ������� ��������� ������ ������ �� ������� ���������� ������� ����������� ��������� � ���������� �������� ���� ����� ������� � ������ RgxResult
	bool checkString(std::string, Regular_expression&, RgxResult&);
// ���� ��� ���������������� ��������� �������� ������ 1, ������� �������� ��� ���������� ��������� ������ 2,
// ���� links = true, � ������������ ������� ������ �������� ����� ������ ��������������� �� �������� ����� �������
	RgxResult findAll(std::string, std::string, bool links = false);
// ���� ��� ���������������� ��������� �������� ������ 1, ������� �������� ��� ���������� ��������� (�������� 2),
// ���� links = true, � ������������ ������� ������ �������� ����� ������ ��������������� �� �������� ����� �������
	RgxResult findAll(std::string, Regular_expression&, bool links = false);

// ����� ����������� ���������, ������������ ����� ���������������� ����������� ��� (���� ������), �� ��������� ����������� ���������.
// �� ������ ���������� ��������� � ��������� ����.
// 
// ����������� �� string � bool - ����������� ����������� ������� �� ����������� ���������, ���� bool - true, ������������ �������, ��������������� �������� ��������� �����.
// ��� ������������ ���������� ��������� ����� ��������� ���������� sintax_tree_exception, � ���������� ����� ������ � ��������� ����� ������ �������� �������������� ������� �����
// 
// restore_expression() ���������� ���������� ���������, �� �������� �������� ������ ������� (���� ��� �� �������� � ��������, �� ��� ������������� �������� ��������� ����� ������������ ���� �����)
// ��������� ����������������� ������� �������������� �-�����, ������� ������� ���������� �� ����������� � ������� ����� �������, ��� ��������, ������ ����� ����� ��������� ������������,
// �������, ����������� �� ���������������� ��������� ����� ����������� � ���������� ��������, ������������ �� ��������� ���������./
// ��� ������� ������� ��������� (��� ������� �������, � �������� ���� ������� ����� ���������� � ���������) �������������� ���������� � ����� ��������� std::bad_alloc
//
//make_language_inversion() ���������� ��������� ������ ��������, ��������������� �������� �����, �� �������� �������� �������� �������. (�� ������ �������� �������).
//����������: ���� ���� ���������� ��������� ������� �������, ������� ������������ ����������� �� ������, � ���������� bool=true, ������ ������� ������.
//
//make_language_addition()���������� ��������� ������ ��������, ��������������� ��������� �����, �� �������� �������� �������� �������. (�� ������ �������� �������).
	class Regular_expression
	{
	private:
		DeterminedFinalAutomat regular_expression_dfa_;
		Regular_expression() {};
		Regular_expression(DeterminedFinalAutomat& at) : regular_expression_dfa_(at) {}
		friend bool checkString(std::string, std::string);
		friend bool checkString(std::string, std::string, RgxResult&);
		friend bool checkString(std::string, Regular_expression&);
		friend bool checkString(std::string, Regular_expression&, RgxResult&);
		friend RgxResult findAll(std::string, std::string, bool links);
		friend RgxResult findAll(std::string, Regular_expression&, bool links);
		Regular_expression(std::string, int);

	public:

		Regular_expression(std::string,bool inverse=false);
		Regular_expression(Regular_expression& obj) :regular_expression_dfa_(obj.regular_expression_dfa_) {}
		Regular_expression(Regular_expression&&)noexcept;
		~Regular_expression() {};
		std::string restore_expression() const;
		Regular_expression make_language_inversion() const noexcept;
		Regular_expression make_language_addition() const noexcept ;
		Regular_expression& operator=(Regular_expression&);
		Regular_expression& operator=(Regular_expression&&) noexcept;
	};
	
}
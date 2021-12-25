#pragma once
#include "Nodes.h"
#include <stack>
#include <unordered_set>
namespace regex {
	
	
// ����� ��������������� ������, ������� ������ ������ � ������������ �� ��������� ���������� ����������� ���������
// ���� inverse = true �� ������ ������ ��� �������� �����.
// SintaxTree_Exception ������������� ��� ��������� ����������
// ������ ������ ����� ���� ������� � ������� (� �������� � ����� �������)
// � ���������� ���������� ������ � ��� ����� ��� �� ����� ������ ���, ��������������� ������� ����������� ���������
// � ���������� ������ ���������� ����������� ������ � ��� �������� ���� ���������/�������, ������� ��� ������ � ��� ����� �������������� ��� � ���
// ������� ������ - ��� �������, ������� ������������� ���� �� ���� ���� ������, ����� � ��������, ����� ������� ������� ��� ��� ������������� ���������� ������ ��� ��������� �������.
// buildTreePrepare �������������� ������ � ���������� ������, ���������� �� ������� � ������ ������������� ����� ������ - ������� � ������������.
// buld ������ ������ ������� ������������� ������, �������� 5*���������� ��� �������� � �������� ������ ������� ������ ����� (� ���������� ������� ������ ����� ����� ����������� � ����������)
// �� ���� ������ ������ ���� ������ �������������� ��� �������� ���������� ����������.
// pass - ����� ������ ������, ������� � ����� �� ���������� ������ �������������� ����� ��� ������ ������, � � ������ ������ ����� "����-�����-�����"
// ��� ���������� ��� ��� ������� ����, � ���������� � ����� ����� ��� ��� ����� ������. (�������� �� ��������� ����� - ��������)
	class SintaxTree
	{
	private:
		std::shared_ptr<Node> root_;
		std::list<std::shared_ptr<Node>> nodesList_;
		std::unordered_set<std::string> alphabet;
		bool buildTreePrepare(const std::string& sourceString);
		void build(bool inverse = false);
		void build(std::list<std::shared_ptr<Node>>::iterator, std::list<std::shared_ptr<Node>>::iterator, bool inverse = false);
	public:
		SintaxTree(const std::string& sourceString, bool inverse = false);
		~SintaxTree() { NamedGroup::tableClear(); };
		void clear() {
			root_ = nullptr;
		}
		void print() const {
			int deep = 0;
			std::shared_ptr<Node> ptr = root_;
			pass(0, ptr);	
		}
		void pass(int deep, std::shared_ptr<Node> w) const;
		void pass(std::shared_ptr<Node>w);
		inline const std::unordered_set <std::string>& getAlphabet() {
			return alphabet;
		}
		inline std::unique_ptr<NondeterminedFinalAutomata> stealNfa() {
			return std::move(root_->stealNodesNfa());
		}
	};
}

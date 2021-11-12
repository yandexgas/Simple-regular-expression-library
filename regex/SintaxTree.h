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
		bool buildTreePrepare(const std::string& sourceString) noexcept;
		void build(bool inverse = false);
		void build(std::list<std::shared_ptr<Node>>::iterator, std::list<std::shared_ptr<Node>>::iterator, bool inverse = false);
	public:
		SintaxTree(const std::string& sourceString, bool inverse = false) {
			buildTreePrepare(sourceString);
			build(inverse);
			pass(root_);
		};
		~SintaxTree() { NamedGroup::tableClear(); };
		void clear() {
			root_ = nullptr;
		}
		void print() {
			int deep = 0;
			std::shared_ptr<Node> ptr = root_;
			pass(0, ptr);
			
		}
		void pass(int deep,std::shared_ptr<Node> w) {
			if (w->getLeft() != nullptr)
				pass(deep + 1, w->getLeft());
			for (int i = 0; i < deep * 10; i++)
				std::cout << " ";
			std::cout << w->getSimbol() << std::endl;
			if (w->getRight() != nullptr)
				pass(deep + 1, w->getRight());
		}
		void pass(std::shared_ptr<Node>w) {
			if (w->getLeft() != nullptr)
				pass(w->getLeft());
			if (w->getRight() != nullptr)
				pass(w->getRight());
			w->buildNfa();		
		}
		inline const std::unordered_set <std::string>& getAlphabet() {
			return alphabet;
		}
		inline std::unique_ptr<NondeterminedFinalAutomata> stealNfa() {
			return std::move(root_->stealNodesNfa());
		}
	};
}

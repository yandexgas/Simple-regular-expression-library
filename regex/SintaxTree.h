#pragma once
#include "Nodes.h"
#include <stack>
#include <unordered_set>
namespace regex {
	
	
// Класс синтаксического дерева, который строит дерево в конструкторе по заданному строковому регулярному выражению
// если inverse = true то строит дерево для инверсии языка.
// SintaxTree_Exception выбрасывается при нарушении синтаксиса
// объект дерева может быть выведен в консоль (в основном в целях отладки)
// В результате построения дерева в его корне так же будет лежать НКА, соответствующий данному регулярному выражению
// В актуальной версии библиотеки возможность работы с НКА напрямую была исключена/урезана, поэтому для работы с ним лучше препобразовать его в ДКА
// алфавит дерева - все символы, которым соответствует хотя бы один лист дерева, нужен в основном, чтобы быстрее строить ДКА без необходимости перебирать вообще все возможные символы.

// buildTreePrepare подготавливает строку к построению дерева, преобразуя ее символы в список незаконченных узлов дерева - листьев и метасимволов.
// buld строит дерево методом многократного обхода, совершая 5*количество пар открытых и закрытых скобок обходов списка узлов (в результате каждого обхода часть узлов склеивается в поддеревья)
// за один проход внутри пары скобок обрабатываются все операции одинкового приоритета.
// pass - метод обхода дерева, который в одной из перегрузок делает центрированный обход для печати дерева, а в другой делает обход "лево-право-центр"
// для построения НКА для данного узла, в результате в корне будет НКА для всего дерева. (Строится по алгоритму Ямада - Томпсона)
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

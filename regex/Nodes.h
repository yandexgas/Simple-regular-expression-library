#pragma once
#include <iostream>
#include<vector>
#include <ranges>
#include <algorithm>
#include<limits>
#include<string>
#include <unordered_set>

#include <list>
#include<sstream>
#include "NondeterminedFinalAutomata.h"
#include "SintaxTree_Exception.h"
namespace regex {
// Перечисление типов узлов, нужно чтобы быстро определять тип класса не используя typeId
	enum class NodeTypes {
		LEAF,
		SCREENING,
		OR,
		CONCAT,
		POSITIVE_СLOSURE,
		OPTION,
		ANY,
		REPEAT,
		NAMED_GROUP,
		LINK_NAMED_GROUP,
		OPEN_PRIORITY,
		CLOSE_PRIORITY
	};
//шаблон функции генерации узла по символу анализируемой строки и "метасимволу" соотвутствующему данному типу узла
	template<class T>
	std::unique_ptr<T> CrtNode(std::string::const_iterator& iter,std::string sign)  {
		std::string tmp;
		int count = 0;
		for (int i : std::ranges::views::iota(0, (int)sign.length())) {
			tmp += *iter++;
			count++;
		}
		
		if (tmp == sign) {
			--iter;
			return  std::make_unique<T>();
		}
		else {
			iter -= count;
			return nullptr;
		}
	}
// Абстрактный класс узла дерева, имеет указатель на алфовит, на автомат узла (если он создан и не украден узлами стоящими в иерархии выше), указатель на родителя
// По факту определяет интерфейс любого узла, а именно методы:
// создать узел по символу (а точнее по текущему итератору анализируемой строки)
// получить тип узла (это быстрее чем использовать typeid)
// установить детей по итератору на данный узел в списке узлов, с возможностью делать инверсию (для бинарных узлов)
// метод который позволяет определить, является ли узел завершенным
// построить НКА узла и установить родителя
// получить указатели на левое и правое поддерево
// "украсть" указатель на НКА узла (потому что как правило НКА любого узла используется только 1 раз, поэтому есть смысл отдать этот указатель тому объекту, который его просит
// и не хранить его дальше)

	class Node
	{
	protected: 
		std::weak_ptr<Node> parent_;
		std::unordered_set<std::string>* alphabet;
		std::unique_ptr<NondeterminedFinalAutomata> automatOfNode;
	public:
		Node() : parent_(),automatOfNode(nullptr),alphabet(nullptr) {}
		virtual ~Node() {}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const = 0;
		virtual NodeTypes getType() const=0;
		virtual bool setChild(std::list<std::shared_ptr<Node>>::iterator, bool inverse =false) = 0;
		virtual bool complited() const noexcept = 0;
		virtual std::int8_t getPriority()const noexcept = 0;
		virtual std::string getSimbol() const noexcept = 0;
		virtual void buildNfa() = 0;
		inline void setParent(std::shared_ptr<Node> iter) noexcept {
			parent_ = iter;
		}
		virtual std::shared_ptr<Node>  getLeft()const noexcept { return nullptr; }
		virtual std::shared_ptr<Node> getRight()const noexcept { return nullptr; }
		std::unique_ptr<NondeterminedFinalAutomata> stealNodesNfa() {
			return std::move(automatOfNode);
		}
		std::weak_ptr<Node> getParent() const noexcept { return parent_; }
		auto operator <=>(Node* b) {
			return getPriority() <=> (*b).getPriority();
		}
		void setAlphabet(std::unordered_set<std::string>* a) noexcept {
			alphabet = a;
		}
	};

//Класс листа определяет узел, который всегда будет листом дерева, это всегда символ алфавита и не метасимвол, если только он не был экранирован
//этот узел всегда завершенный потому что у него не может быть детей и не обязаны быть родители
// НКА данного узла - просто два состояния, с переходом из 1 во 2 по символу листа
	class Leaf :public Node
	{
	private:
		static const std::int8_t priority = 0;
		const std::string sign_;
	public:
		Leaf( const std::string sgn = "") :Node(), sign_(sgn) {}
		virtual ~Leaf()override {}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			std::string s;
			s += *iter;
			return std::make_unique<Leaf>(s);
		}
		virtual NodeTypes getType() const noexcept override {
			return NodeTypes::LEAF;
		}
		virtual bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override {
			return true;
		}
		virtual bool complited() const noexcept override {
			return true;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return Leaf::priority;
		}
		virtual std::string getSimbol() const  noexcept override {
			return sign_;
		}
		virtual void buildNfa() override {
			std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
			std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
			start->addTransition(end, getSimbol());
			automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
		}
	};
// класс метасимвола, наследуемый от узла, хранит указатели на левое и правое поддерево.
// в общем случае представляет тип Нуль-арного оператора, то есть того, у которого нет детей, поэтому так же является всегда завершенным
	class MetaNode :public Node
	{
		
	protected:
		std::shared_ptr<Node> left_;
		std::shared_ptr<Node> right_;
	public:
		MetaNode() :Node() {
			left_ = nullptr;
			right_ = nullptr;
		};
		virtual ~MetaNode()override {};
		virtual std::shared_ptr<Node> getLeft() const noexcept override { return left_; }
		virtual std::shared_ptr<Node> getRight() const noexcept override { return right_; }
	};

	class SoloMetaNode :public MetaNode
	{
	public:
		SoloMetaNode() :MetaNode() {};
		virtual bool setChild(std::list<std::shared_ptr<Node>>::iterator, bool inverse = false) override {
			return true;
		}
		virtual bool complited()const noexcept override {
			return true;
		}
	};
// Класс унарного метасимвола, у которого есть сторона действия 0- лево, 1 - право, метод установки детей устанавливает узел
// с соответствующей стороны итератора как поддерево с соотвтествующей стороны, а вторую стороны оставляет  null
// является завершенным, если нужная сторона заполнена
	class UnaryMetaNode :public MetaNode
	{
	private:
		short int side_;
	public:

		UnaryMetaNode( short int side = 0) :MetaNode() {
			if (side == 0 || side == 1)
				this->side_ = side;
			else
				throw std::invalid_argument("Incorrect side: "+side);
		};

		virtual bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override {
			if (complited())
				return false;
			switch (side_)
			{
			case 0: {
				if ((**(--iter)).complited()) {
					left_ = *(iter);
					left_->setParent(*++iter);
				}
				else
					throw SintaxTree_Exception("Incorrect usage of operation. Some of oherations have less operands then they should.");
					break; 
			}
			case 1:
				if ((**(++iter)).complited()) {
					right_ = *(iter);
					right_->setParent(*--iter);
				}
				else
					throw SintaxTree_Exception("Incorrect usage of operation. Some of oherations have less operands then they should.");
				break;
			}
			return true;
		}
		virtual bool complited() const noexcept override {
			return side_ == 0 ? left_ != nullptr : right_ != nullptr;
		}
	};
// бинарный оператор, является завершенным если оба ребенка не пусты
	class BinaryMetaNode : public MetaNode
	{
	public:
		BinaryMetaNode() :MetaNode() {};
		virtual bool complited() const noexcept override {
			return left_!=nullptr&&right_!=nullptr;
		}
		virtual bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override {
			if (complited())
				return false;
			auto leftNode = --iter;
			++iter;
			auto rightNode = ++iter;
			iter--;
				if ((*leftNode)->complited()&& (*rightNode)->complited()) {
					right_ = !inverse ? *rightNode : *leftNode;
					right_->setParent(*(iter));
					left_ = !inverse ? *leftNode : *rightNode;
					left_->setParent(*iter);			
				}
				else
					throw SintaxTree_Exception("Incorrect usage of operation. Some of oherations have less operands then they should.");		
				return true;
		}		
	};
// Метасимвол экранирования, пример унарного правостороннего оператора, любой узел справа от себя уничтожает и помещает себе ребёнком листм с символом уничтоженного узла.
// НКА для данного узла - просто НКА его ребёнка - листа
	class ScriningNode :public UnaryMetaNode
	{
	private:
		static const std::int8_t priority = 5;
		static std::string sign;
	public:
		ScriningNode() :UnaryMetaNode(1) {};
		virtual NodeTypes getType() const override {
			return NodeTypes::SCREENING;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<ScriningNode>(iter, sign);
		}
		virtual std::string getSimbol() const noexcept override {
			return ScriningNode::sign;
		}
		static std::string getSign()  noexcept {
			return ScriningNode::sign;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return ScriningNode::priority;
		}
		virtual bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override {
			if (complited())
				return false;
			++iter;
			right_ = std::make_shared<Leaf>((*iter)->getSimbol());
			right_->setParent(*--iter);
			return true;
		}
		virtual void buildNfa()override {
			automatOfNode = right_->stealNodesNfa();
		}
	};
// Метасимвол именованной группы захвата, правосторонний оператор
// основные поля - имя группы (должно быть уникально, иначе будет вызвано исключение)
// статический член - указатель на таблицу со всеми именами групп и соответствующими ими строками
// дружественным является класс ссылки на именованную группу.
// определяет класс события состояния автомата, для детектирования начала и конца группы захвата и записи строки по соответствующиему имени в таблицу, при анализе строки по регулярному выражению
// автомат узла - автомат его ребёнка, но в начало и конец автомата добавлено Событие детектирования границ группы захвата (как экземпляр объекта-события)
// имеет статические методы для работы с таблицей групп (очищение и получение)
	class NamedGroup:public UnaryMetaNode
	{
	private:
		static const std::int8_t priority = 4;
		const std::string::const_iterator eos_;
		std::string nameOfGroup_;
		static std::string sign;
		static std::shared_ptr<std::unordered_map<std::string, std::string>> groups;
		friend class LinkNamedGroup;

		class NamedGroupAction : public Action
		{
		private:
			bool visited=false;
			std::string::const_iterator begin;
			std::string::const_iterator end;
			std::string nameofgroup_;
			bool ofset;
			void doAsExit(std::string::const_iterator& currentChar) noexcept {
				if (currentChar > begin)
					begin = currentChar;
			};
			void doAsEnter(std::string::const_iterator& currentChar)  {
				if (begin._Ptr != nullptr) {
					if (currentChar >= end)
						end = currentChar;
					std::string str;
					if (begin <= end) {
						auto tmp = begin;
						for (; tmp != end; tmp++)
							str += *tmp;
						str += *end;
						tmp++;
					}
					(*groups)[nameofgroup_] = str;
				}
			};
		public:
			NamedGroupAction(std::string name,bool ofset=false) : nameofgroup_(name),ofset(ofset) {};
			virtual void doAction(std::string::const_iterator& currentChar, bool isEnter = true,AutomataState* state = nullptr)override {
				if (isEnter)
					doAsEnter(currentChar);
				else
					doAsExit(currentChar);
			};
			virtual ~NamedGroupAction() override {}
		};
	public:
		NamedGroup( std::string name = "", const std::string::const_iterator end = {}) :UnaryMetaNode(/*"(<",*/ 1), eos_(end), nameOfGroup_(name) {
			if (groups->contains(name) && name != "") {
				throw SintaxTree_Exception("repeated usage name of group");
			}
			else if (name != "") {
				(*groups)[name] = "";
			}
		};
		virtual ~NamedGroup() override {
		}
		virtual NodeTypes getType() const override {
			return NodeTypes::NAMED_GROUP;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return NamedGroup::priority;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const  override {
			std::string nameOfGroup_ = "";
			std::string tmp;
			bool correct = false;
			int count =0;
			for (int i : std::ranges::views::iota(0, (int)sign.length())) {
				if (iter != eos_) {
					count++;
					tmp += *iter++;
				}
				else break;
			}
			if (tmp == sign) {
				while (iter!=eos_ && !correct)
				{
					
					if (*iter == '>' && count > 2) {
						count = 0;
						while (iter != eos_ && !correct)
						{
							if (*iter == ')' && count > 0 && *(iter - 1) != ScriningNode::getSign()[0])
								correct = true;
							else if (*iter == ')')
								return nullptr;
							else
							{
								++iter;
								++count;
							}
						}
					}
					else if (std::isalpha(*iter))
					{
						nameOfGroup_ += *iter;
						++iter;
						++count;
					}
					else break;
				}
			}
			iter -= count;
			return correct ? std::make_unique<NamedGroup>(nameOfGroup_) : nullptr;
		}
		virtual std::string getSimbol() const noexcept override {
			return NamedGroup::sign + nameOfGroup_ + ">";
		}
		/*virtual bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override {
			if (complited())
				return false;
			UnaryMetaNode::setChild(iter);
			if (complited()) {
				if (groups->contains(nameOfGroup_))
					throw SintaxTree_Exception("repeted usage name for group: " + nameOfGroup_);
				else
					(*groups)[nameOfGroup_] = "";
			}
			return true;

		}*/
		virtual void buildNfa() override {
			automatOfNode = right_->stealNodesNfa();
			bool ofset = false;
		if (parent_.lock() != nullptr) {
				if (parent_.lock()->getType() == NodeTypes::CONCAT) {
					if (parent_.lock()->getRight()->getType() == NodeTypes::NAMED_GROUP) {
						if (std::dynamic_pointer_cast<NamedGroup, Node>(parent_.lock()->getRight())->nameOfGroup_ == this->nameOfGroup_)
							ofset = true;
					}
				}
			}
			auto action = std::make_shared<NamedGroupAction>(nameOfGroup_,ofset);
			automatOfNode->getStartState()->addEExitAction(action);
			automatOfNode->getAcceptState()->addEntranceAction(action);
		}
		static bool tableEmpty() {
			return groups->empty();
		}
		static void tableClear() {
			groups->clear();
		}
		static std::unordered_map<std::string,std::string> getTable() {
			return *groups;
		}
	};
// Узел конкатениции - бинарный оператор, симол - "concat" испольщуется только для вывода дерева на экран, как такового метасимвола конкатеницаии нет, достаточно, чтобы два завершенных узла
// стояли рядом. НКА узла - объединеие НКА левого и правого поддеревьев, склеиваются при этом конец левого и начало правого автоматов.
	class ConcatNode:public BinaryMetaNode
	{
	private:
		static const std::int8_t priority = 2;
	public:
		ConcatNode() :BinaryMetaNode() {};
		virtual NodeTypes getType() const override {
			return NodeTypes::CONCAT;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return ConcatNode::priority;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter)const  override {
			return nullptr;
		}
		virtual std::string getSimbol() const noexcept override {
			return "concat";
		}
		virtual void buildNfa()override {
			std::unique_ptr<NondeterminedFinalAutomata> leftAutom = left_->stealNodesNfa();
			std::unique_ptr<NondeterminedFinalAutomata> rightAutom = right_->stealNodesNfa();
			std::list<std::unique_ptr<Transition>>* transitions = rightAutom->getStartState()->getAllTransitions();
			for (auto i = transitions->begin(); i != transitions->end(); i++) {
				leftAutom->getAcceptState()->addTransition(std::move(*i));
			}
			leftAutom->getAcceptState()->addEntranceAction(rightAutom->getStartState()->getEntranceActions());
			leftAutom->getAcceptState()->addEExitAction(rightAutom->getStartState()->getExitActions());
			leftAutom->setAcceptState(rightAutom->getAcceptState());
			for (auto i = rightAutom->getCurrentStates().begin(); i != rightAutom->getCurrentStates().end(); i++) {
				if ((*i) != rightAutom->getStartState() && (*i) != rightAutom->getAcceptState())
					leftAutom->addStateToList(*i);
			}
			automatOfNode = std::move(leftAutom);
		}
	};

	
// Узел "или", Нка Узла - создается новое стартовое состояние из которого eps переходы ведут в старт НКА левого и правого поддеревьев, и новый конец, в который из концов левого
// и правого ведут eps переходы
	class OrNode:public BinaryMetaNode
	{
	private:
		static const std::int8_t priority = 1;
		static std::string sign;
	public:
		OrNode() :BinaryMetaNode() {}
		virtual NodeTypes getType() const override {
			return NodeTypes::OR;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<OrNode>(iter, sign);
		}
		virtual std::string getSimbol() const noexcept override {
			return OrNode::sign;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return OrNode::priority;
		}

		virtual void buildNfa()override {
			std::unique_ptr<NondeterminedFinalAutomata> leftAutom = left_->stealNodesNfa();
			std::unique_ptr<NondeterminedFinalAutomata> rightAutom = right_->stealNodesNfa();
			std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
			std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
			start->addTransition(leftAutom->getStartState());
			start->addTransition(rightAutom->getStartState());
			leftAutom->getAcceptState()->addTransition(end);
			rightAutom->getAcceptState()->addTransition(end);
			leftAutom->getAcceptState()->setAccptable(false);
			rightAutom->getAcceptState()->setAccptable(false);
			automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
			automatOfNode->getCurrentStates().insert(automatOfNode->getCurrentStates().end(), leftAutom->getCurrentStates().begin(), leftAutom->getCurrentStates().end());
			automatOfNode->getCurrentStates().insert(automatOfNode->getCurrentStates().end(), rightAutom->getCurrentStates().begin(), rightAutom->getCurrentStates().end());
		}
	};
//узел позитивного замыкания
// НКА узла - новый старт и новый конец которые соединены ерs переходами с автоматом левого ребёнка и из конца левого ребёнка в начало его автомата ведёт так же eps переход

	class PositiveClosureNode: public UnaryMetaNode
	{
	private:
		static const std::int8_t priority = 3;
		static std::string sign;
	public:
		PositiveClosureNode() :UnaryMetaNode(0) {};
		virtual NodeTypes getType() const	override {
			return NodeTypes::POSITIVE_СLOSURE;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter)const  override {
			return CrtNode<PositiveClosureNode>(iter, sign);
		}
		virtual std::string getSimbol() const noexcept override {
			return PositiveClosureNode::sign;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return PositiveClosureNode::priority;
		}
		virtual void buildNfa()override {
			std::unique_ptr<NondeterminedFinalAutomata> leftAutom = left_->stealNodesNfa();
			std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
			std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
			start->addTransition(leftAutom->getStartState());
			leftAutom->getAcceptState()->addTransition(end);
			leftAutom->getAcceptState()->addTransition(leftAutom->getStartState());
			leftAutom->setAcceptState(end);
			leftAutom->setStartState(start);
			automatOfNode = std::move(leftAutom);
		}
	};
//Опциональный узел. НКА - НКА ребёнка с eps переходом из стартовое состояние - в конечное.
	class OptionNode: public UnaryMetaNode
	{
	private:
		static const std::int8_t priority = 3;
		static std::string sign;
	public:
		OptionNode() :UnaryMetaNode(0) {};
		virtual NodeTypes getType() const	override {
			return NodeTypes::OPTION;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<OptionNode>(iter, sign);
		}
		virtual std::string getSimbol() const  noexcept override {
			return OptionNode::sign;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return OptionNode::priority;
		}
		virtual void buildNfa()override {
			//std::unique_ptr<NondeterminedFinalAutomata> leftAutom = left_->stealNodesNfa();
			//std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
			//std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
			/*
			start->addTransition(leftAutom->getStartState());
			start->addTransition(end);
			leftAutom->getAcceptState()->addTransition(end);
			leftAutom->setAcceptState(end);
			leftAutom->setStartState(start);*/
			//leftAutom->getStartState()->addTransition(leftAutom->getAcceptState());
			//start->addTransition(leftAutom->getStartState());
			//start->addTransition(leftAutom->getAcceptState());
			//leftAutom->setStartState(start);

			automatOfNode = left_->stealNodesNfa();
			automatOfNode->getStartState()->addTransition(automatOfNode->getAcceptState());
		}
	};
// "Любой символ". Нуль арный метасимвол. автомат - переход из старта в конец по пустой строке.
	class AnyNode: public SoloMetaNode
	{
	private:
		static const  std::int8_t priority = 0;
		static std::string sign;
	public:
		AnyNode() :SoloMetaNode() {};
		virtual NodeTypes getType()	const override {
			return NodeTypes::ANY;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<AnyNode>(iter, sign);
		}
		virtual std::string getSimbol() const noexcept override {
			return AnyNode::sign;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return AnyNode::priority;
		}
		virtual void buildNfa()override {
			std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
			std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
			start->addTransition(end, "\0");
			automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
		}
	};
// Узел повторения в диапазоне, границы могут быть опущены, тогда вместо левой - 0 , вместо правой - бесконечность.
	class RepeatNode: public UnaryMetaNode
	{
	private:
		static  const std::int8_t priority = 3;
		static std::string sign;
		const std::string::const_iterator eos_;
		std::pair<std::optional<std::int32_t>, std::optional<std::int32_t>> diapason_;
	public:
		RepeatNode(const std::string::const_iterator end = {}, std::pair<std::optional<std::int32_t>, std::optional<std::int32_t>> diapason = { 1,1 }) :UnaryMetaNode(0), eos_(end), diapason_(diapason) {};
		virtual NodeTypes getType() const	override {
			return NodeTypes::REPEAT;
		}
		virtual ~RepeatNode() override {}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter)  const override {
			int count = 0;
			std::string first, second;
			if (*iter == '{') {
				++iter;
				++count;
				while (iter!=eos_&&std::isdigit(*iter))
				{
					first += *iter++;
					count++;
				}
				if (iter != eos_ && *iter == ',') {
					++iter;
					++count;
					while (iter!=eos_&&std::isdigit(*iter))
					{
						second += *iter++;
						count++;
					}
					if (iter != eos_ && *iter == '}') {
						std::pair<std::optional<std::int32_t>, std::optional<std::int32_t>> dia;
						if (first.empty())
							dia.first = std::nullopt;
						else
							dia.first=std::stoi(first);
						if (second.empty())
							dia.second = std::nullopt;
						else
							dia.second = std::stoi(second);

						if (dia.first && dia.second && dia.first > dia.second) 
							throw SintaxTree_Exception("left value of repeat diapason greater than right value.");

						auto f = eos_;
						f = {};
						return std::make_unique<RepeatNode>(f, dia);
						
					}
				}
			}
			iter -= count;
			return nullptr;
		}
		virtual std::string getSimbol() const noexcept override{
			std::stringstream s{};
			s << RepeatNode::sign;
			if(diapason_.first)
				s<< diapason_.first.value();
			s << ',';
			if (diapason_.second)
				s << diapason_.second.value();
			s<< '}';
			std::string res;
			s >> res;
			return res;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return RepeatNode ::priority;
		}
		virtual void buildNfa()override {
			std::unique_ptr<NondeterminedFinalAutomata> leftAutom = left_->stealNodesNfa();
			std::unique_ptr<NondeterminedFinalAutomata> leftCopy(new NondeterminedFinalAutomata());
			*leftCopy = *leftAutom;//Копирование
			if (!diapason_.second) {
				
				std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
				std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
				start->addTransition(end);
				//if (!diapason_.first) {
				//	automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
				//	return;
				//}
				start->addTransition(leftAutom->getStartState());
				leftAutom->getAcceptState()->addTransition(end);
				leftAutom->getAcceptState()->addTransition(leftAutom->getStartState());
				leftAutom->setAcceptState(end);
				leftAutom->setStartState(start);
				automatOfNode = std::move(leftAutom);
			}
			else {
				if ((diapason_.first && diapason_.first.value() == diapason_.second.value())||(!diapason_.first&& diapason_.second.value() == 0)) {
					if (diapason_.second.value() == 0) {
						std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
						std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
						start->addTransition(end);
						automatOfNode= std::make_unique<NondeterminedFinalAutomata>(start, end);
						return;
					}
				}
				else {
					leftAutom->getStartState()->addTransition(leftAutom->getAcceptState());
					std::unique_ptr<NondeterminedFinalAutomata> leftCopy2(new NondeterminedFinalAutomata());
					*leftCopy2 = *leftAutom;
					for (int i = diapason_.first ? diapason_.first.value() + 1 : 1; i < diapason_.second.value(); i++) {
						/*
						std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
						std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
						start->addTransition(leftAutom->getStartState());
						start->addTransition(end);
						leftAutom->getAcceptState()->addTransition(end);
						leftAutom->setAcceptState(end);
						leftAutom->setStartState(start);*/

						/*std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
						start->addTransition(leftAutom->getStartState(),);
						start->addTransition(leftAutom->getAcceptState());
						leftAutom->setStartState(start);*/

						std::unique_ptr<NondeterminedFinalAutomata> addittive(new NondeterminedFinalAutomata());
						*addittive = *leftCopy2;

						std::list<std::unique_ptr<Transition>>* transitions = addittive->getStartState()->getAllTransitions();
						for (auto i = transitions->begin(); i != transitions->end(); i++) {
							leftAutom->getAcceptState()->addTransition(std::move(*i));
						}
						//leftAutom->getAcceptState()->addEntranceAction(addittive->getStartState()->getEntranceActions());
						//leftAutom->getAcceptState()->addEExitAction(addittive->getStartState()->getExitActions());
						leftAutom->setAcceptState(addittive->getAcceptState());
						for (auto i = addittive->getCurrentStates().begin(); i != addittive->getCurrentStates().end(); i++) {
							if ((*i) != addittive->getStartState() && (*i) != addittive->getAcceptState())
								leftAutom->addStateToList(*i);
						}

						//if (i == diapason_.second.value() - 1)
							//automatOfNode = std::move(leftAutom);
					}
					automatOfNode = std::move(leftAutom);
				}
			}
			if (diapason_.first&&diapason_.first.value()>0) {
				std::unique_ptr<NondeterminedFinalAutomata> firstpart(new NondeterminedFinalAutomata());
				*firstpart = *leftCopy;
				for (int i = 1; i < diapason_.first.value(); i++) {
					std::unique_ptr<NondeterminedFinalAutomata> addittive(new NondeterminedFinalAutomata());
					*addittive = *leftCopy;
					
					std::list<std::unique_ptr<Transition>>* transitions = addittive->getStartState()->getAllTransitions();
					for (auto i = transitions->begin(); i != transitions->end(); i++) {
						firstpart->getAcceptState()->addTransition(std::move(*i));
					}
					firstpart->getAcceptState()->addEntranceAction(addittive->getStartState()->getEntranceActions());
					firstpart->getAcceptState()->addEExitAction(addittive->getStartState()->getExitActions());
					firstpart->setAcceptState(addittive->getAcceptState());
					for (auto i = addittive->getCurrentStates().begin(); i != addittive->getCurrentStates().end(); i++) {
						if ((*i) !=addittive->getStartState() && (*i) != addittive->getAcceptState())
							firstpart->addStateToList(*i);
					}
				}
				if (automatOfNode != nullptr) {
				std::list<std::unique_ptr<Transition>>* transitions = automatOfNode->getStartState()->getAllTransitions();
				for (auto i = transitions->begin(); i != transitions->end(); i++) {
					firstpart->getAcceptState()->addTransition(std::move(*i));
				}
					firstpart->getAcceptState()->addEntranceAction(automatOfNode->getStartState()->getEntranceActions());
					firstpart->getAcceptState()->addEExitAction(automatOfNode->getStartState()->getExitActions());
					firstpart->setAcceptState(automatOfNode->getAcceptState());
					for (auto i = automatOfNode->getCurrentStates().begin(); i != automatOfNode->getCurrentStates().end(); i++) {
						if ((*i) != automatOfNode->getStartState() && (*i) != automatOfNode->getAcceptState())
							firstpart->addStateToList(*i);
					}
				}
				automatOfNode = std::move(firstpart);
			}
		}
	};
//Ссылка на именованную группы захвата, содержит имя группы захвата и указатель на таблицу с группами захвата. Изначально вместо перехода - абстрактная ссылка 
// если при анализе строки группа захвата уже была заполнена, то этот переход заменится на значение из этой группы при входе в старт автомата этого узла, иначе возникнет исключение.

	class LinkNamedGroup : public SoloMetaNode
	{
	private:
		std::string::const_iterator eos_;
		std::string nameOfGroup_;
		static const std::int8_t priority = 0;
		static std::string sign;
		static std::shared_ptr<std::unordered_map<std::string, std::string>> groups;
		static std::int16_t links;

		class LinkGroupAction : public Action
		{
		private:
			std::string nameofgroup_;
			std::string link;
		public:
			LinkGroupAction(std::string& name,std::string link): nameofgroup_(name),link(link){}
			virtual ~LinkGroupAction() override {};
			virtual void doAction(std::string::const_iterator& c, bool b =false,AutomataState* state = nullptr)override {
				if ((*groups).contains(nameofgroup_)) {
					std::string str = (*groups)[nameofgroup_];
					auto tmp = state->makeTransition(link);
					if (tmp) 
						state->changeCondition(link, str);
				}
				else
					throw std::logic_error("attempt to access an uninitialized capture group");
			}
		};

	public:
		LinkNamedGroup(std::string name = "",const std::string::const_iterator end = {}) :SoloMetaNode(), eos_(end), nameOfGroup_(name) {};
		virtual ~LinkNamedGroup() override {};
		virtual NodeTypes getType()	const override {
			return NodeTypes::LINK_NAMED_GROUP;
		}
		 std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			std::string nameOfGroup_ = "";
			int count = 0;
			if (*iter == '<') {
				++iter;
				++count;
				while (iter!=eos_&&(std::isalpha(*iter)||*iter=='>'))
				{
					if (*iter == '>' && count > 0) {
						
						return std::make_unique<LinkNamedGroup>(nameOfGroup_);
					}
					nameOfGroup_ += *iter;
					count++;
					iter++;
				}
			}
			iter -= count;
			return nullptr;
		}
		 virtual std::string getSimbol() const noexcept override {
			 return LinkNamedGroup::sign + nameOfGroup_ + ">";
		 }
		 virtual std::int8_t getPriority() const noexcept override {
			 return LinkNamedGroup::priority;
		 }
		 virtual bool complited() const noexcept override {
			 return groups->contains(nameOfGroup_);
		 }
		 virtual void buildNfa()override {
			 std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
			 std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
			 std::stringstream str;
			 str << nameOfGroup_ << "#lnk" << links;
			 std::string st;
			 str >> st;
			 start->addTransition(end,st);
			 alphabet->insert(st);
			 start->addEntranceAction(std::make_shared<LinkGroupAction>(nameOfGroup_,st));
			 LinkNamedGroup::links++;
			 automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
		 }
	};

	// Два метасимвола скобок, которые используются при построении дерева, после завершения построения в дереве не остается данных узлов.
	class OpenPriority: public SoloMetaNode
	{
	private:
		static  const std::int8_t priority = 0;
		static std::string sign;
	public:
		OpenPriority(bool=0) :SoloMetaNode(/*sign_,*/) {};
		virtual NodeTypes getType() const	override {
			return NodeTypes::OPEN_PRIORITY;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<OpenPriority>(iter, sign);
		}

		virtual std::string getSimbol() const  noexcept override {
			return OpenPriority::sign;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return OpenPriority::priority;
		}
		virtual void buildNfa()override {}
		virtual bool complited() const noexcept override {
			return false;
		}
	};
	class ClosePriority :public  SoloMetaNode
	{
	private:
		static const std::int8_t priority = 0;
		static std::string sign;
	public:
		ClosePriority() :SoloMetaNode() {};
		virtual NodeTypes getType()	const override {
			return NodeTypes::CLOSE_PRIORITY;
		}
		virtual std::unique_ptr<Node> createNode(std::string::const_iterator& iter)const  override {
			return CrtNode<ClosePriority>(iter, sign);
		}

		virtual std::string getSimbol() const noexcept override {
			return ClosePriority::sign;
		}
		virtual std::int8_t getPriority() const noexcept override {
			return ClosePriority::priority;
		}
		virtual void buildNfa()override {}
		virtual bool complited()const noexcept override {
			return false;
		}
	};
}
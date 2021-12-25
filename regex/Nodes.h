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
		~Leaf()override {}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override;
		NodeTypes getType() const noexcept override {
			return NodeTypes::LEAF;
		}
		bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override {
			return true;
		}
		bool complited() const noexcept override {
			return true;
		}
		std::int8_t getPriority() const noexcept override {
			return Leaf::priority;
		}
		std::string getSimbol() const  noexcept override {
			return sign_;
		}
		void buildNfa() override;
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
		~MetaNode()override {};
		std::shared_ptr<Node> getLeft() const noexcept override { return left_; }
		std::shared_ptr<Node> getRight() const noexcept override { return right_; }
	};

	class SoloMetaNode :public MetaNode
	{
	public:
		SoloMetaNode() :MetaNode() {};
		bool setChild(std::list<std::shared_ptr<Node>>::iterator, bool inverse = false) override {
			return true;
		}
		bool complited()const noexcept override {
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

		UnaryMetaNode(short int side = 0);

		bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override;
		bool complited() const noexcept override {
			return side_ == 0 ? left_ != nullptr : right_ != nullptr;
		}
	};
// бинарный оператор, является завершенным если оба ребенка не пусты
	class BinaryMetaNode : public MetaNode
	{
	public:
		BinaryMetaNode() :MetaNode() {};
		bool complited() const noexcept override {
			return left_ != nullptr && right_ != nullptr;
		}
		bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override;
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
		NodeTypes getType() const override {
			return NodeTypes::SCREENING;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<ScriningNode>(iter, sign);
		}
		std::string getSimbol() const noexcept override {
			return ScriningNode::sign;
		}
		static std::string getSign()  noexcept {
			return ScriningNode::sign;
		}
		std::int8_t getPriority() const noexcept override {
			return ScriningNode::priority;
		}
		bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override ;
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
			std::optional<std::string::const_iterator> begin;
			std::optional<std::string::const_iterator> end;
			std::string nameofgroup_;
			bool ofset;
			void doAsExit(std::string::const_iterator& currentChar) noexcept;
			void doAsEnter(std::string::const_iterator& currentChar);
		public:
			NamedGroupAction(std::string name,bool ofset=false) : nameofgroup_(name),ofset(ofset) {};
			void doAction(std::string::const_iterator& currentChar, bool isEnter = true,AutomataState* state = nullptr)override {
				if (isEnter)
					doAsEnter(currentChar);
				else
					doAsExit(currentChar);
			};
			~NamedGroupAction() override {}
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
		~NamedGroup() override {
		}
		NodeTypes getType() const override {
			return NodeTypes::NAMED_GROUP;
		}
		std::int8_t getPriority() const noexcept override {
			return NamedGroup::priority;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const  override;
		std::string getSimbol() const noexcept override {
			return NamedGroup::sign + nameOfGroup_ + ">";
		}
		bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override;
		void buildNfa() override;
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
		NodeTypes getType() const override {
			return NodeTypes::CONCAT;
		}
		std::int8_t getPriority() const noexcept override {
			return ConcatNode::priority;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter)const  override {
			return nullptr;
		}
		std::string getSimbol() const noexcept override {
			return "concat";
		}
		void buildNfa() override ;
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
		NodeTypes getType() const override {
			return NodeTypes::OR;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<OrNode>(iter, sign);
		}
		std::string getSimbol() const noexcept override {
			return OrNode::sign;
		}
		std::int8_t getPriority() const noexcept override {
			return OrNode::priority;
		}

		void buildNfa()override;
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
		NodeTypes getType() const	override {
			return NodeTypes::POSITIVE_СLOSURE;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter)const  override {
			return CrtNode<PositiveClosureNode>(iter, sign);
		}
		std::string getSimbol() const noexcept override {
			return PositiveClosureNode::sign;
		}
		std::int8_t getPriority() const noexcept override {
			return PositiveClosureNode::priority;
		}
		void buildNfa() override;
	};
//Опциональный узел. НКА - НКА ребёнка с eps переходом из стартовое состояние - в конечное.
	class OptionNode: public UnaryMetaNode
	{
	private:
		static const std::int8_t priority = 3;
		static std::string sign;
	public:
		OptionNode() :UnaryMetaNode(0) {};
		NodeTypes getType() const	override {
			return NodeTypes::OPTION;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<OptionNode>(iter, sign);
		}
		std::string getSimbol() const  noexcept override {
			return OptionNode::sign;
		}
		std::int8_t getPriority() const noexcept override {
			return OptionNode::priority;
		}
		void buildNfa()override {
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
		NodeTypes getType()	const override {
			return NodeTypes::ANY;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<AnyNode>(iter, sign);
		}
		std::string getSimbol() const noexcept override {
			return AnyNode::sign;
		}
		std::int8_t getPriority() const noexcept override {
			return AnyNode::priority;
		}
		void buildNfa()override;
	};

// Узел повторения в диапазоне, границы могут быть опущены, тогда вместо левой - 0 , вместо правой - бесконечность.
	class RepeatNode : public UnaryMetaNode
	{
	private:
		static  const std::int8_t priority = 3;
		static std::string sign;
		const std::string::const_iterator eos_;
		std::pair<std::optional<std::int32_t>, std::optional<std::int32_t>> diapason_;
	public:
		RepeatNode(const std::string::const_iterator end = {}, std::pair<std::optional<std::int32_t>, std::optional<std::int32_t>> diapason = { 1,1 }) :UnaryMetaNode(0),
			eos_(end), diapason_(diapason) {};

		NodeTypes getType() const	override {
			return NodeTypes::REPEAT;
		}
		~RepeatNode() override {}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter)  const override;
		std::string getSimbol() const noexcept override;
		std::int8_t getPriority() const noexcept override {
			return RepeatNode::priority;
		}
		void buildNfa()override;
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
			const std::string constLink;
			std::string link;

		public:
			LinkGroupAction(std::string& name,std::string link): nameofgroup_(name),link(link),constLink(link){}
			~LinkGroupAction() override {};
			void doAction(std::string::const_iterator& c, bool b = false, AutomataState* state = nullptr)override;
		};

	public:
		LinkNamedGroup(std::string name = "",const std::string::const_iterator end = {}) :SoloMetaNode(), eos_(end), nameOfGroup_(name) {};
		~LinkNamedGroup() override {};
		NodeTypes getType()	const override {
			return NodeTypes::LINK_NAMED_GROUP;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override;
		 std::string getSimbol() const noexcept override {
			 return LinkNamedGroup::sign + nameOfGroup_ + ">";
		 }
		 std::int8_t getPriority() const noexcept override {
			 return LinkNamedGroup::priority;
		 }
		bool complited() const noexcept override {
			 return groups->contains(nameOfGroup_);
		 }
		void buildNfa()override;
	};

	// Два метасимвола скобок, которые используются при построении дерева, после завершения построения в дереве не остается данных узлов.
	class OpenPriority: public SoloMetaNode
	{
	private:
		static  const std::int8_t priority = 0;
		static std::string sign;
	public:
		OpenPriority(bool=0) :SoloMetaNode(/*sign_,*/) {};
		NodeTypes getType() const	override {
			return NodeTypes::OPEN_PRIORITY;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter) const override {
			return CrtNode<OpenPriority>(iter, sign);
		}

		std::string getSimbol() const  noexcept override {
			return OpenPriority::sign;
		}
		std::int8_t getPriority() const noexcept override {
			return OpenPriority::priority;
		}
		void buildNfa()override {}
		bool complited() const noexcept override {
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
		NodeTypes getType()	const override {
			return NodeTypes::CLOSE_PRIORITY;
		}
		std::unique_ptr<Node> createNode(std::string::const_iterator& iter)const  override {
			return CrtNode<ClosePriority>(iter, sign);
		}

		std::string getSimbol() const noexcept override {
			return ClosePriority::sign;
		}
		std::int8_t getPriority() const noexcept override {
			return ClosePriority::priority;
		}
		void buildNfa()override {}
		bool complited()const noexcept override {
			return false;
		}
	};
}
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
// ������������ ����� �����, ����� ����� ������ ���������� ��� ������ �� ��������� typeId
	enum class NodeTypes {
		LEAF,
		SCREENING,
		OR,
		CONCAT,
		POSITIVE_�LOSURE,
		OPTION,
		ANY,
		REPEAT,
		NAMED_GROUP,
		LINK_NAMED_GROUP,
		OPEN_PRIORITY,
		CLOSE_PRIORITY
	};
//������ ������� ��������� ���� �� ������� ������������� ������ � "�����������" ���������������� ������� ���� ����
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
// ����������� ����� ���� ������, ����� ��������� �� �������, �� ������� ���� (���� �� ������ � �� ������� ������ �������� � �������� ����), ��������� �� ��������
// �� ����� ���������� ��������� ������ ����, � ������ ������:
// ������� ���� �� ������� (� ������ �� �������� ��������� ������������� ������)
// �������� ��� ���� (��� ������� ��� ������������ typeid)
// ���������� ����� �� ��������� �� ������ ���� � ������ �����, � ������������ ������ �������� (��� �������� �����)
// ����� ������� ��������� ����������, �������� �� ���� �����������
// ��������� ��� ���� � ���������� ��������
// �������� ��������� �� ����� � ������ ���������
// "�������" ��������� �� ��� ���� (������ ��� ��� ������� ��� ������ ���� ������������ ������ 1 ���, ������� ���� ����� ������ ���� ��������� ���� �������, ������� ��� ������
// � �� ������� ��� ������)

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

//����� ����� ���������� ����, ������� ������ ����� ������ ������, ��� ������ ������ �������� � �� ����������, ���� ������ �� �� ��� �����������
//���� ���� ������ ����������� ������ ��� � ���� �� ����� ���� ����� � �� ������� ���� ��������
// ��� ������� ���� - ������ ��� ���������, � ��������� �� 1 �� 2 �� ������� �����
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
// ����� �����������, ����������� �� ����, ������ ��������� �� ����� � ������ ���������.
// � ����� ������ ������������ ��� ����-������ ���������, �� ���� ����, � �������� ��� �����, ������� ��� �� �������� ������ �����������
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
// ����� �������� �����������, � �������� ���� ������� �������� 0- ����, 1 - �����, ����� ��������� ����� ������������� ����
// � ��������������� ������� ��������� ��� ��������� � ��������������� �������, � ������ ������� ���������  null
// �������� �����������, ���� ������ ������� ���������
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
// �������� ��������, �������� ����������� ���� ��� ������� �� �����
	class BinaryMetaNode : public MetaNode
	{
	public:
		BinaryMetaNode() :MetaNode() {};
		bool complited() const noexcept override {
			return left_ != nullptr && right_ != nullptr;
		}
		bool setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse = false) override;
	};
// ���������� �������������, ������ �������� ��������������� ���������, ����� ���� ������ �� ���� ���������� � �������� ���� ������� ����� � �������� ������������� ����.
// ��� ��� ������� ���� - ������ ��� ��� ������ - �����
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
// ���������� ����������� ������ �������, �������������� ��������
// �������� ���� - ��� ������ (������ ���� ���������, ����� ����� ������� ����������)
// ����������� ���� - ��������� �� ������� �� ����� ������� ����� � ���������������� ��� ��������
// ������������� �������� ����� ������ �� ����������� ������.
// ���������� ����� ������� ��������� ��������, ��� �������������� ������ � ����� ������ ������� � ������ ������ �� ����������������� ����� � �������, ��� ������� ������ �� ����������� ���������
// ������� ���� - ������� ��� ������, �� � ������ � ����� �������� ��������� ������� �������������� ������ ������ ������� (��� ��������� �������-�������)
// ����� ����������� ������ ��� ������ � �������� ����� (�������� � ���������)
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
// ���� ������������ - �������� ��������, ����� - "concat" ������������ ������ ��� ������ ������ �� �����, ��� �������� ����������� ������������� ���, ����������, ����� ��� ����������� ����
// ������ �����. ��� ���� - ���������� ��� ������ � ������� �����������, ����������� ��� ���� ����� ������ � ������ ������� ���������.
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

	
// ���� "���", ��� ���� - ��������� ����� ��������� ��������� �� �������� eps �������� ����� � ����� ��� ������ � ������� �����������, � ����� �����, � ������� �� ������ ������
// � ������� ����� eps ��������
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
//���� ����������� ���������
// ��� ���� - ����� ����� � ����� ����� ������� ��������� ��s ���������� � ��������� ������ ������ � �� ����� ������ ������ � ������ ��� �������� ���� ��� �� eps �������

	class PositiveClosureNode: public UnaryMetaNode
	{
	private:
		static const std::int8_t priority = 3;
		static std::string sign;
	public:
		PositiveClosureNode() :UnaryMetaNode(0) {};
		NodeTypes getType() const	override {
			return NodeTypes::POSITIVE_�LOSURE;
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
//������������ ����. ��� - ��� ������ � eps ��������� �� ��������� ��������� - � ��������.
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
// "����� ������". ���� ����� ����������. ������� - ������� �� ������ � ����� �� ������ ������.
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

// ���� ���������� � ���������, ������� ����� ���� �������, ����� ������ ����� - 0 , ������ ������ - �������������.
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

//������ �� ����������� ������ �������, �������� ��� ������ ������� � ��������� �� ������� � �������� �������. ���������� ������ �������� - ����������� ������ 
// ���� ��� ������� ������ ������ ������� ��� ���� ���������, �� ���� ������� ��������� �� �������� �� ���� ������ ��� ����� � ����� �������� ����� ����, ����� ��������� ����������.

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

	// ��� ����������� ������, ������� ������������ ��� ���������� ������, ����� ���������� ���������� � ������ �� �������� ������ �����.
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
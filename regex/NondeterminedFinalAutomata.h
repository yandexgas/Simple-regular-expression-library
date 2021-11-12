#pragma once
#include <optional>
#include <variant>
#include <iostream>
#include <ranges>
#include <algorithm>
#include<string>
#include <list>
#include <unordered_map>
#include<sstream>
#include<unordered_set>
namespace regex {

	class Transition;
	class Action;
	class AutomataState;

// ����� ������� ���������, ����� ����� doAction, � ������� �������� ��������� �� ������� ��������� ��������, �������� ������������� ������ �� ������ ���������
// � ������� ���������� � ���, � ����� ������ ��������� ��������
	class Action {
	public:
		virtual void doAction(std::string::const_iterator&, bool b = true, AutomataState* state = nullptr) {}
		virtual ~Action() {};
		virtual int order() { return 1; }
	};

// ����� �������� (������������ ������ ��� ���, ��� ��� ���� ������� ��� �����������, ����� ���������� ��� eps ���������)
// �������� ��������� �� ������� ��������� �������� � ����������� ������� ��������, ���� ��� nullopt, ���������� eps �������
// �������� ����������� �� �������� �������� � ����������� ������� ��������
// ����� �������� �� �������, ���������� ����������� ����� ��������� (nullopt ���� �������� �� ����� ������� ���). ��� �������� �������� ExitAction and Entrance Action ���������
// � �������� ��������� �������������� (���� ������� ��������)

	class Transition {
	private:
		std::weak_ptr<AutomataState> targetState;
		std::optional<std::string> condition;
	public:
		Transition(std::shared_ptr<AutomataState>target, std::optional<std::string> condition = {}) :targetState(target), condition(condition) {}
		~Transition() {}
		std::optional<std::shared_ptr<AutomataState>> makeTransition(std::string::const_iterator*);
		void setCondition(std::string str) {
			condition = str;
		}
		const std::optional<std::string>& getCondition() {
			return condition;
		}
		std::shared_ptr<AutomataState> getTargetState() {
			return targetState.lock();
		}
	};

//����� ��������� ��������
// ������ ������ ��������� (������ � ���, �.� ��� ��� ����� ���������� ������������� eps ��������� ������������ ��� ������� - ������ - ����� ���������"
// ����� �������, ������� "�����������" ��������� ��� ��� � ������ ������� � �������� �������.
// ����� make transition ����� 2 ���������� - ������ ��������� ������� �� �������, ���� ������� ���������� �������� � ��������� ��������� 
// ������ - ���������� ������� ��������� �� ��������� �������, �� �������� ������� � �������� ������� ����� ���������

	class AutomataState {
	private:
		std::list<std::unique_ptr<Transition>> transitions_;
	protected:
		unsigned short number;
		bool acceptable_ = false;
		std::unordered_set<std::shared_ptr<Action>> entranceAction_;
		std::unordered_set<std::shared_ptr<Action>> exitAction_;

	public:
		AutomataState(bool isAcceptable = false) : acceptable_(isAcceptable), number(0) {}
		//AutomataState(std::list<std::unique_ptr<Transition>>&& trans, bool isAcceptable=false) : transitions_(trans), acceptable_(isAcceptable) {}
		virtual ~AutomataState() {}
		virtual AutomataState& addTransition(std::unique_ptr<Transition> transition) {
			transitions_.emplace_back(std::move(transition));
			return *this;
		}
		inline AutomataState& setNumber(unsigned short t) {
			number = t;
			return *this;
		}
		inline const unsigned short getNumber() {
			return number;
		}
		virtual AutomataState& addTransition(std::shared_ptr<AutomataState>target, std::optional<std::string> condition = {}) {
			transitions_.emplace_back(std::make_unique<Transition>(target, condition));
			return *this;
		}
		inline AutomataState& addEntranceAction(std::shared_ptr<Action> action) { 
			entranceAction_.insert(action);
			return *this;
		}
		inline AutomataState& addEntranceAction(std::unordered_set<std::shared_ptr<Action>>& action) {
			if (!action.empty())
				entranceAction_.insert(action.begin(), action.end());
			return *this;
		}
		inline AutomataState& addEExitAction(std::shared_ptr<Action> action) {
			exitAction_.insert(action);
			return *this;
		}
		inline AutomataState& addEExitAction(std::unordered_set<std::shared_ptr<Action>>& action) {
			if (!action.empty())
				exitAction_.insert(action.begin(), action.end());
			return *this;
		}
		virtual std::shared_ptr <AutomataState> makeTransition(std::string::const_iterator* c) {
			std::optional<std::shared_ptr<AutomataState>> res = std::nullopt;
			for (auto i = transitions_.begin(); i != transitions_.end(); i++) {
				res = (**i).makeTransition(c);
				if (res) {
					doExit(*c);
					res.value()->doEnter(*c);
					return res.value();
				}
			}
			return nullptr;
		}
		virtual std::shared_ptr <AutomataState> makeTransition(std::string str, bool exp=false) {
			for (auto i = transitions_.begin(); i != transitions_.end(); i++) {
				if ((**i).getCondition() == str) {
					return (**i).getTargetState();
				}
			}
			return nullptr;
		}
		AutomataState& makeTransition(std::shared_ptr<AutomataState>&);
		virtual std::list<std::unique_ptr<Transition>>* getAllTransitions() {
			return &transitions_;
		}
		virtual void changeCondition(std::string,std::string) {}
		inline std::unordered_set<std::shared_ptr<Action>>& getEntranceActions() {
			return entranceAction_;
		}
		inline std::unordered_set<std::shared_ptr<Action>>& getExitActions() {
			return exitAction_;
		}

		inline bool isAcceptable() {
			return acceptable_;
		}
		inline AutomataState& setAccptable(bool acceptable) {
			acceptable_ = acceptable;
			return*this;
		}
		void doEnter(std::string::const_iterator& c);
		void doExit(std::string::const_iterator& c);
	};
	
// ����������� ����� ��������� �������, ���������� ��������� �� ��������� ��������� � �� �������� ��������� (����������, ������������� ������ ��� ���, ��� ������ �������� �� �� ������ ����� ������)
	class FinalAutomata {
	protected:
		std::shared_ptr<AutomataState> startState_;
		std::shared_ptr<AutomataState> acceptableState_;
		FinalAutomata() {};
	public:
		FinalAutomata(std::shared_ptr<AutomataState> start, std::shared_ptr<AutomataState> accept) noexcept : startState_(start), acceptableState_(accept) {}
		virtual void makeTransition(std::string) = 0;
		inline std::shared_ptr<AutomataState> getStartState() {
			return startState_;
		}
		inline std::shared_ptr<AutomataState> getAcceptState() {
			return acceptableState_;
		}
		virtual void setStartState(std::shared_ptr<AutomataState> start) = 0;
		virtual void setAcceptState(std::shared_ptr<AutomataState> state) = 0;

	};

// ����� ��������� ��������. (����������� ������� �� ��������, ������������ ��� �������������� ������� �������� � ���). �������� ������ ���� ���������
// ���������� ������ ��� �������������� � ��� ������� ����������� ���������.

	class NondeterminedFinalAutomata : public FinalAutomata
	{
	private:
		std::list<std::shared_ptr<AutomataState>> currentStates_;
		
	public:
		NondeterminedFinalAutomata() {}
		NondeterminedFinalAutomata(std::shared_ptr<AutomataState> start, std::shared_ptr<AutomataState> accept) noexcept : FinalAutomata(start,accept) {
			currentStates_.push_back(startState_);
			currentStates_.push_back(acceptableState_);
		}
		~NondeterminedFinalAutomata(){}
		std::list<std::shared_ptr<AutomataState>>& getCurrentStates() {
			return currentStates_;
		}
		NondeterminedFinalAutomata& addStateToList(std::shared_ptr<AutomataState> state) {
			currentStates_.push_back(state);
			return *this;
		}
		// �� ��������������
		virtual void makeTransition(std::string) override {
			auto end = currentStates_.end();
			for (auto i = currentStates_.begin(); i != end; i++) {
			}
		} 
		virtual void setStartState(std::shared_ptr<AutomataState> start)override {
			if (start->isAcceptable())
				throw std::logic_error("For algorithm, which hab been used in this lib, start state can't be acceptable state");
			else {
				startState_ = start;
				currentStates_.push_back(startState_);
			}
		}
		virtual void setAcceptState(std::shared_ptr<AutomataState> state)override {
			if (!state->isAcceptable())
				throw std::logic_error("State must be acceptable");
			else {
				acceptableState_->setAccptable(false);
				acceptableState_ = state;
				currentStates_.push_back(acceptableState_);
			}
		}

		NondeterminedFinalAutomata& operator=(const NondeterminedFinalAutomata& source) {
			std::unordered_map<std::shared_ptr<AutomataState>, std::shared_ptr<AutomataState>> unordered_map;
			std::list<std::shared_ptr<AutomataState>> newStates;
			for (auto i = source.currentStates_.begin(); i != source.currentStates_.end(); i++) {
				if (!unordered_map.contains(*i)) {
					unordered_map.emplace(*i, std::make_shared<AutomataState>((**i).isAcceptable()));
					unordered_map[*i]->getEntranceActions() = (*i)->getEntranceActions();
					unordered_map[*i]->getExitActions() = (*i)->getExitActions();
				}
				currentStates_.emplace_back(unordered_map[*i]);
				for (auto j = ((*i)->getAllTransitions())->begin(); j != ((*i)->getAllTransitions())->end(); j++) {
					if (!unordered_map.contains((*j)->getTargetState())) {
						unordered_map.emplace((*j)->getTargetState(), std::make_shared<AutomataState>((*j)->getTargetState()->isAcceptable()));
						unordered_map[(*j)->getTargetState()]->getEntranceActions()= (*j)->getTargetState()->getEntranceActions();
						unordered_map[(*j)->getTargetState()]->getExitActions() = (*j)->getTargetState()->getExitActions();
					}
					unordered_map[*i]->addTransition(unordered_map[(*j)->getTargetState()], (*j)->getCondition());
				}
			}
			startState_ = unordered_map[source.startState_];
			acceptableState_ = unordered_map[source.acceptableState_];
			return *this;
		}
	};
}
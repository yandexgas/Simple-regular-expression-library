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

// Класс события состояния, имеет метод doAction, в который подается указатель на текущее состояние автомата, итератор анализируемой строки на данном состяонии
// и булевая переменная о том, в каком режиме выполнять действие
	class Action {
	public:
		virtual void doAction(std::string::const_iterator&, bool b = true, AutomataState* state = nullptr) {}
		virtual ~Action() {};
	};

// Класс перехода (используется только для НКА, для ДКА была сделана его оптимизация, ввиду отсутствия там eps переходов)
// Содержит указатель на целевое состояние перехода и опционально условие перехода, если оно nullopt, обозначает eps переход
// Содердит конструктор из целевого состяния и опционально условия перехода
// Метод перехода по символу, возвращает опционально новое состояние (nullopt если перехода по этому символу нет). При переходе вызывает ExitAction and Entrance Action исходного
// и целевого состояний соответственно (если переход возможен)

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
		const std::optional<std::string>& getCondition() const {
			return condition;
		}
		std::shared_ptr<AutomataState> getTargetState() const {
			return targetState.lock();
		}
	};

//Класс состояния автомата
// Хранит список переходов (только в НКА, т.к для ДКА ввиду отсутствия дублирующихся eps переходов используется хэш таблица - символ - новое состояние"
// номер вершины, условие "принимающее" состояние или нет и списки входных и выходных событий.
// Метод make transition имеет 2 перегрузки - первая выполняет переход по символу, если переход невозможен попадает в тупиковое состояние 
// вторая - возвращает целевое состояние по заданному символу, не выполняя входные и выходные события этого состояния

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
		inline const unsigned short getNumber() const {
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
		virtual std::shared_ptr <AutomataState> makeTransition(std::string::const_iterator* c);
		virtual std::shared_ptr <AutomataState> makeTransition(std::string str, bool exp = false);
		//AutomataState& makeTransition(std::shared_ptr<AutomataState>&);
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
	
// абстрактный Класс конечного автомта, содержащий указатель на стартовое состояние и на конечное состояние (устаревший, использеуется только для НКА, ДКА теперь построен не на основе этого класса)
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

// Класс конечного автомата. (возможности урезаны до минимума, необходимого для преобразования данного автомата в ДКА). Содержит список всех состояний
// необходимо только для преобразование в ДКА методом объединения состояний.

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
		// не поддерживается
		void makeTransition(std::string) override {
			auto end = currentStates_.end();
			for (auto i = currentStates_.begin(); i != end; i++) {
			}
		} 
		void setStartState(std::shared_ptr<AutomataState> start)override;
		void setAcceptState(std::shared_ptr<AutomataState> state)override;

		NondeterminedFinalAutomata& operator=(const NondeterminedFinalAutomata& source);
	};
}
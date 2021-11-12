#pragma once
#include "Nodes.h"
#include <unordered_set>
#include <vector>

namespace regex {


// Класс состояния ДКА, оптимизированная версия класса состояния автомата, не требуется класс Transition для хранения переходов, они все уникальны
// поэтому для быстрого доступа используется unordered map
// булевая переменная, имеет ли это состояние ссылку на группу захвата
// Методы польностью совместимы с методами обычного состояния автомата, для обратной совместимости метод GetAllTransitions из
// таблицы переходов генерирует список Transitions создавая новые объекты, к сожалению ответственность за удаление этого списка лежит на объекте, которые вызвал этот метод.

	class DfaState : public AutomataState {
	private:
		std::unordered_map<std::string, std::weak_ptr<AutomataState>> transitions_;
		bool hasLinkGroup = false;
	public:
		DfaState (bool isAcceptable = false) : AutomataState(isAcceptable) {}
		~DfaState()override {};
		virtual AutomataState& addTransition(std::unique_ptr<Transition> transition)override {
			transitions_[transition->getCondition().value()] = transition->getTargetState();
			if (transition->getCondition().value().size() > 4)
				hasLinkGroup = true;
			return *this;
		}
		virtual AutomataState& addTransition(std::shared_ptr<AutomataState>target, std::optional<std::string> condition = {})override {
			if(!transitions_.contains(condition.value()))
			transitions_[condition.value()] = target;
			if (condition.value().size() > 4)
				hasLinkGroup = true;
			return *this;
		}
		virtual std::shared_ptr <AutomataState> makeTransition(std::string::const_iterator* c)override {
			std::string str(1, **c);
			if (transitions_.contains(str)) {
				doExit(*c);
				auto t = transitions_[str].lock();
				t->doEnter(*c);
				return t;
			}
			else {
				if (hasLinkGroup) {
					for (auto i = transitions_.begin(); i != transitions_.end(); i++) {
						if ((*i).first.size() > 1) {
							auto tmp = *c;
							try {
								bool ret = true;
								for (int j = 0; j < (*i).first.size(); j++) {
									if ((*i).first[j] != *tmp) {
										ret = false;
										break;
									}
									tmp++;
								}
								if (ret) {
									doExit(*c);
									*c = --tmp;
									auto t = (*i).second.lock();
									t->doEnter(*c);
									return t;
								}
							}
							catch (...) {}
						}
					}
				}
				if (transitions_.contains("")) {
					doExit(*c);
					auto t = transitions_[""].lock();
					t->doEnter(*c);
					return t;
				}
				else return nullptr;
			}
		}
		virtual std::shared_ptr <AutomataState> makeTransition(std::string str, bool exp=false) override {
			if (transitions_.contains(str))
				return transitions_[str].lock();
			else if(!exp)
			return nullptr;
			else {
				if (str.size() == 1) {
					if (transitions_.contains(""))
						return transitions_[""].lock();
				}
			}
			return nullptr;
		}
		virtual std::list<std::unique_ptr<Transition>>* getAllTransitions() override {
			std::list<std::unique_ptr<Transition>>* lst = new std::list<std::unique_ptr<Transition>>();
			for (auto i = transitions_.begin(); i != transitions_.end(); i++) {
				lst->push_back(std::make_unique<Transition>(i->second.lock(), i->first));
			}
			return lst;
		}
		virtual void changeCondition(std::string current, std::string comming)override {
			auto tmp = transitions_[current];
			transitions_.erase(current);
			transitions_[comming] = tmp;
		}
	};
// концепт объекта указателя на итерируемый объект
	template<class T>
	concept Itarable =
		requires (T a) {
		a->begin();
	};
//Класс ДКА
// явно хранит количество состояний, указатель на начало, вектор принимающийх и непринимающих состояний (после минимизации последний - очищается), и указатели на все состояния.
// для реализации перестройки ДКА из НКА методы eps- замыкания для одного сосотяния и для множества состояний
// а так же замыкания по символу(строке) для группы состояний
// метод объединения состояний который возвращает указатель на новое состояние являеющееся объединением старых (переходы в этом случае теряются, сохраняется статус принимающего и 
// входные и выходные события всех входящих состояний.
// метод минимизации (для ускорения требует обязательно подавать множество всех символов, по которым могут быть переходы в дка)
// метод построения K_пути(с памятью), принимает указатель на кэш для ускорения работы и минимизации количества рекурсий, множество метасимволов алфавита (чтобы экранировать их)
// вершины и номер пути между ними
// метод проверки единичной строки, проверки эквивалентности двух состояний (если они представлены 2 разными обхектами)
// восстановление регулярного выражения
// поиск всех подстрок подходящих под регулярное выражение
// оператор умножения автоматов (принимающие состояния назначаются так, что получаемый автомат - соотвтетствует разности 2-х языков)

// Данный класс является самостоятельным и самодостаточным, поэтому с ним можно работать напрямую в том числе в задачах не относящихся к анализу регулярных выражений
// можно определять и добавлять события для состояний получая, некотроый аналог StateMachine Compiler, однако для работы с регулярками лучше использовать его обёртка в виде 
// класса Regular_expression

	class DeterminedFinalAutomat
	{
	private:
		unsigned short states_count_;
		std::shared_ptr<AutomataState> start_;
		std::vector<std::shared_ptr<AutomataState>> acceptStates_;
		std::vector<std::shared_ptr<AutomataState>> nonacceptStates_;
		std::vector<std::shared_ptr<AutomataState>> allStates;
		std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> epsilonClosure(std::shared_ptr<AutomataState>);
		std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> epsilonClosure(std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>);
		std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> charClosure(std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>, const std::string);
		template<Itarable T>
		std::shared_ptr<AutomataState>  mergeStates(T src) {
			std::shared_ptr<AutomataState> result = std::make_shared<DfaState>();
			for (auto i = src->cbegin(); i != src->cend(); i++) {
				result->addEntranceAction((**i).getEntranceActions());
				result->addEExitAction((**i).getExitActions());
				if ((**i).isAcceptable()) {
					result->setAccptable(true);
				}
			}
			return result;
		}
		std::shared_ptr<AutomataState>  mergeStates(std::shared_ptr<AutomataState>, std::shared_ptr<AutomataState>) const noexcept;
		void minimization(const std::unordered_set<std::string>&);
		std::string k_path(std::optional<std::string >*** memoryMatrix, std::unordered_set<std::string>&,std::shared_ptr<AutomataState> i, std::shared_ptr<AutomataState> j, unsigned short k) const;
	public:
		DeterminedFinalAutomat(std::unique_ptr<NondeterminedFinalAutomata>,const std::unordered_set<std::string>&, bool minim=true);
		DeterminedFinalAutomat(DeterminedFinalAutomat& obj) :states_count_(obj.states_count_), start_(obj.start_),acceptStates_(obj.acceptStates_), allStates(obj.allStates) {}
		DeterminedFinalAutomat() :states_count_(0), start_(nullptr) {}
		bool checkString(std::string) const;
		static bool statesEquals(std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>, std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>);
		std::string regexRecover() const;
		std::vector<std::string> findAll(std::string, std::vector<std::unordered_map<std::string, std::string>>* groups= nullptr) const;

		DeterminedFinalAutomat& operator=(DeterminedFinalAutomat&& obj) noexcept;
		DeterminedFinalAutomat& operator=(DeterminedFinalAutomat& obj) noexcept;
		DeterminedFinalAutomat operator*(const DeterminedFinalAutomat& obj) const noexcept;


	};

}
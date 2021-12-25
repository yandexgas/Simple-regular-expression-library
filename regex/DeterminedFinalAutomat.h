#pragma once
#include "Nodes.h"
#include <unordered_set>
#include <vector>

namespace regex {


// ����� ��������� ���, ���������������� ������ ������ ��������� ��������, �� ��������� ����� Transition ��� �������� ���������, ��� ��� ���������
// ������� ��� �������� ������� ������������ unordered map
// ������� ����������, ����� �� ��� ��������� ������ �� ������ �������
// ������ ���������� ���������� � �������� �������� ��������� ��������, ��� �������� ������������� ����� GetAllTransitions ��
// ������� ��������� ���������� ������ Transitions �������� ����� �������, � ��������� ��������������� �� �������� ����� ������ ����� �� �������, ������� ������ ���� �����.

	class DfaState : public AutomataState {
	private:
		std::unordered_map<std::string, std::weak_ptr<AutomataState>> transitions_;
		bool hasLinkGroup = false;
	public:
		DfaState (bool isAcceptable = false) : AutomataState(isAcceptable) {}
		~DfaState()override {};
		AutomataState& addTransition(std::unique_ptr<Transition> transition)override;
		AutomataState& addTransition(std::shared_ptr<AutomataState>target, std::optional<std::string> condition = {})override;
		std::shared_ptr <AutomataState> makeTransition(std::string::const_iterator* c)override;
		std::shared_ptr <AutomataState> makeTransition(std::string str, bool exp = false) override;
		std::list<std::unique_ptr<Transition>>* getAllTransitions() override;
		void changeCondition(std::string current, std::string comming)override;
	};
// ������� ������� ��������� �� ����������� ������
	template<class T>
	concept Itarable =
		requires (T a) {
		a->begin();
	};
//����� ���
// ���� ������ ���������� ���������, ��������� �� ������, ������ ������������ � ������������� ��������� (����� ����������� ��������� - ���������), � ��������� �� ��� ���������.
// ��� ���������� ����������� ��� �� ��� ������ eps- ��������� ��� ������ ��������� � ��� ��������� ���������
// � ��� �� ��������� �� �������(������) ��� ������ ���������
// ����� ����������� ��������� ������� ���������� ��������� �� ����� ��������� ����������� ������������ ������ (�������� � ���� ������ ��������, ����������� ������ ������������ � 
// ������� � �������� ������� ���� �������� ���������.
// ����� ����������� (��� ��������� ������� ����������� �������� ��������� ���� ��������, �� ������� ����� ���� �������� � ���)
// ����� ���������� K_����(� �������), ��������� ��������� �� ��� ��� ��������� ������ � ����������� ���������� ��������, ��������� ������������ �������� (����� ������������ ��)
// ������� � ����� ���� ����� ����
// ����� �������� ��������� ������, �������� ��������������� ���� ��������� (���� ��� ������������ 2 ������� ���������)
// �������������� ����������� ���������
// ����� ���� �������� ���������� ��� ���������� ���������
// �������� ��������� ��������� (����������� ��������� ����������� ���, ��� ���������� ������� - �������������� �������� 2-� ������)

// ������ ����� �������� ��������������� � ���������������, ������� � ��� ����� �������� �������� � ��� ����� � ������� �� ����������� � ������� ���������� ���������
// ����� ���������� � ��������� ������� ��� ��������� �������, ��������� ������ StateMachine Compiler, ������ ��� ������ � ����������� ����� ������������ ��� ������ � ���� 
// ������ Regular_expression

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
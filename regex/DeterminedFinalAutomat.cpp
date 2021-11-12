#include "DeterminedFinalAutomat.h"
#include <vector>
#include <stack>
#include <ranges>
namespace regex {
	int containState(std::vector<std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>>& src, std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> find) {
		size_t j = 0;
		for (auto i = src.cbegin(); i != src.cend(); i++,j++) {

			if(DeterminedFinalAutomat::statesEquals(*i,find))
				break;
		}
		return j >= src.size()? - 1:j;
	}

	DeterminedFinalAutomat::DeterminedFinalAutomat(std::unique_ptr<NondeterminedFinalAutomata> nfa, const std::unordered_set<std::string>& alphabet, bool minim): states_count_(0) {
		std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> start = epsilonClosure(nfa->getStartState());
		start_ = mergeStates(start);
		std::vector<std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>> stateGroups_toProcess;
		stateGroups_toProcess.push_back(start);
		std::vector<std::list<std::pair<std::string, int>>> transitionTable;

		for (size_t i = 0; i < stateGroups_toProcess.size(); i++) {
			transitionTable.push_back({});
			std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> current;
			for (auto j = alphabet.cbegin(); j != alphabet.cend(); j++) {
				current = epsilonClosure(charClosure(stateGroups_toProcess[i], *j));
				if (current != nullptr) {
					int index;
					if (index = containState(stateGroups_toProcess, current), index < 0) {
						index = stateGroups_toProcess.size();
						stateGroups_toProcess.push_back(current);
					}
					std::pair<std::string, int> transit(*j, index);
					transitionTable[i].push_back(transit);
				}
			}
		}
		std::list<std::shared_ptr<Transition>> trans;
		allStates.push_back(start_);

		for (size_t i = 1; i < stateGroups_toProcess.size(); i++)
			allStates.push_back(mergeStates(stateGroups_toProcess[i]));

		for (size_t i = 0; i < allStates.size(); i++) {
			std::unique_ptr<Transition>delayed = nullptr;
			for (auto j = transitionTable[i].cbegin(); j != transitionTable[i].cend(); j++) {
					allStates[i]->addTransition(allStates[j->second], j->first);	
			}
			if (allStates[i]->isAcceptable()&& allStates[i]!=start_)
				acceptStates_.push_back(allStates[i]);
			else if ((allStates[i] != start_)||!start_->isAcceptable())
			{
				nonacceptStates_.push_back(allStates[i]);
			}
		}
		if(minim)
			minimization(alphabet);
	}

	std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> DeterminedFinalAutomat::epsilonClosure(std::shared_ptr<AutomataState> root) {
		std::stack<std::shared_ptr<AutomataState>> plan;
		std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> used = std::make_shared<std::unordered_set<std::shared_ptr<AutomataState>>>();
		if (root != nullptr) {
			plan.push(root);
			while (!plan.empty()) {
				auto tmp = plan.top();
				plan.pop();
				if (!used->contains(tmp)) {
					used->insert(tmp);
					auto transits = tmp->getAllTransitions();
					for (auto i = transits->begin(); i != transits->end(); i++) {
						if (!(**i).getCondition()) {
							plan.push((**i).getTargetState());
						}
					}
				}
			}
		}
		return used->empty()?nullptr: used;
	}

	std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>  DeterminedFinalAutomat::epsilonClosure(std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> rootSet) {
		std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> result = std::make_shared<std::unordered_set<std::shared_ptr<AutomataState>>>();
		if (rootSet != nullptr) {
			for (auto i = rootSet->begin(); i != rootSet->end(); i++) {
				auto tmp = epsilonClosure(*i);
				for (auto j = tmp->begin(); j != tmp->end(); j++)
					result->insert(*j);
			}
		}
		return result->empty() ? nullptr : result;
	}

	std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>>  DeterminedFinalAutomat::charClosure(std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> rootSet, const std::string condition) {
		std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> result = std::make_shared<std::unordered_set<std::shared_ptr<AutomataState>>>();
		if (rootSet != nullptr) {
			for (auto i = rootSet->begin(); i != rootSet->end(); i++) {
				auto tmp = (**i).getAllTransitions();
				for (auto j = tmp->begin(); j != tmp->end(); j++) {
					if ((*j)->getCondition() == condition || (*j)->getCondition() == "")
						result->insert((*j)->getTargetState());
				}
			}
		}
		return result->empty() ? nullptr :result;
	}

	bool  DeterminedFinalAutomat::statesEquals(std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> fst, std::shared_ptr<std::unordered_set<std::shared_ptr<AutomataState>>> scd) {
		
		if (fst->size() != scd->size())
			return false;
		for (auto i = fst->cbegin(); i != fst->cend(); i++) {
			if (!scd->contains(*i))
				return false;
		}
		return true;
	}

	

	bool DeterminedFinalAutomat :: checkString(std::string str) const {
		NamedGroup::tableClear();
		auto current = start_;
		auto c = str.cbegin();
		current->doEnter(c);
		for (auto c = str.cbegin(); c != str.cend(); c++) {
			current = current->makeTransition(&c);
			if (current == nullptr)
				return false;
		}
		return current->isAcceptable();
	}

	void DeterminedFinalAutomat::minimization(const std::unordered_set<std::string>& alphabet) {
		using namespace std;
		vector<shared_ptr<vector<shared_ptr<AutomataState>>>> groups;
		shared_ptr<vector<shared_ptr<AutomataState>>> copy = make_shared<vector<shared_ptr<AutomataState>>>(acceptStates_);
		groups.push_back(copy);
		copy = make_shared<vector<shared_ptr<AutomataState>>>(nonacceptStates_);
		groups.push_back(copy);
		if (start_->isAcceptable()) {
			copy = make_shared<vector<shared_ptr<AutomataState>>>();
			copy->push_back(start_);
			groups.push_back(copy);
		}
		vector<shared_ptr<vector<shared_ptr<AutomataState>>>> newGroups;
		std::unordered_map<shared_ptr<AutomataState>, shared_ptr<vector<shared_ptr<AutomataState>>>> groupsTable;
		for (auto i = groups.begin(); i != groups.end(); i++) {
			for (auto j = (**i).begin(); j != (**i).end(); j++) {
				groupsTable[*j] = *i;
			}
		}
		while (newGroups.size() != groups.size()) {
			std::unordered_map<shared_ptr<AutomataState>, shared_ptr<vector<shared_ptr<AutomataState>>>> newgroupsTable;
			if (!newGroups.empty())
				groups = std::move(newGroups);
			for (size_t i = 0; i < groups.size(); i++) {
				if (groups[i]->size() == 1) {
					newGroups.push_back(groups[i]);
					auto fst = (*groups[i])[0];
					if (!newgroupsTable.contains(fst)) {
						newgroupsTable[fst] = groups[i];
					}
					continue;
				}
				for (size_t j = 0; j < groups[i]->size(); j++) {
					auto fst = (*groups[i])[j];
					if (!newgroupsTable.contains(fst)) {
						std::shared_ptr<std::vector<std::shared_ptr<AutomataState>>> tmp = std::make_shared<std::vector<std::shared_ptr<AutomataState>>>();
						tmp->push_back(fst);
						newgroupsTable[fst] = tmp;
						newGroups.push_back(tmp);
					}
					for (size_t k = j+1; k < groups[i]->size(); k++) {
						auto scd = (*groups[i])[k];
						bool add = true;
						if (!newgroupsTable.contains(scd)) {
							for (auto i = alphabet.begin(); i != alphabet.end(); i++) {
								auto res1 = fst->makeTransition(*i);
								auto res2 = scd->makeTransition(*i);
								if (res1 && res2) {
									if (groupsTable[res1] != groupsTable[res2]) {
										add = false;
										break;
									}
								}
								else if (!(res1 == nullptr && res2 == nullptr)) {
									add = false;
									break;
								}
							}
						}
						if (add) {
							if (!newgroupsTable.contains(scd)) {
								newgroupsTable[fst]->push_back(scd);
								newgroupsTable[scd] = newgroupsTable[fst];
							}
						}
					}
				}
			}
			groupsTable = move(newgroupsTable);
		}
		groups = std::move(newGroups);
		auto start = groupsTable[start_];
		allStates.clear();
		acceptStates_.clear();
		std::unordered_map<shared_ptr<vector<shared_ptr<AutomataState>>>, shared_ptr<AutomataState>> oldToNew;
		for (auto i = groups.begin(); i != groups.end(); i++) {
				auto tmp = mergeStates(*i);	
				allStates.push_back(tmp);
				tmp->setNumber(allStates.size());
				if (tmp->isAcceptable())
					acceptStates_.push_back(tmp);

				oldToNew[*i] = tmp;
				if (start == *i)
					start_ = tmp;
		}
		states_count_ = allStates.size();
		for (auto i =groups.begin(); i != groups.end(); i++) {
			for (auto j = alphabet.begin(); j != alphabet.end(); j++) {
				auto res = (**i)[0]->makeTransition(*j);
				if (res) {
					oldToNew[(*i)]->addTransition(oldToNew[groupsTable[res]], *j);
				}
			}
		}
		
		nonacceptStates_.clear();
		/*vector < std::shared_ptr<list< shared_ptr<AutomataState>>>> groups;
		bool empt = true;
		std::shared_ptr<list< shared_ptr<AutomataState>>> g = std::make_shared < list< shared_ptr<AutomataState>>>();
		*g = std::move(acceptStates_);
		groups.push_back(std::move(g));
		if (!nonacceptStates_.empty()) {
			std::shared_ptr<list< shared_ptr<AutomataState>>> g = std::make_shared < list< shared_ptr<AutomataState>>>();
			*g = std::move(nonacceptStates_);
			groups.push_back(g);
			empt = false;
		}
		
		int num = empt?1:2;
		int prev;
		do {
			prev = groups.size();
			for (int i = 0; i < groups.size(); i++) {
				if (groups[i]->size() > 1) {
					for (auto j = groups[i]->begin(); j != groups[i]->end(); j++) {
						auto fst = *j;
						if ((**j).getNum() == -1) {
							(**j).setNum(num++);							
							groups.push_back(std::make_shared < list< shared_ptr<AutomataState>>>());
							groups[num - 1]->push_back(*j);
							groups[i]->erase(j--);
							
						}
						auto k = j;
						for (k++; k != groups[i]->end(); k++) {
							bool add = true;
							if ((**k).getNum() == -1|| (**k).getNum() <(empt?1:2)) {
								for (auto i = alphabet.begin(); i != alphabet.end(); i++) {
									auto res1 = (*fst).makeTransition(*i);
									auto res2 = (**k).makeTransition(*i);
									if (res1 && res2) {
										if (res1->getNum() != res2->getNum()) {
											add = false;
											break;
										}
										
									}
									else if (!(res1 == nullptr && res2 == nullptr)) {
										add = false;
										break;
									}
								}
							}
							if (add) {
								(**k).setNum((**j).getNum());
								groups[(**j).getNum()]->push_back(*k);
								groups[i]->erase(k--);
							}
						}
					}
				}
			}
		} while (prev != groups.size());
		allStates.clear();
		for (int i = 0; i < groups.size(); i++) {
			auto tmp = mergeStates(groups[i]);
			allStates.push_back(tmp);
			if(start_->getNum()==i)
				start_ = tmp;
		}
		for (int i =0; i < groups.size(); i++) {
			for (auto j = alphabet.begin(); j != alphabet.end(); j++) {
				auto res = groups[i]->front()->makeTransition(*j);
				if (res) {
					allStates[i]->addTransition(allStates[res->getNum()], *j);
				}
			}
		}
		acceptStates_.clear();
		nonacceptStates_.clear();*/
	}

	std::string DeterminedFinalAutomat::regexRecover() const {
		
		std::optional<std::string >*** memoryMatrix = new std::optional<std::string>**[states_count_];
		for (int i = 0; i < states_count_; i++) {
			memoryMatrix[i] = new std::optional<std::string>*[states_count_];
			for (int j = 0; j < states_count_; j++) {
				memoryMatrix[i][j]= new std::optional<std::string>[states_count_];
			}
		}
		std::unordered_set<std::string> meta;
		meta.insert(")");
		meta.insert("(");
		meta.insert("&");
		meta.insert("+");
		meta.insert("?");
		meta.insert(".");
		meta.insert("|");
		meta.insert("{");
		meta.insert("<");
		std::string result = "";
		try {
			for (auto j = acceptStates_.begin(); j != acceptStates_.end(); j++) {
				auto tmpRes = k_path(memoryMatrix, meta,start_, *j, states_count_);
				if (!tmpRes.empty() && !result.empty()) {
					result += '|';
				}
				result += tmpRes;
			}
			/*if (start_->isAcceptable()) {
				auto tmpRes = k_path(memoryMatrix, meta,start_, start_, states_count_);
				if (!tmpRes.empty() && !result.empty()) {
					result += '|';
				}
				result += tmpRes;
			}*/
		}
		catch (...) {
			for (int i = 0; i < states_count_; i++) {
				for (int j = 0; j < states_count_; j++) {
					delete[] memoryMatrix[i][j];
				}
				delete[] memoryMatrix[i];
			}
			delete[] memoryMatrix;
			throw std::exception("Unexpected error");
		}
		for (int i = 0; i < states_count_; i++) {
			for (int j = 0; j < states_count_; j++) {
				delete[] memoryMatrix[i][j];
			}
			delete[] memoryMatrix[i];
		}
		delete[] memoryMatrix;
		return result;
	}

	std::string DeterminedFinalAutomat::k_path(std::optional<std::string >*** memoryMatrix, std::unordered_set<std::string>& meta, std::shared_ptr<AutomataState> i, std::shared_ptr<AutomataState> j, unsigned short k) const {
		unsigned short in = i->getNumber()-1, jn = j->getNumber()-1;
		if (k == 0) {
			if (!memoryMatrix[in][jn][k]) {
				if (in == jn)
					memoryMatrix[in][jn][k] = "e{,0}";
				else
					memoryMatrix[in][jn][k] = "";
				std::unique_ptr<std::list<std::unique_ptr<Transition>>> lst(i->getAllTransitions());
					for (auto n = lst->begin(); n != lst->end(); n++) {
						if ((*n)->getTargetState() == j) {
							if (!(memoryMatrix[in][jn][k].value().empty())) {
								memoryMatrix[in][jn][k].value() += '|';
							}
							bool addbr = memoryMatrix[in][jn][k].value().size() > 1;
							if(addbr)
							memoryMatrix[in][jn][k].value() += '(';
							auto tmp= (*n)->getCondition().value();
							if (meta.contains(tmp))
								tmp = '&' + tmp;
							memoryMatrix[in][jn][k].value() += tmp.empty() ? "." : tmp;
							if(addbr)
							memoryMatrix[in][jn][k].value() += ')';
						}
					}
			}
			return memoryMatrix[in][jn][k].value();
		}
		else {
			std::string result = "";
			std::string tmpres = "";
			std::string tmpres2 = "";
			memoryMatrix[in][jn][k - 1] = memoryMatrix[in][jn][k-1]? memoryMatrix[in][jn][k - 1].value(): k_path(memoryMatrix, meta, i, j, k - 1);
			tmpres = memoryMatrix[in][jn][k - 1].value();
			if (!tmpres.empty()) {
				bool addbr = tmpres.size() > 1;
				if(addbr)
				result += '(';
				result += tmpres;
				if(addbr)
				result += ')';
			}	
			memoryMatrix[in][k-1][k - 1] = memoryMatrix[in][k-1][k - 1] ? memoryMatrix[in][k-1][k - 1].value() : k_path(memoryMatrix,meta, i, allStates[k - 1], k - 1);
			tmpres = memoryMatrix[in][k-1][k - 1].value();
			if (tmpres.empty())
				return result;
			tmpres2 += tmpres;
			tmpres2 += '(';
			memoryMatrix[k-1][k-1][k - 1] = memoryMatrix[k-1][k-1][k - 1] ? memoryMatrix[k-1][k-1][k - 1].value() : k_path(memoryMatrix,meta,allStates[k - 1], allStates[k - 1], k - 1);
			tmpres = memoryMatrix[k-1][k-1][k - 1].value();
			tmpres2 += tmpres;
			tmpres2 += "){,}";
			memoryMatrix[k-1][jn][k - 1] = memoryMatrix[k-1][jn][k - 1] ? memoryMatrix[k-1][jn][k - 1].value() : k_path(memoryMatrix, meta,allStates[k - 1], j, k - 1);
			tmpres = memoryMatrix[k-1][jn][k - 1].value();
			if (tmpres.empty())
				return result;
			tmpres2 += tmpres;
			bool addbracket = false;
			if (!result.empty()) {
				result += "|";
				if (tmpres2.size() > 1) {
					result += '(';
					addbracket = true;
				}
			}
			result += tmpres2;
			if(addbracket)
			result += ')';
			return result;
		}
	}

	DeterminedFinalAutomat& DeterminedFinalAutomat::operator=(DeterminedFinalAutomat&& obj) noexcept {
		if (this == &obj)
			return*this;
		states_count_ = obj.states_count_;
		obj.states_count_ = 0;
		start_ = std::move(obj.start_);
		acceptStates_ = std::move(obj.acceptStates_);
//		nonacceptStates_ = std::move(obj.nonacceptStates_);
		allStates = std::move(obj.allStates);
		return *this;
	}

	DeterminedFinalAutomat& DeterminedFinalAutomat::operator=(DeterminedFinalAutomat& obj) noexcept {
		if (this == &obj)
			return*this;
		states_count_ = obj.states_count_;
		start_ = (obj.start_);
		acceptStates_ =obj.acceptStates_;
		//		nonacceptStates_ = std::move(obj.nonacceptStates_);
		allStates = obj.allStates;
		return *this;
	}

	std::shared_ptr<AutomataState>  DeterminedFinalAutomat::mergeStates(std::shared_ptr<AutomataState>fst, std::shared_ptr<AutomataState>scd) const noexcept {
		std::shared_ptr<AutomataState> result = std::make_shared<DfaState>();
			result->addEntranceAction((*fst).getEntranceActions());
			result->addEExitAction((*fst).getExitActions());
			result->addEntranceAction((*scd).getEntranceActions());
			result->addEExitAction((*scd).getExitActions());
		return result;
	}

	DeterminedFinalAutomat DeterminedFinalAutomat::operator*(const DeterminedFinalAutomat& obj) const noexcept {
		std::vector<std::pair<std::shared_ptr<AutomataState>, std::shared_ptr<AutomataState>>> oldToNew;
		DeterminedFinalAutomat result;
		for (size_t i = 0; i < allStates.size(); i++) {
			for (size_t j = 0; j < obj.allStates.size(); j++) {
				result.allStates.push_back(mergeStates(allStates[i], obj.allStates[j]));
				if (result.start_ == nullptr && allStates[i] == start_ && obj.allStates[j] == obj.start_)
					result.start_ = result.allStates.back();
				result.states_count_++;
				if (allStates[i]->isAcceptable() && !obj.allStates[j]->isAcceptable()) {
					result.allStates.back()->setAccptable(true).setNumber(allStates.size());
						result.acceptStates_.push_back(result.allStates.back());
				}
				std::pair<std::shared_ptr<AutomataState>, std::shared_ptr<AutomataState>> tmp(allStates[i], obj.allStates[j]);
				oldToNew.push_back(tmp);
			}
		}
		std::shared_ptr<AutomataState> error = std::make_shared<DfaState>(true);
		error->setNumber(result.allStates.size() + 1);
		error->addTransition(error, "");
		for (size_t i = 0; i < result.allStates.size(); i++) {
			std::unique_ptr<std::list<std::unique_ptr<Transition>>> lst(oldToNew[i].second->getAllTransitions());
			if (!lst->empty()) {
				for (auto j = lst->begin(); j != lst->end(); j++) {
					auto tmp = oldToNew[i].first->makeTransition((**j).getCondition().value(), true);
					if (tmp) {
						std::pair<std::shared_ptr<AutomataState>, std::shared_ptr<AutomataState>> tmp_pair(tmp, (**j).getTargetState());
						for (size_t k = 0; k < oldToNew.size(); k++) {
							if (oldToNew[k] == tmp_pair) {
								result.allStates[i]->addTransition(result.allStates[k], (**j).getCondition());
								break;
							}
						}
						//result.allStates[i]->addTransition(result.allStates[newToOld[tmp_pair]],(**j).getCondition());
					}
				}
			}
			//else {
				result.allStates[i]->addTransition(error, "");
			//}
		}
		result.acceptStates_.push_back(error);
		result.allStates.push_back(error);
		return result;
	}
	std::vector<std::string> DeterminedFinalAutomat::findAll(std::string str, std::vector<std::unordered_map<std::string, std::string>>* groups) const {
		std::vector<std::string> result;
		auto c = str.cbegin();
		auto last_accept = c;
		bool was_accepted = false;
		while (c!=str.cend())
		{
			NamedGroup::tableClear();
			auto current = start_;
			current->doEnter(c);
			auto previous = current;
			for (auto i = c; i != str.cend(); i++) {
				previous = current;
				current = current->makeTransition(&i);
				if (current == nullptr) {
					if (previous->isAcceptable()) {
						std::string st = "";
						for (auto j = c; j != i; j++) {
							st += *j;
						}
						result.push_back(st);
						if (groups) {
							groups->push_back(NamedGroup::getTable());
						}
						c = i;
						was_accepted = false;
						break;
					}
					else if (was_accepted) {
						std::string st = "";
						for (auto j = c; j <= last_accept; j++) {
							st += *j;
						}
						result.push_back(st);
						if (groups) {
							groups->push_back(NamedGroup::getTable());
						}
						c = last_accept + 1;
						was_accepted = false;
						break;
					}
					else
					{
						c++;
						break;
					}
				}
				else {
					if (current->isAcceptable()) {
						was_accepted = true;
						last_accept = i;
					}
				}
					
			}
		}
		
		return result;
	}
}
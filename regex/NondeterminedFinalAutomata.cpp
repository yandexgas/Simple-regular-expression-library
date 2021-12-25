#include "NondeterminedFinalAutomata.h"
namespace regex {


    std::optional<std::shared_ptr<AutomataState>> regex::Transition::makeTransition(std::string::const_iterator* c)
    {

        if (condition.value().empty() || (condition.value().size()==1&&condition.value()[0] == **c)) {
            return targetState.lock();
        }
        else if (condition.value().size() > 1) {
            auto tmp = *c;
            try {
                for (size_t i = 0; i < condition.value().size(); i++) {
                    if (condition.value()[i] != *tmp) {
                        return{};
                    }
                    tmp++;
                }
                *c = --tmp;
                return targetState.lock();
            }
            catch (...) {
                return {};
            }
        }
        return {};
    }
    void AutomataState::doEnter(std::string::const_iterator& c) {
        for (auto i = entranceAction_.begin(); i != entranceAction_.end(); i++) {
            (**i).doAction(c,true,this);
        }
    }
    void AutomataState::doExit(std::string::const_iterator& c) {
        for (auto i = exitAction_.begin(); i != exitAction_.end(); i++) {
            (**i).doAction(c,false);
        }
    }

    std::shared_ptr <AutomataState> AutomataState::makeTransition(std::string::const_iterator* c) {
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

     std::shared_ptr <AutomataState> AutomataState::makeTransition(std::string str, bool exp) {
        for (auto i = transitions_.begin(); i != transitions_.end(); i++) {
            if ((**i).getCondition() == str) {
                return (**i).getTargetState();
            }
        }
        return nullptr;
    }

     void NondeterminedFinalAutomata::setStartState(std::shared_ptr<AutomataState> start) {
         if (start->isAcceptable())
             throw std::logic_error("For algorithm, which hab been used in this lib, start state can't be acceptable state");
         else {
             startState_ = start;
             currentStates_.push_back(startState_);
         }
     }

     void NondeterminedFinalAutomata::setAcceptState(std::shared_ptr<AutomataState> state) {
         if (!state->isAcceptable())
             throw std::logic_error("State must be acceptable");
         else {
             acceptableState_->setAccptable(false);
             acceptableState_ = state;
             currentStates_.push_back(acceptableState_);
         }
     }

     NondeterminedFinalAutomata& NondeterminedFinalAutomata::operator=(const NondeterminedFinalAutomata& source) {
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
                     unordered_map[(*j)->getTargetState()]->getEntranceActions() = (*j)->getTargetState()->getEntranceActions();
                     unordered_map[(*j)->getTargetState()]->getExitActions() = (*j)->getTargetState()->getExitActions();
                 }
                 unordered_map[*i]->addTransition(unordered_map[(*j)->getTargetState()], (*j)->getCondition());
             }
         }
         startState_ = unordered_map[source.startState_];
         acceptableState_ = unordered_map[source.acceptableState_];
         return *this;
     }
}
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
}
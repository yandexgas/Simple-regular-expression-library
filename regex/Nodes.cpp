#include "Nodes.h"

namespace regex{
	std::unique_ptr<Node> Leaf::createNode(std::string::const_iterator& iter) const {
			std::string s;
			s += *iter;
			return std::make_unique<Leaf>(s);
		}

	void Leaf::buildNfa() {
		std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
		std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
		start->addTransition(end, getSimbol());
		automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
	}

	UnaryMetaNode::UnaryMetaNode(short int side) :MetaNode() {
		if (side == 0 || side == 1)
			this->side_ = side;
		else
			throw std::invalid_argument("Incorrect side: " + side);
	};

	bool UnaryMetaNode::setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse)  {
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

	bool BinaryMetaNode::setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse)  {
		if (complited())
			return false;
		auto leftNode = --iter;
		++iter;
		auto rightNode = ++iter;
		iter--;
		if ((*leftNode)->complited() && (*rightNode)->complited()) {
			right_ = !inverse ? *rightNode : *leftNode;
			right_->setParent(*(iter));
			left_ = !inverse ? *leftNode : *rightNode;
			left_->setParent(*iter);
		}
		else
			throw SintaxTree_Exception("Incorrect usage of operation. Some of oherations have less operands then they should.");
		return true;
	}

	bool ScriningNode::setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse) {
		if (complited())
			return false;
		++iter;
		right_ = std::make_shared<Leaf>((*iter)->getSimbol());
		right_->setParent(*--iter);
		return true;
	}

	void NamedGroup::NamedGroupAction::doAsExit(std::string::const_iterator& currentChar) noexcept {
		if (!groups->contains(nameofgroup_)) {
			begin = {};
			end = {};
			(*groups)[nameofgroup_] = "";
		}
		if (!begin || currentChar > begin.value())
			begin = currentChar;
	};

	void NamedGroup::NamedGroupAction::doAsEnter(std::string::const_iterator& currentChar) {
		if (groups->contains(nameofgroup_) && begin) {
			if (!end || currentChar >= end.value())
				end = currentChar;
			std::string str;
			if (begin <= end) {
				auto tmp = begin.value();
				for (; tmp != end; tmp++)
					str += *tmp;
				str += *(end.value());
				tmp++;
			}
			(*groups)[nameofgroup_] = str;
		}
	};

	std::unique_ptr<Node> NamedGroup::createNode(std::string::const_iterator& iter) const  {
		std::string nameOfGroup_ = "";
		std::string tmp;
		bool correct = false;
		int count = 0;
		for (int i : std::ranges::views::iota(0, (int)sign.length())) {
			if (iter != eos_) {
				count++;
				tmp += *iter++;
			}
			else break;
		}
		if (tmp == sign) {
			while (iter != eos_ && !correct)
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

	bool NamedGroup::setChild(std::list<std::shared_ptr<Node>>::iterator iter, bool inverse) {
		if (complited())
			return false;
		UnaryMetaNode::setChild(iter);
		if (complited()) {
			(*groups)[nameOfGroup_] = "";
		}
		return true;
	}

	void NamedGroup::buildNfa() {
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
		auto action = std::make_shared<NamedGroupAction>(nameOfGroup_, ofset);
		automatOfNode->getStartState()->addEExitAction(action);
		automatOfNode->getAcceptState()->addEntranceAction(action);
	}

	void ConcatNode::buildNfa() {
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

	void OrNode::buildNfa() {
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

	void PositiveClosureNode::buildNfa() {
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

	void AnyNode::buildNfa(){
		std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
		std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
		start->addTransition(end, "\0");
		automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
	}

	std::unique_ptr<Node> RepeatNode::createNode(std::string::const_iterator& iter)  const {
		int count = 0;
		std::string first, second;
		if (*iter == '{') {
			++iter;
			++count;
			while (iter != eos_ && std::isdigit(*iter))
			{
				first += *iter++;
				count++;
			}
			if (iter != eos_ && *iter == ',') {
				++iter;
				++count;
				while (iter != eos_ && std::isdigit(*iter))
				{
					second += *iter++;
					count++;
				}
				if (iter != eos_ && *iter == '}') {
					std::pair<std::optional<std::int32_t>, std::optional<std::int32_t>> dia;
					if (first.empty())
						dia.first = std::nullopt;
					else
						dia.first = std::stoi(first);
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

	std::string RepeatNode::getSimbol() const noexcept  {
		std::stringstream s{};
		s << RepeatNode::sign;
		if (diapason_.first)
			s << diapason_.first.value();
		s << ',';
		if (diapason_.second)
			s << diapason_.second.value();
		s << '}';
		std::string res;
		s >> res;
		return res;
	}

	void RepeatNode::buildNfa() {
		std::unique_ptr<NondeterminedFinalAutomata> leftAutom = left_->stealNodesNfa();
		std::unique_ptr<NondeterminedFinalAutomata> leftCopy(new NondeterminedFinalAutomata());
		*leftCopy = *leftAutom;//Копирование
		if (!diapason_.second) {

			std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
			std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
			start->addTransition(end);
			start->addTransition(leftAutom->getStartState());
			leftAutom->getAcceptState()->addTransition(end);
			leftAutom->getAcceptState()->addTransition(leftAutom->getStartState());
			leftAutom->setAcceptState(end);
			leftAutom->setStartState(start);
			automatOfNode = std::move(leftAutom);
		}
		else {
			if ((diapason_.first && diapason_.first.value() == diapason_.second.value()) || (!diapason_.first && diapason_.second.value() == 0)) {
				if (diapason_.second.value() == 0) {
					std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
					std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
					start->addTransition(end);
					automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
					return;
				}
			}
			else {
				leftAutom->getStartState()->addTransition(leftAutom->getAcceptState());
				std::unique_ptr<NondeterminedFinalAutomata> leftCopy2(new NondeterminedFinalAutomata());
				*leftCopy2 = *leftAutom;
				for (int i = diapason_.first ? diapason_.first.value() + 1 : 1; i < diapason_.second.value(); i++) {
					std::unique_ptr<NondeterminedFinalAutomata> addittive(new NondeterminedFinalAutomata());
					*addittive = *leftCopy2;

					std::list<std::unique_ptr<Transition>>* transitions = addittive->getStartState()->getAllTransitions();
					for (auto i = transitions->begin(); i != transitions->end(); i++) {
						leftAutom->getAcceptState()->addTransition(std::move(*i));
					}
					leftAutom->setAcceptState(addittive->getAcceptState());
					for (auto i = addittive->getCurrentStates().begin(); i != addittive->getCurrentStates().end(); i++) {
						if ((*i) != addittive->getStartState() && (*i) != addittive->getAcceptState())
							leftAutom->addStateToList(*i);
					}
				}
				automatOfNode = std::move(leftAutom);
			}
		}
		if (diapason_.first && diapason_.first.value() > 0) {
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
					if ((*i) != addittive->getStartState() && (*i) != addittive->getAcceptState())
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

	void LinkNamedGroup::LinkGroupAction::doAction(std::string::const_iterator& c, bool, AutomataState* state ){
		if ((*groups).contains(nameofgroup_)) {
			std::string str = (*groups)[nameofgroup_];
			//if (link != str) {
			auto tmp = state->makeTransition(link);
			auto lnk = tmp ? link : constLink;
			tmp = tmp ? tmp : state->makeTransition(constLink);
			if (tmp) {
				state->changeCondition(lnk, str);
				link = str;
			}
			//}
		}
		else
			throw std::logic_error("attempt to access an uninitialized capture group");
	}

	std::unique_ptr<Node> LinkNamedGroup::createNode(std::string::const_iterator& iter) const  {
		std::string nameOfGroup_ = "";
		int count = 0;
		if (*iter == '<') {
			++iter;
			++count;
			while (iter != eos_ && (std::isalpha(*iter) || *iter == '>'))
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

	void LinkNamedGroup::buildNfa() {
		std::shared_ptr<AutomataState> start = std::make_shared<AutomataState>();
		std::shared_ptr<AutomataState> end = std::make_shared<AutomataState>(true);
		std::stringstream str;
		str << nameOfGroup_ << "#lnk" << links;
		std::string st;
		str >> st;
		start->addTransition(end, st);
		alphabet->insert(st);
		start->addEntranceAction(std::make_shared<LinkGroupAction>(nameOfGroup_, st));
		LinkNamedGroup::links++;
		automatOfNode = std::make_unique<NondeterminedFinalAutomata>(start, end);
	}
}
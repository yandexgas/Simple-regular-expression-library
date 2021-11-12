#include "SintaxTree.h"

namespace regex {
	bool SintaxTree::buildTreePrepare(const std::string& sourceString) noexcept{

		std::vector<std::unique_ptr<Node>> nodeTemlates;
		nodeTemlates.emplace_back(std::make_unique<ScriningNode>());
		nodeTemlates.emplace_back(std::make_unique<NamedGroup>("",sourceString.cend()));
		nodeTemlates.emplace_back(std::make_unique<OpenPriority>());
		nodeTemlates.emplace_back(std::make_unique<ClosePriority>());
		nodeTemlates.emplace_back(std::make_unique<PositiveClosureNode>());
		nodeTemlates.emplace_back(std::make_unique<OptionNode>());
		nodeTemlates.emplace_back(std::make_unique<RepeatNode>(sourceString.cend()));
		nodeTemlates.emplace_back(std::make_unique<ConcatNode>());
		nodeTemlates.emplace_back(std::make_unique<AnyNode>());
		nodeTemlates.emplace_back(std::make_unique<LinkNamedGroup>( "",sourceString.cend()));
		nodeTemlates.emplace_back(std::make_unique<OrNode>());
		nodeTemlates.emplace_back(std::make_unique<Leaf>());
		nodesList_.emplace_back(std::make_shared<OpenPriority>());

		for (std::string::const_iterator i = sourceString.cbegin(); i != sourceString.cend(); i++) {
			std::unique_ptr<Node> tmp = nullptr;
			int nodeTemplateNumber = -1;
			switch (*i)
			{
			case'&': {
				alphabet.insert("&");
				if ((i + 1)!= sourceString.cend()) {
					std::string str(1, *(i + 1));
					alphabet.insert(str);
				}
				nodeTemplateNumber = 0;
				break;
			}
			case '(': {
				tmp = nodeTemlates[1]->createNode(i);
				if(tmp==nullptr)
					tmp= nodeTemlates[2]->createNode(i);
				break; }
			case ')': nodeTemplateNumber = 3;
				break;
			case'+':  
				nodeTemplateNumber = 4;
				break; 
			case'?': 
				nodeTemplateNumber = 5;
				break; 
			case '{':
				nodeTemplateNumber = 6;
				break;
			case '.': {
				nodeTemplateNumber = 8;
				alphabet.insert("");
				break; }
			case '<': 
				nodeTemplateNumber = 9;
				break;
			case '|': 
				nodeTemplateNumber = 10;
				break;
			default: {
				nodeTemplateNumber = 11;
				std::string str(1, *i);
				alphabet.insert(str);
				break; }
			}	
			if (nodeTemplateNumber != -1) {
				tmp = nodeTemlates[nodeTemplateNumber]->createNode(i);
				if (tmp == nullptr && nodeTemplateNumber != 11)
					tmp = nodeTemlates[11]->createNode(i);
			}
				bool addBracket = (*tmp).getType() == NodeTypes::NAMED_GROUP;
				if (nodeTemplateNumber == 9)
					tmp->setAlphabet(&alphabet);
				nodesList_.emplace_back(std::move(tmp));	
				if (addBracket)
					nodesList_.emplace_back(std::make_shared<OpenPriority>(30));
		}
		nodesList_.emplace_back(std::make_shared<ClosePriority>());
		return true;
	}
	
	void SintaxTree::build(bool inverse) {

		std::stack<std::list<std::shared_ptr<Node>>::iterator> openBrakets;
		for (auto i = nodesList_.begin(); i != nodesList_.end(); ++i) {
			if ((*i)->getType() == NodeTypes::SCREENING) {
				(**i).setChild(i);
				auto prev = i;
				nodesList_.erase(++i);
				i = prev;
				continue;
			}
			if ((*i)->getType() == NodeTypes::OPEN_PRIORITY)
				openBrakets.push(i);
			else if ((*i)->getType() == NodeTypes::CLOSE_PRIORITY) {
				auto prev = openBrakets.top();
				openBrakets.pop();
				if (prev != nodesList_.cbegin()) {
					build(prev--, i,inverse);
					i = ++prev;
				}
				else {
					build(prev, i,inverse);
					break;
				}
			}
		}
		if (!openBrakets.empty())
			throw SintaxTree_Exception('(');
		if(nodesList_.size()>1)
			throw SintaxTree_Exception(')');
		root_ = nodesList_.front();
		nodesList_.clear();
	}

	void SintaxTree::build(std::list<std::shared_ptr<Node>>::iterator beg, std::list<std::shared_ptr<Node>>::iterator end, bool inverse ) {
		for (int prior = 4; prior > 1; prior--) {
			for (auto i = beg; i != end; i++) {
				if ((**i).getPriority() == prior) {
					if ((**i).setChild(i)) {
						switch ((**i).getType())
						{
						case regex::NodeTypes::SCREENING: {
							auto prev = i;
							nodesList_.erase(++i);
							i = prev;
							break; }
						case regex::NodeTypes::OR: {
							auto prev = i;
							nodesList_.erase(++i);
							i = --prev;
							prev--;
							nodesList_.erase(i);
							i = ++prev;
							break; }
						case regex::NodeTypes::POSITIVE_�LOSURE: {
							auto prev = --i;
							prev--;
							nodesList_.erase(i);
							i = ++prev;
							break; }
						case regex::NodeTypes::OPTION: {
							auto prev = --i;
							prev--;
							nodesList_.erase(i);
							i = ++prev;
							break; }
						case regex::NodeTypes::REPEAT: {
							auto prev = --i;
							prev--;
							nodesList_.erase(i);
							i = ++prev;
							break; }
						case regex::NodeTypes::NAMED_GROUP: {
							auto prev = i;
							nodesList_.erase(++i);
							i = prev;
							break; }
						default:
							throw std::runtime_error("Shock");
						}
					}
				}
			}
		}
		auto i = beg;
		i++;
		for (; i != end; i++) {
			auto ptr = i;
			ptr++;
			if (ptr != end) {
				nodesList_.emplace(ptr, std::make_shared<ConcatNode>());
				ptr = i;
				ptr++;
				(**ptr).setChild(ptr,inverse);
				nodesList_.erase(++ptr);
				ptr = i;
				i--;
				nodesList_.erase(ptr);
				
			}
		}
		nodesList_.erase(beg);
		nodesList_.erase(end);
	}
}
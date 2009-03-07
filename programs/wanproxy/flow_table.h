#ifndef	FLOW_TABLE_H
#define	FLOW_TABLE_H

struct Flow {
	struct Endpoint {
		std::string source_;
		std::string destination_;
		std::string codec_;
	};

	enum Direction {
		Incoming,
		Outgoing,
	};

	Endpoint local_;
	Direction direction_;
	Endpoint remote_;
};

class FlowTable {
	LogHandle log_;
	std::string name_;
	std::map<void *, Flow> flows_;
public:
	FlowTable(const std::string& name)
	: log_("/flow/table"),
	  name_(name),
	  flows_()
	{ }

	~FlowTable()
	{ }

	void insert(void *key, Flow::Endpoint local, Flow::Direction direction,
		    Flow::Endpoint remote)
	{
		if (flows_.find(key) != flows_.end()) {
			ERROR(log_) << "Duplicate flow.";
			return;
		}
		flows_[key].local_ = local;
		flows_[key].direction_ = direction;
		flows_[key].remote_ = remote;
	}

	void erase(void *key)
	{
		if (flows_.find(key) == flows_.end()) {
			ERROR(log_) << "Flow not in table.";
			return;
		}
		flows_.erase(key);
	}

	template<typename T>
	void enumerate(T *obj, void (T::*method)(const FlowTable *, const Flow *)) const
	{
		if (flows_.empty()) {
			(obj->*method)(this, NULL);
			return;
		}

		std::map<void *, Flow>::const_iterator it;

		for (it = flows_.begin(); it != flows_.end(); ++it)
			(obj->*method)(this, &it->second);
	}
};

#endif /* !FLOW_TABLE_H */

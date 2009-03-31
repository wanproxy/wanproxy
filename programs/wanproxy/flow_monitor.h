#ifndef	FLOW_MONITOR_H
#define	FLOW_MONITOR_H

class Flow;
class FlowTable;

class FlowMonitor {
	class Serializer {
		FlowMonitor *monitor_;
		std::map<std::string, std::string> tables_;
	public:
		Serializer(FlowMonitor *monitor)
		: monitor_(monitor),
		  tables_()
		{ }

		void serialize(const FlowTable *, const Flow *);
		void operator() (Buffer *) const;
	};
	friend class Serializer;

	LogHandle log_;
	Action *action_;
	Action *stop_action_;
	bool stop_;
	std::map<const FlowTable *, std::string> flow_tables_;
public:
	FlowMonitor(void);
	~FlowMonitor();

	void monitor(const std::string&, FlowTable *);
private:
	void report(void);
	void schedule_timeout(void);
	void stop(void);
};

#endif /* !FLOW_MONITOR_H */

#ifndef	FLOW_MONITOR_H
#define	FLOW_MONITOR_H

class Flow;
class FlowTable;

class FlowMonitor {
	LogHandle log_;
	Action *action_;
	std::map<FlowTable *, std::string> flow_tables_;
public:
	FlowMonitor(void);
	~FlowMonitor();

	void monitor(const std::string&, FlowTable *);
private:
	void report_flow(const FlowTable *, const Flow *);
	void report(void);
	void schedule_timeout(void);
};

#endif /* !FLOW_MONITOR_H */

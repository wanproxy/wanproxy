#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include "flow_monitor.h"
#include "flow_table.h"

std::ostream& operator<< (std::ostream&, const Flow::Direction&);
std::ostream& operator<< (std::ostream&, const Flow::Endpoint&);

FlowMonitor::FlowMonitor(void)
: log_("/flow/monitor"),
  action_(NULL),
  flow_tables_()
{ }

FlowMonitor::~FlowMonitor()
{
	ASSERT(action_ == NULL);
}

void
FlowMonitor::monitor(const std::string& name, FlowTable *flow_table)
{
	DEBUG(log_) << "Monitoring flow table " << flow_table << " named " << name;

	flow_tables_[flow_table] = name;

	if (action_ == NULL) {
		schedule_timeout();
	}
}

void
FlowMonitor::report_flow(const FlowTable *, const Flow *flow)
{
	if (flow == NULL) {
		return;
	}
	INFO(log_) << flow->local_ << flow->direction_ << flow->remote_;
}

void
FlowMonitor::report(void)
{
	action_->cancel();
	action_ = NULL;

	std::map<FlowTable *, std::string>::const_iterator it;

	for (it = flow_tables_.begin(); it != flow_tables_.end(); ++it) {
		FlowTable *flow_table = it->first;

		INFO(log_) << "Flow table " << it->second << ":";
		flow_table->enumerate(this, &FlowMonitor::report_flow);
	}

	if (!flow_tables_.empty()) {
		schedule_timeout();
	}
}

void
FlowMonitor::schedule_timeout(void)
{
	Callback *cb = callback(this, &FlowMonitor::report);
	action_ = EventSystem::instance()->timeout(10 * 1000, cb);
}

std::ostream&
operator<< (std::ostream& os, const Flow::Direction& direction)
{
	switch (direction) {
	case Flow::Outgoing:
		return (os << " -> ");
	case Flow::Incoming:
		return (os << " <- ");
	default:
		NOTREACHED();
	}
}

std::ostream&
operator<< (std::ostream& os, const Flow::Endpoint& endpoint)
{
	return (os << endpoint.source_ << " <-> " << endpoint.codec_ << " <-> " << endpoint.destination_);
}

#include <common/buffer.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_system.h>

#include "flow_monitor.h"
#include "flow_table.h"

std::ostream& operator<< (std::ostream&, const Flow::Direction&);
std::ostream& operator<< (std::ostream&, const Flow::Endpoint&);

void
FlowMonitor::Serializer::serialize(const FlowTable *flow_table, const Flow *flow)
{
	std::string name = monitor_->flow_tables_[flow_table];

	if (flow == NULL) {
		tables_[name] = "<nil />";
		return;
	}

	std::ostringstream ostr;

	ostr << "<flow direction=\"" << flow->direction_ << "\">";
	ostr << "<local " << flow->local_ << " />";
	ostr << "<remote " << flow->remote_ << " />";
	ostr << "</flow>";

	tables_[name] += ostr.str();
}

void
FlowMonitor::Serializer::operator() (Buffer *output) const
{
	std::ostringstream ostr;

	/* XXX doctype, etc.  */

	std::map<std::string, std::string>::const_iterator it;
	for (it = tables_.begin(); it != tables_.end(); ++it) {
		ostr << "<table name=\"" << it->first << "\">";
		ostr << it->second;
		ostr << "</table>";
	}

	output->append(ostr.str());
}

FlowMonitor::FlowMonitor(void)
: log_("/flow/monitor"),
  action_(NULL),
  stop_action_(NULL),
  stop_(false),
  flow_tables_()
{ }

FlowMonitor::~FlowMonitor()
{
	ASSERT(action_ == NULL);
	ASSERT(stop_action_ == NULL);
}

void
FlowMonitor::monitor(const std::string& name, FlowTable *flow_table)
{
	DEBUG(log_) << "Monitoring flow table " << flow_table << " named " << name;

	flow_tables_[flow_table] = name;

	if (action_ == NULL) {
		schedule_timeout();

		ASSERT(stop_action_ == NULL);
		Callback *cb = callback(this, &FlowMonitor::stop);
		stop_action_ = EventSystem::instance()->register_interest(EventInterestStop, cb);
	}
}

void
FlowMonitor::report(void)
{
	action_->cancel();
	action_ = NULL;

	Serializer serializer(this);

	std::map<const FlowTable *, std::string>::const_iterator it;
	for (it = flow_tables_.begin(); it != flow_tables_.end(); ++it) {
		const FlowTable *flow_table = it->first;
		flow_table->enumerate(&serializer, &Serializer::serialize);
	}

	Buffer buf;
	serializer(&buf);

	INFO(log_) << buf;

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

void
FlowMonitor::stop(void)
{
	stop_action_->cancel();
	stop_action_ = NULL;

	action_->cancel();
	action_ = NULL;
}

std::ostream&
operator<< (std::ostream& os, const Flow::Direction& direction)
{
	switch (direction) {
	case Flow::Outgoing:
		return (os << "outgoing");
	case Flow::Incoming:
		return (os << "incoming");
	default:
		NOTREACHED();
	}
}

std::ostream&
operator<< (std::ostream& os, const Flow::Endpoint& endpoint)
{
	return (os << "source=\"" << endpoint.source_ << "\" destination=\"" << endpoint.destination_ << "\" codec=\"" << endpoint.codec_ << "\"");
}

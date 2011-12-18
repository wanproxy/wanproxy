#include <common/buffer.h>
#include <common/test.h>

#include <event/action.h>
#include <event/callback.h>
#include <event/event_callback.h>
#include <event/event_main.h>
#include <event/event_system.h>

#include <http/http_server_pipe.h>

#include <io/channel.h>

#include <io/pipe/pipe.h>
#include <io/pipe/splice.h>

class SinkStream : public StreamChannel {
	Buffer buffer_;
public:
	SinkStream(void)
	: buffer_()
	{ }

	~SinkStream()
	{ }

	Buffer get_buffer(void) const
	{
		return (buffer_);
	}

	Action *close(SimpleCallback *cb)
	{
		return (cb->schedule());
	}

	Action *read(size_t, EventCallback *)
	{
		NOTREACHED("/sink/stream");
	}

	Action *write(Buffer *buf, EventCallback *cb)
	{
		buf->moveout(&buffer_);
		cb->param(Event::Done);
		return (cb->schedule());
	}

	Action *shutdown(bool shut_read, bool shut_write, EventCallback *cb)
	{
		ASSERT("/sink/stream", !shut_read);
		ASSERT("/sink/stream", shut_write);

		cb->param(Event::Done);
		return (cb->schedule());
	}
};

class SourceStream : public StreamChannel {
	Buffer buffer_;
public:
	SourceStream(void)
	: buffer_()
	{ }

	SourceStream(const Buffer& buffer)
	: buffer_(buffer)
	{ }

	~SourceStream()
	{ }

	void set_buffer(const Buffer& buffer)
	{
		buffer_ = buffer;
	}

	Action *close(SimpleCallback *cb)
	{
		return (cb->schedule());
	}

	Action *read(size_t amt, EventCallback *cb)
	{
		if (amt == 0 || amt >= buffer_.length()) {
			Buffer tmp;
			buffer_.moveout(&tmp);
			cb->param(Event(Event::EOS, tmp));
		} else {
			Buffer tmp;
			buffer_.moveout(&tmp, amt);
			cb->param(Event(Event::Done, tmp));
		}
		return (cb->schedule());
	}

	Action *write(Buffer *, EventCallback *)
	{
		NOTREACHED("/source/stream");
	}

	Action *shutdown(bool, bool, EventCallback *)
	{
		NOTREACHED("/source/stream");
	}
};

class HTTPServerPipeTest {
protected:
	LogHandle log_;
	TestGroup& group_;
	SinkStream sink_;
	HTTPServerPipe pipe_;
private:
	SourceStream source_;
	Splice *splice_;
	Action *splice_action_;
	Test splice_success_;
protected:
	Action *request_action_;
public:
	HTTPServerPipeTest(const LogHandle& log, TestGroup& group, const Buffer& source)
	: log_(log),
	  group_(group),
	  sink_(),
	  pipe_(log_ + "/pipe"),
	  source_(source),
	  splice_(NULL),
	  splice_action_(NULL),
	  splice_success_(group, "Splice success."),
	  request_action_(NULL)
	{
		HTTPRequestEventCallback *rcb = callback(this, &HTTPServerPipeTest::request);
		request_action_ = pipe_.request(rcb);

		splice_ = new Splice(log_ + "/splice", &source_, &pipe_, &sink_);
		EventCallback *cb = callback(this, &HTTPServerPipeTest::splice_complete);
		splice_action_ = splice_->start(cb);
	}

	virtual ~HTTPServerPipeTest()
	{
		ASSERT(log_, splice_ == NULL);
		ASSERT(log_, splice_action_ == NULL);

		if (request_action_ != NULL) {
			DEBUG(log_) << "Test exited without receiving a request.";
			request_action_->cancel();
			request_action_ = NULL;
		}
	}

private:
	virtual void request(Event, HTTPProtocol::Request) = 0;

	void splice_complete(Event e)
	{
		splice_action_->cancel();
		splice_action_ = NULL;

		if (e.type_ == Event::EOS)
			splice_success_.pass();

		delete splice_;
		splice_ = NULL;
	}
};

class HTTP09Test : HTTPServerPipeTest {
public:
	HTTP09Test(TestGroup& group, const std::string& line_ending)
	: HTTPServerPipeTest("/test/http/server/pipe/http_0_9", group, "GET /" + line_ending)
	{ }

	~HTTP09Test(void)
	{
		{
			Test _(group_, "Request received.");
			if (request_action_ == NULL)
				_.pass();
		}

		HTTPProtocol::Response resp;
		{
			Test _(group_, "Decode response.");

			Buffer buf = sink_.get_buffer();
			if (resp.decode(&buf))
				_.pass();
		}

		{
			Test _(group_, "Expected status.");
			if (resp.start_line_.equal("HTTP/1.1 200 OK"))
				_.pass();
		}

		/* XXX Check headers.  */

		{
			Test _(group_, "Expected body.");
			if (resp.body_.equal("Successful HTTP/0.9 request."))
				_.pass();
		}
	}

private:
	void request(Event e, HTTPProtocol::Request msg)
	{
		ASSERT(log_, request_action_ != NULL);
		request_action_->cancel();
		request_action_ = NULL;

		{
			Test _(group_, "Successfully got request.");
			if (e.type_ == Event::Done)
				_.pass();
		}

		{
			Test _(group_, "Expected start line.");
			if (msg.start_line_.equal("GET /"))
				_.pass();
		}

		{
			Test _(group_, "Expected headers.");
			if (msg.headers_.empty())
				_.pass();
		}

		pipe_.send_response(HTTPProtocol::OK, "Successful HTTP/0.9 request.");
	}
};

int
main(void)
{
	TestGroup g("/test/http/server/pipe1", "HTTPServerPipe #1");

	HTTP09Test http_0_9_test_crlf(g, "\r\n");
	HTTP09Test http_0_9_test_lf(g, "\n");
	event_main();

	return (0);
}

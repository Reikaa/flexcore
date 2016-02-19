#define BOOST_ALL_DYN_LINK
#define BOOST_LOG_USE_NATIVE_SYSLOG
#include <logging/logger.hpp>
#include <boost/utility/empty_deleter.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/core.hpp>
#include <ios>

namespace fc
{

stream_handle::stream_handle(std::function<void()> deleter) : deleter(deleter)
{
}

stream_handle::~stream_handle()
{
	deleter();
}

logger* logger::get()
{
	static logger instance;
	return &instance;
}

using namespace boost::log;

template <class backend>
using sync_sink = sinks::synchronous_sink<backend>;

void logger::add_file_log(std::string filename)
{
	auto file_sink = boost::make_shared<sinks::text_file_backend>(
	    keywords::file_name = filename,
	    keywords::open_mode = std::ios_base::app | std::ios_base::out);
	core::get()->add_sink(boost::make_shared<sync_sink<sinks::text_file_backend>>(file_sink));
}

void logger::add_syslog_log(std::string progname)
{
	auto syslog_sink = boost::make_shared<sinks::syslog_backend>(
	    keywords::use_impl = sinks::syslog::impl_types::native,
	    keywords::ident = progname);
	core::get()->add_sink(boost::make_shared<sync_sink<sinks::syslog_backend>>(syslog_sink));
}

stream_handle logger::add_stream_log(std::ostream& stream, logger::flush flush,
                                     logger::cleanup cleanup)
{
	auto stream_sink = boost::make_shared<sinks::text_ostream_backend>();
	auto shared_stream = boost::shared_ptr<std::ostream>(&stream, boost::empty_deleter());
	stream_sink->add_stream(shared_stream);
	stream_sink->auto_flush(static_cast<bool>(flush));
	core::get()->add_sink(boost::make_shared<sync_sink<sinks::text_ostream_backend>>(stream_sink));

	// Prepare a function that will remove the stream from the sink
	std::function<void()> cleanup_fun;
	if (static_cast<bool>(cleanup))
		cleanup_fun = [=]() { stream_sink->remove_stream(shared_stream); };
	else
		cleanup_fun = []() {};

	return stream_handle(cleanup_fun);
}

logger::logger()
{
}

} // namespace fc

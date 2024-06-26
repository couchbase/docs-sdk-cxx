// #tag::logging[]
#import <couchbase/logger.hxx>

void
initialize_logger()
{
    // Initialize logging to standard output
    couchbase::logger::initialize_console_logger();

    // Initialize logging to a file
    couchbase::logger::initialize_file_logger("/path/to/file");

    // Set log level
    couchbase::logger::set_level(couchbase::logger::log_level::warn);
}
// #end::logging[]

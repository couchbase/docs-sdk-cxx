#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>
#include <couchbase/logger.hxx>

#include <iostream>
#include <system_error>

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

int
main()
{
    {
        // #tag::timeout[]
        auto options = couchbase::cluster_options(username, password);
        options.timeouts()
                .key_value_timeout(std::chrono::seconds(5))
                .query_timeout(std::chrono::seconds(10));
        // #end::timeout[]
    }
}

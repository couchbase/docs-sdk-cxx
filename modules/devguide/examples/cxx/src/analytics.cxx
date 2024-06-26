// tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

#include <tao/json/to_string.hpp>
// end::imports[]

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, couchbase::cluster_options{ username, password }).get();

int
main()
{

    {
        // #tag::analytics_query[]
        std::string query{ R"(SELECT "hello" AS greeting)" };
        auto [err, res] = cluster.analytics_query(query).get();
        if (err) {
            fmt::println("Got an error doing analytics query: {}", err);
        } else {
            auto rows = res.rows_as_json();
            for (const auto& row : rows) {
                fmt::println("row: {}", tao::json::to_string(row));
            }
        }
        // #end::analytics_query[]
    }

    {
        // #tag::simple_parameters[]
        std::string query{ R"(SELECT airportname, country FROM airports WHERE country = ?)" };
        auto options = couchbase::analytics_options().positional_parameters("France");
        auto [err, res] = cluster.analytics_query(query, options).get();
        // #end::simple_parameters[]
    }

    {
        // #tag::named_parameters[]
        std::string query{ R"(SELECT airportname, country FROM airports WHERE country = $country)" };
        auto options = couchbase::analytics_options().named_parameters(std::pair{ "country", "France" });
        auto [err, res] = cluster.analytics_query(query, options).get();
        // #end::named_parameters[]
    }

    {
        // #tag::additional_parameters[]
        std::string query{ R"(SELECT airportname, country FROM airports WHERE country = "France")" };
        auto options = couchbase::analytics_options()
                 // Ask the analytics service to give this request higher priority
                 .priority(true)
                 // The client context id is returned in the results, so can be used by the
                 // application to correlate requests and responses
                 .client_context_id("my-id")
                 // Override how long the analytics query is allowed to take before timing out
                 .timeout(std::chrono::milliseconds(90000));

        auto [err, res] = cluster.analytics_query(query, options).get();
        // #end::additional_parameters[]
    }

    {
        // #tag::metadata[]
        std::string query{ R"(SELECT airportname, country FROM airports WHERE country = "France")" };
        auto [err, res] = cluster.analytics_query(query).get();

        if (err) {
            fmt::println("Got an error doing analytics query: {}", err);
        } else {
            auto rows = res.rows_as_json();
            for (const auto& row : rows) {
                fmt::println("row: {}", tao::json::to_string(row));
            }

            auto elapsed_time = res.meta_data().metrics().elapsed_time();
            auto result_count = res.meta_data().metrics().result_count();
            auto error_count = res.meta_data().metrics().error_count();
        }
        // #end::metadata[]
    }
}

#include <couchbase/codec/raw_json_transcoder.hxx>

// #tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/cas.hxx>
#include <couchbase/fmt/error.hxx>

#include <fmt/format.h>
#include <tao/json/value.hpp>
#include <tao/json/to_string.hpp>

#include <iostream>
// #end::imports[]

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

auto
main() -> int
{
    // #tag::cluster[]
    auto options = couchbase::cluster_options(username, password);
    options.apply_profile("wan_development");
    auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
    if (connect_err) {
        fmt::println("Unable to connect to the cluster: {}", connect_err);
        return 1;
    }
    auto bucket = cluster.bucket(bucket_name);
    auto scope = bucket.scope(scope_name);
    auto collection = bucket.scope(scope_name).collection(collection_name);
    // #end::cluster[]

    {
        // tag::simple[]
        std::string statement = "SELECT * from `travel-sample` LIMIT 10;";
        auto [err, result] = cluster.query(statement, {}).get();
        // end::simple[]

        // tag::simple-results[]
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Got {} rows", result.rows_as<couchbase::codec::tao_json_serializer, tao::json::value>().size());
        }
        // end::simple-results[]
    }

    {
        // tag::get-rows[]
        auto [err, result] = cluster.query("SELECT * FROM `travel-sample` LIMIT 10;", {}).get();
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            for (const auto& row : result.rows_as_json()) {
                fmt::println("{}", tao::json::to_string(row));
            }
        }
        // end::get-rows[]
    }

    {
        // tag::positional[]
        std::string stmt = R""""(
                            SELECT COUNT(*)
                            FROM `travel-sample`.inventory.airport
                            WHERE country=$1;
                            )"""";
        auto [err, result] =
          cluster
            .query(
              stmt, couchbase::query_options().adhoc(false).positional_parameters("United States")
            )
            .get();
        // end::positional[]
        fmt::println("{}", tao::json::to_string(result.rows_as_json().at(0)));
    }

    {
        // tag::named[]
        std::string stmt = R""""(
                            SELECT COUNT(*)
                            FROM `travel-sample`.inventory.airport
                            WHERE country=$country;
                            )"""";
        auto [err, result] =
          cluster
            .query(stmt, couchbase::query_options().named_parameters(std::make_pair<std::string, std::string>("country", "United States")))
            .get();
        // end::named[]
        fmt::println("{}", tao::json::to_string(result.rows_as_json().at(0)));
    }

    {
        // tag::request-plus[]
        auto [err, result] = cluster
                               .query(
                                 "SELECT * FROM `travel-sample` LIMIT 10;",
                                 couchbase::query_options().scan_consistency(couchbase::query_scan_consistency::request_plus)
                               )
                               .get();
        // end::request-plus[]
    }

    {
        tao::json::value content{ { "foo", "bar" } };
        // tag::at-plus[]
        auto [upsert_err, upsert_result] = collection.upsert("id", content).get();
        assert(!upsert_err); // Just for demo, a production app should check the result properly

        couchbase::mutation_state state;
        state.add(upsert_result);
        auto [err, result] =
          cluster.query("SELECT * FROM `travel-sample` LIMIT 10;", couchbase::query_options().consistent_with(state)).get();
        if (err) {
            fmt::println("Error: {}", err);
        }
        // end::at-plus[]
    }

    {
        // tag::scope-level[]
        auto [err, result] = scope.query("SELECT * FROM airline LIMIT 10;", {}).get();
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            for (const auto& row : result.rows_as_json()) {
                fmt::println("{}", tao::json::to_string(row));
            }
        }
        // end::scope-level[]
    }

    {
        // #tag::create-index[]
        auto err = cluster.query_indexes().create_primary_index("users", {}).get();
        if (err) {
            fmt::println("Error creating primary index: {}", err);
        }
        // Fore brevity, skipping error handling in the rest for the examples
        cluster.query_indexes().create_index("users", "index_name", { "name" }, {}).get();
        cluster.query_indexes().create_index("users", "index_email", { "email" }, {}).get();
        // #end::create-index[]
    }

    {
        // #tag::build-index[]
        cluster.query_indexes()
          .create_primary_index(
            "users", couchbase::create_primary_query_index_options().build_deferred(true)
          )
          .get();
        cluster.query_indexes()
          .create_index(
            "users",
            "index_name",
            { "name" },
            couchbase::create_query_index_options().build_deferred(true)
          )
          .get();
        cluster.query_indexes()
          .create_index(
            "users",
            "index_email",
            { "email" },
            couchbase::create_query_index_options().build_deferred(true)
          )
          .get();

        // Build the indexes
        cluster.query_indexes().build_deferred_indexes("users", {}).get();

        // Wait for the indexes to build
        cluster.query_indexes()
          .watch_indexes(
            "users",
            { "index_name", "index_email" },
            couchbase::watch_query_indexes_options().watch_primary(true).timeout(
              std::chrono::seconds(30)
            )
          )
          .get();
        // #end::build-index[]
    }

    cluster.close().get();
    return 0;
}

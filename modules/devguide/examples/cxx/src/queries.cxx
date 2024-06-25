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
        std::cout << "Unable to connect to the cluster: " << fmt::format("{}", connect_err) << "\n";
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
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            std::cout << fmt::format("Got {} rows\n", result.rows_as<couchbase::codec::tao_json_serializer, tao::json::value>().size());
        }
        // end::simple-results[]
    }

    {
        // tag::get-rows[]
        auto [err, result] = cluster.query("SELECT * FROM `travel-sample` LIMIT 10;", {}).get();
        if (err) {
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            for (const auto& row : result.rows_as_json()) {
                std::cout << tao::json::to_string(row) << "\n";
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
        auto [err, result] = cluster.query(stmt, couchbase::query_options().adhoc(false).positional_parameters("United States")).get();
        // end::positional[]
        std::cout << fmt::format("{}", tao::json::to_string(result.rows_as_json().at(0))) << "\n";
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
        std::cout << fmt::format("{}", tao::json::to_string(result.rows_as_json().at(0))) << "\n";
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
            std::cout << fmt::format("Error: {}\n", err);
        }
        // end::at-plus[]
    }

    {
        // tag::scope-level[]
        auto [err, result] = scope.query("SELECT * FROM airline LIMIT 10;", {}).get();
        if (err) {
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            for (const auto& row : result.rows_as_json()) {
                std::cout << tao::json::to_string(row) << "\n";
            }
        }
        // end::scope-level[]
    }

    cluster.close().get();
    return 0;
}

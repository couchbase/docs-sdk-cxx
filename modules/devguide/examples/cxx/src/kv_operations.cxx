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
static constexpr auto bucket_name{ "travel-sample" };
static constexpr auto scope_name{ "inventory" };
static constexpr auto collection_name{ "airline" };

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
        // #tag::upsert[]
        auto content = tao::json::value{
            { "foo", "bar" },
            { "baz", "qux" },
        };
        auto [err, result] = collection.upsert("document-key", content).get();
        if (err) {
            fmt::println("Error: {}\n", err);
        } else {
            fmt::println("Document upsert successful");
        }
        // #end::upsert[]
    }

    cluster.close().get();
    return 0;
}

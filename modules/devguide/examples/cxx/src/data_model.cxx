#include <couchbase/codec/raw_json_transcoder.hxx>

// #tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/cas.hxx>
#include <couchbase/fmt/error.hxx>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

#include <functional>
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
    auto cluster_options = couchbase::cluster_options(username, password);
    cluster_options.apply_profile("wan_development");
    auto [connect_err, cluster] =
      couchbase::cluster::connect(connection_string, cluster_options).get();
    if (connect_err) {
        std::cout << "Unable to connect to the cluster: " << fmt::format("{}", connect_err) << "\n";
        return 1;
    }
    auto bucket = cluster.bucket(bucket_name);
    auto collection = bucket.scope(scope_name).collection(collection_name);
    // #end::cluster[]

    {
        collection.upsert("doc-id", tao::json::value{ { "foo", "bar" } }).get();

        // #tag::xattr[]
        auto [err, result] =
          collection
            .lookup_in(
              "doc-id",
              couchbase::lookup_in_specs{
                couchbase::lookup_in_specs::get(couchbase::subdoc::lookup_in_macro::expiry_time)
                  .xattr(),
              }
            )
            .get();
        // #end::xattr[]
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Result: {}", result.content_as<std::uint32_t>(0));
        }
    }

    cluster.close().get();
    return 0;
}

#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };
auto cluster_options = couchbase::cluster_options(username, password);

auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, cluster_options).get();

auto bucket = cluster.bucket(bucket_name);
auto collection = bucket.scope(scope_name).collection(collection_name);

int
main()
{
    {
        // tag::upsert_future[]
        auto content = tao::json::value{
                { "foo", "bar" },
                { "baz", "qux" },
        };

        // The .get() call returns the result from the std::future<std::pair<couchbase::error, couchbase::mutation_result>>
        auto [err, res] = collection.upsert("document-key", content).get();
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Success");
        }
        // end::upsert_future[]
    }

    {
        // tag::upsert_callback[]
        auto content = tao::json::value{
                { "foo", "bar" },
                { "baz", "qux" },
        };
        collection.upsert("document-key", content, {}, [](auto err, auto res) {
            if (err) {
                fmt::println("Error: {}", err);
            } else {
                // Can easily 'chain' another operation here. e.g. get the document we just upserted.
                fmt::println("Success");
            }
        });
        // end::upsert_callback[]
    }
}

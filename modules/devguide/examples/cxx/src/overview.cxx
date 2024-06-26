
#import <couchbase/cluster.hxx>
#import <couchbase/fmt/error.hxx>

#import <tao/json/to_string.hpp>

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
    std::vector<double> vector_query;
    auto scope = cluster.bucket(bucket_name).scope(scope_name);


    {
        // #tag::upsert[]
        auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

        const std::string document_id{ "minimal_example" };
        const tao::json::value basic_doc{
                { "a", 1.0 },
                { "b", 2.0 },
        };

        auto [err, res] = collection.upsert(document_id, basic_doc, {}).get();
        if (err) {
            fmt::println("Unable to perform upsert: {}", err);
        } else {
            fmt::println("id: {}, CAS: {}", document_id, res.cas().value());
        }
        // #end::upsert[]
    }

    {
        // #tag::query[]
        auto scope = cluster.bucket(bucket_name).scope(scope_name);

        auto [err, resp] = scope.query("SELECT * FROM airline WHERE id = 10").get();
        if (err) {
            fmt::println("Unable to perform query: {}", err);
        }

        for (const auto& row : resp.rows_as_json()) {
            fmt::println("row: {}", tao::json::to_string(row));
        }
        // #end::query[]
    }

    {
        // #tag::vector_search[]
        couchbase::search_request request(couchbase::vector_search(couchbase::vector_query("vector_field", vector_query)));

        auto [err, res] = scope.search("vector-index", request).get();

        if (err) {
            fmt::println("Got an error doing vector search: {}", err);
        } else {
            for (const auto& row : res.rows()) {
                fmt::println("id: {}, score: {}", row.id(), row.score());
            }
        }
        // #end::vector_search[]
    }
}
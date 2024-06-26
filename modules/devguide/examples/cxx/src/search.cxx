// tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>
#include <couchbase/match_all_query.hxx>
#include <couchbase/match_query.hxx>
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
    std::vector<double> vector_query;
    std::vector<double> another_vector_query;
    auto scope = cluster.bucket(bucket_name).scope(scope_name);
    auto collection = scope.collection(collection_name);

    // #tag::vector_search[]
    {
        couchbase::search_request request(couchbase::vector_search(couchbase::vector_query("vector_field", vector_query)));

        auto [err, res] = scope.search("vector-index", request).get();
    }
    // #end::vector_search[]
    {
        // #tag::multiple_vector_queries[]
        std::vector<couchbase::vector_query> vector_queries{
            couchbase::vector_query("vector_field", vector_query).num_candidates(2).boost(0.3),
            couchbase::vector_query("vector_field", another_vector_query).num_candidates(5).boost(0.7)
        };

        auto request = couchbase::search_request(couchbase::vector_search(vector_queries));
        auto [err, res] = scope.search("vector-index", request).get();
        // #end::multiple_vector_queries[]
    }

    {
        // #tag::combined_search_request[]
        auto request = couchbase::search_request(couchbase::match_all_query())
                         .vector_search(couchbase::vector_search(couchbase::vector_query("vector_field", vector_query)));

        auto [err, res] = scope.search("vector-and-fts-index", request).get();
        // #end::combined_search_request[]
    }

    {
        // #tag::match_query[]
        auto request = couchbase::search_request(couchbase::match_query("swanky"));
        auto options = couchbase::search_options().limit(10);

        auto [err, res] = cluster.search("travel-sample-index-hotel-description", request, options).get();

        if (err) {
            fmt::println("Got an error doing search: {}", err);
        } else {
            auto& rows = res.rows();
            // Handle rows
        }
        // #end::match_query[]
    }

    {
        // #tag::working_with_results[]
        auto request = couchbase::search_request(couchbase::match_query("swanky"));
        auto options = couchbase::search_options().limit(10);

        auto [err, res] = cluster.search("travel-sample-index-hotel-description", request, options).get();

        if (err) {
            fmt::println("Got an error doing search: {}", err);
        } else {
            for (const auto& row : res.rows()) {
                auto id = row.id();
                auto score = row.score();
                // ...
            }

            // Metadata
            auto max_score = res.meta_data().metrics().max_score();
            auto success_count = res.meta_data().metrics().success_partition_count();
        }
        // #end::working_with_results[]
    }

    {
        // #tag::consistency[]
        const tao::json::value hotel{
            { "name", "Hotel California" },
            { "desc", "Such a lonely place" },
        };

        auto [insert_err, insert_res] = collection.insert("newHotel", hotel).get();
        if (insert_err) {
            fmt::println("Got an error inserting the document: {}", insert_err);
        } else {
            auto mutation_state = couchbase::mutation_state();
            mutation_state.add(insert_res);

            auto request = couchbase::search_request(couchbase::match_query("lonely"));
            auto options = couchbase::search_options().limit(10).consistent_with(mutation_state);
            auto [search_err, search_res] = cluster.search("travel-sample-index", request, options).get();
            // ...
        }
        // #end::consistency[]
    }
}
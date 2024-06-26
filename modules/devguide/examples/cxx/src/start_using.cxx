
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

#include <iostream>

#include <tao/json.hpp>


static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

int
main(int argc, char** argv)
{
    static constexpr auto connection_string{ "couchbase://127.0.0.1" };
    static constexpr auto username{ "Administrator" };
    static constexpr auto password{ "password" };

    {

        auto options = couchbase::cluster_options(username, password);
        auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
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
        static constexpr auto connection_string{ "couchbase://127.0.0.1" };
        static constexpr auto username{ "Administrator" };
        static constexpr auto password{ "password" };
        // #tag::connect_capella[]
        auto options = couchbase::cluster_options(username, password);
        options.apply_profile("wan_development");

        auto [err, cluster] = couchbase::cluster::connect(connection_string, options).get();

        if (err) {
            fmt::println("Unable to connect to the cluster: {}", err);
        } else {
            // Application code here
        }
        // #end::connect_capella[]
    }

    {
        // #tag::connect_onprem[]
        couchbase::cluster_options options(username, password);

        auto [err, cluster] = couchbase::cluster::connect(connection_string, options).get();
        if (err) {
            fmt::println("Unable to connect to the cluster: {}", err);
        } else {
            // Application code here
        }
        // #end::connect_onprem[]
    }

    {
        static constexpr auto username{ "Administrator" };
        static constexpr auto password{ "password" };
        // #tag::tls_skip_verify[]
        couchbase::cluster_options options(username, password);
        options.security().tls_verify(couchbase::tls_verify_mode::none);
        // #end::tls_skip_verify[]
    }


    {
        auto options = couchbase::cluster_options(username, password);
        auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
        // #tag::get[]
        const std::string document_id{ "minimal_example" };

        auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

        auto [err, res] = collection.get(document_id).get();

        if (err) {
            fmt::println("Unable to perform get: {}", err);
        } else {
            std::cout << "id: " << document_id << ", result: " << res.content_as<tao::json::value>() << "\n";
        }
        // #end::get[]
    }

    {
        auto options = couchbase::cluster_options(username, password);
        auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
        // #tag::replace[]
        auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

        const std::string document_id{ "minimal_example" };
        const tao::json::value basic_doc{
                { "a", 1.0 },
                { "b", 2.0 },
        };

        auto [err, res] = collection.replace(document_id, basic_doc).get();
        if (err) {
            fmt::println("Unable to perform replace: {}", err);
        } else {
            fmt::println("id: {}, CAS: {}", document_id, res.cas().value());
        }
        // #end::replace[]
    }

    {
        auto options = couchbase::cluster_options(username, password);
        auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
        // #tag::remove[]
        auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

        const std::string document_id{ "minimal_example" };

        auto [err, res] = collection.remove(document_id).get();
        if (err) {
            fmt::println("Unable to perform remove: {}", err);
        } else {
            fmt::println("id: {}, CAS: {}", document_id, res.cas().value());
        }
        // #end::remove[]
    }




}
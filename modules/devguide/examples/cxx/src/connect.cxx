// tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>
// end::imports[]

static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

int
main(int argc, char** argv)
{
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
        std::string connection_string{ argv[1] }; // "couchbase://127.0.0.1"
        std::string username{ argv[2] };          // "Administrator"
        std::string password{ argv[3] };          // "password"

        couchbase::cluster_options options(username, password);

        auto [err, cluster] = couchbase::cluster::connect(connection_string, options).get();
        if (err) {
            fmt::println("Unable to connect to the cluster: {}", err);
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
}

#include <couchbase/cluster.hxx>



static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

int
main(int argc, char** argv)
{
    {
        static constexpr auto username{ "Administrator" };
        static constexpr auto password{ "password" };
        // #tag::tls_skip_verify[]
        couchbase::cluster_options options(username, password);
        options.security().tls_verify(couchbase::tls_verify_mode::none);
        // #end::tls_skip_verify[]
    }
}
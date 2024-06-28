#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

#include <iostream>

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
    {
        // tag::ping[]
        auto options = couchbase::ping_options().service_types({ couchbase::service_type::key_value, couchbase::service_type::query });
        auto [err, res] = cluster.ping(options).get();
        if (err) {
            fmt::println("Got an error doing ping: {}", err);
        } else {
            fmt::println("{}", res.as_json());
        }
        /*
        {
            "id":"0x10290d100","kv":[
                {
                    "id":"0000000072b21d66",
                    "last_activity_us":2363294,
                    "local":"10.112.195.1:51473",
                    "remote":"10.112.195.101:11210",
                    "status":"connected"
                },
                {
                    "id":"000000000ba84e5e",
                    "last_activity_us":7369021,
                    "local":"10.112.195.1:51486",
                    "remote":"10.112.195.102:11210",
                    "status":"connected"
                },
                {
                    "id":"0000000077689398",
                    "last_activity_us":4855640,
                    "local":"10.112.195.1:51409",
                    "remote":"10.112.195.103:11210",
                    "status":"connected"
                }
            ],
            "sdk":"cxx/1.0.0/be41e5e;Darwin/arm64",
            "version":1
        }
        */
        // end::ping[]
    }

    {
        // #tag::diagnostics[]
        auto [err, res] = cluster.diagnostics().get();
        if (err) {
            fmt::println("Got an error doing diagnostics: {}", err);
        } else {
            fmt::println(res.as_json());
        }
        /*
        {
            "id":"0x10290d100","kv":[
                {
                    "id":"0000000072b21d66",
                    "last_activity_us":2363294,
                    "local":"10.112.195.1:51473",
                    "remote":"10.112.195.101:11210",
                    "status":"connected"
                },
                {
                    "id":"000000000ba84e5e",
                    "last_activity_us":7369021,
                    "local":"10.112.195.1:51486",
                    "remote":"10.112.195.102:11210",
                    "status":"connected"
                },
                {
                    "id":"0000000077689398",
                    "last_activity_us":4855640,
                    "local":"10.112.195.1:51409",
                    "remote":"10.112.195.103:11210",
                    "status":"connected"
                }
            ],
            "sdk":"cxx/1.0.0/be41e5e;Darwin/arm64",
            "version":1
        }
        */
        // #end::diagnostics[]
    }
}
// #tag::connection_lifecycle[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

#include <iostream>

int
main(int argc, const char* argv[])
{
    if (argc != 4) {
        std::cout << "USAGE: ./start_using couchbase://127.0.0.1 Administrator password\n";
        return 1;
    }

    std::string connection_string{ argv[1] }; // "couchbase://127.0.0.1"
    std::string username{ argv[2] };          // "Administrator"
    std::string password{ argv[3] };          // "password"
    std::string bucket_name{ "travel-sample" };

    auto options = couchbase::cluster_options(username, password);
    // customize through the 'options'.
    // For example, optimize timeouts for WAN
    options.apply_profile("wan_development");

    // [1] connect to cluster using the given connection string and the options
    auto [err, cluster] = couchbase::cluster::connect(connection_string, options).get();
    if (err) {
        fmt::println("Unable to connect to the cluster: {}", err);
        return 1;
    }

    // get a bucket reference
    auto bucket = cluster.bucket(bucket_name);

    // get a user-defined collection reference
    auto scope = bucket.scope("tenant_agent_00");
    auto collection = scope.collection("users");

    { // your database interactions here
    }

    // close cluster connection
    cluster.close().get();
    return 0;
}
// #end::connection_lifecycle[]

void
connect()
{
    std::string connection_string{ "127.0.0.1" }; // "couchbase://127.0.0.1"
    std::string username{ "Administrator" };          // "Administrator"
    std::string password{ "password" };          // "password"

    auto options = couchbase::cluster_options(username, password);
    // #tag::connect_line[]
    auto [err, cluster] = couchbase::cluster::connect(connection_string, options).get();
    // #end::connect_line[]
}


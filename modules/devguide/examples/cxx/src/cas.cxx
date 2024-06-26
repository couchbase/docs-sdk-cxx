// tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

#include <iostream>
// end::imports[]

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, couchbase::cluster_options{ username, password }).get();
// #tag::loop[]
int
casLoop(const couchbase::collection& collection, const std::string& doc_id, int max_retries = 10)
{
    for (int i = 0; i < max_retries; i++) {
        // Get the current document contents
        auto [get_err, get_res] = collection.get(doc_id).get();

        if (get_err) {
            fmt::println("Got an error during get: {}", get_err);
            return 1;
        }

        // Get and modify the content
        auto content = get_res.content_as<tao::json::value>();
        content["visitCount"] = content["visitCount"].get_unsigned() + 1;

        // Try to replace the document, using CAS
        auto [replace_err, replace_res] = collection.replace(doc_id, content, couchbase::replace_options().cas(get_res.cas())).get();
        if (replace_err) {
            // Check if the error returned is a cas mismatch, if it is, we retry
            if (replace_err.ec() == couchbase::errc::common::cas_mismatch) {
                continue;
            }
            // Something else went wrong - fast fail
            fmt::println("Something else went wrong during replace: {}", replace_err);
            return 1;
        }
        // Succeeded - we're done
        return 0;
    }
    std::cout << "Maxed out our retries\n";
    return 1;
}
// #end::loop[]

int
main()
{
    auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

    // #tag::lockAndUnlock[]
    auto [get_err, get_res] = collection.get_and_lock("key", std::chrono::seconds(10)).get();
    if (get_err) {
        fmt::println("Got an error during get_and_lock: {}", get_err);
    } else {
        auto unlock_err = collection.unlock("key", get_res.cas()).get();
        if (unlock_err) {
            fmt::println("Got an error during unlock: {}", unlock_err);
        } else {
            // Successfully unlocked
        }
    }
    // #end::lockAndUnlock[]
}

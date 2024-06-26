#include <couchbase/cluster.hxx>
#include <couchbase/fmt/error.hxx>

static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

int
main()
{
    std::string connection_string{ "127.0.0.1" }; // "couchbase://127.0.0.1"
    std::string username{ "Administrator" };          // "Administrator"
    std::string password{ "password" };          // "password"
    std::string doc_id{ "id" };
    auto options = couchbase::cluster_options(username, password);
    auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
    auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

    auto new_json = tao::json::value{
            { "foo", "bar" },
            { "baz", "qux" },
    };

    {
        // tag::replace[]
        auto content = tao::json::value{
                { "foo", "bar" },
                { "baz", "qux" },
        };
        auto [err, res] = collection.replace("does-not-exist", content).get();

        if (err.ec() == couchbase::errc::key_value::document_not_found) {
            fmt::println("Key not found - full error: {}", err);
        } else if (err) {
            fmt::println("Some other error happened: {}", err);
        } else {
            fmt::println("Operation succeeded");
        }
        // end::replace[]
    }

    {
        // tag::get[]
        auto [err, res] = collection.get("does-not-exist").get();

        if (err.ec() == couchbase::errc::key_value::document_not_found) {
            fmt::println("Key not found - full error: {}", err);
        } else if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Operation succeeded");
        }
        // end::get[]
    }

    {
        // tag::doc_exists[]
        auto content = tao::json::value{
                { "foo", "bar" },
                { "baz", "qux" },
        };
        auto [err, res] = collection.insert("does-already-exist", content).get();

        if (err.ec() == couchbase::errc::key_value::document_exists) {
            fmt::println("Key already exists - full error: {}", err);
        } else if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Operation succeeded");
        }
        // end::doc_exists[]
    }
    {
        // tag::cas[]
        for (int i = 0; i < 3; i++) {
            auto [get_err, get_res] = collection.get(doc_id).get();
            if (get_err) {
                fmt::println("Got an error during get: {}", get_err);
            } else {
                auto [replace_err, replace_res] = collection.replace(doc_id, new_json, couchbase::replace_options().cas(get_res.cas())).get();
                if (replace_err.ec() == couchbase::errc::common::cas_mismatch) {
                    continue; // Try again
                } else if (replace_err) {
                    fmt::println("Error: {}", replace_err);
                    break;
                } else {
                    fmt::println("Success");
                    break;
                }
            }
        }
        // end::cas[]
    }

    {
        // tag::query[]
        std::string statement = "SELECT * from `travel-sample` LIMIT 10;";
        auto [err, res] = cluster.query(statement, {}).get();

        if (err) {
            fmt::println("Error: {}", err);
        } else {
            // Do something with the result
        }
        // end::query[]
    }
}

// tag::do_insert[]
std::string
do_insert(const couchbase::collection& collection, const std::string& doc_id, int max_retries = 10)
{
    auto json = tao::json::value{
            { "foo", "bar" },
            { "baz", "qux" },
    };
    for (int attempt = 0; attempt < max_retries; attempt++) {
        auto options = couchbase::insert_options().durability(couchbase::durability_level::majority);
        auto [err, res] = collection.insert(doc_id, json, options).get();

        if (err.ec() == couchbase::errc::key_value::document_exists) {
            // The logic here is that if we failed to insert on the first attempt then
            // it's a true error, otherwise we retried due to an ambiguous error, and
            // the operation was actually successful
            if (attempt == 0) {
                return "failure";
            }
            return "success";
        } else if (err.ec() == couchbase::errc::key_value::durability_ambiguous ||
                   err.ec() == couchbase::errc::common::ambiguous_timeout) {
            // For ambiguous errors on inserts, simply retry them
            continue;
        } else if (err) {
            // Some other non-ambiguous error occurred
            return "failure";
        } else {
            // No error
            return "success";
        }
    }
    // Maxed-out retry attempts
    return "failure";
}
// end::do_insert[]

// tag::do_insert_real[]
std::string
do_insert(const couchbase::collection& collection,
               const std::string& doc_id,
               int max_retries = 10,
               std::chrono::milliseconds delay = std::chrono::milliseconds(5))
{
    auto json = tao::json::value{
            { "foo", "bar" },
            { "baz", "qux" },
    };
    for (int attempt = 0; attempt < max_retries; attempt++) {
        auto options = couchbase::insert_options().durability(couchbase::durability_level::majority);
        auto [err, res] = collection.insert(doc_id, json, options).get();

        if (err.ec() == couchbase::errc::key_value::document_exists) {
            // The logic here is that if we failed to insert on the first attempt then
            // it's a true error, otherwise we retried due to an ambiguous error, and
            // the operation was actually successful
            if (attempt == 0) {
                return "failure";
            }
            return "success";
        } else if (
                   // Ambiguous errors.  The operation may or may not have succeeded.  For inserts,
                   // the insert can be retried, and a DocumentExistsException indicates it was
                   // successful
                   err.ec() == couchbase::errc::key_value::durability_ambiguous ||
                   err.ec() == couchbase::errc::common::ambiguous_timeout ||
                   // Temporary/transient errors that are likely to be resolved
                   // on a retry.
                   err.ec() == couchbase::errc::common::temporary_failure ||
                   err.ec() == couchbase::errc::key_value::durable_write_in_progress ||
                   err.ec() == couchbase::errc::key_value::durable_write_re_commit_in_progress ||
                   // These transient errors won't be returned on an insert, but can be used
                   // when writing similar wrappers for other mutation operations.
                   err.ec() == couchbase::errc::common::cas_mismatch) {
            // Retry the operation after a sleep (which increases on each failure),
            // to avoid potentially further overloading an already failing server.
            std::this_thread::sleep_for(delay *= 2);
            continue;
        } else if (err) {
            // Some other non-ambiguous & non-transient error occurred
            return "failure";
        } else {
            // No error
            return "success";
        }
    }
    // Maxed-out retry attempts
    return "failure";
}
// end::do_insert_real[]

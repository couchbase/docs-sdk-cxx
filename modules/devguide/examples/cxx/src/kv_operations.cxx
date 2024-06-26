#include <couchbase/codec/raw_json_transcoder.hxx>

// #tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/cas.hxx>
#include <couchbase/fmt/error.hxx>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <tao/json/value.hpp>

#include <functional>
#include <iostream>
// #end::imports[]

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

// #tag::replace-retry[]
auto
retry_on_cas_mismatch(std::function<couchbase::error()> op) -> couchbase::error
{
    while (true) {
        // Perform the operation
        auto err = op();
        if (err.ec() == couchbase::errc::common::cas_mismatch) {
            // Retry if the couchbase::error wraps a cas_mismatch error code
            continue;
        } else {
            // If success or any other failure, return it
            return err;
        }
    }
}

void
get_and_replace(const couchbase::collection& collection)
{
    auto initial = tao::json::value{ { "status", "great" } };

    // Insert some initial data
    {
        auto [err, res] = collection.insert("document-key5", initial).get();
        assert(!err); // Just for demo, a production app should check the result properly
    }

    // This is the get-and-replace we want to do, as a lambda
    auto op = [&]() -> couchbase::error {
        auto [get_err, get_res] = collection.get("document-key5").get();
        if (get_err) {
            return get_err;
        }
        auto content = get_res.content_as<tao::json::value>();
        content["status"] = "awesome!";
        auto options = couchbase::replace_options().cas(get_res.cas());
        auto [replace_err, replace_res] = collection.replace("document-key5", content, options).get();
        return replace_err;
    };

    // Send our lambda to retry_on_cas_mismatch to take care of retrying it.
    auto err = retry_on_cas_mismatch(op);
    if (err) {
        fmt::println("Error: {}", err);
    } else {
        fmt::println("Replace with CAS successful");
    }
}
// #end::replace-retry[]

auto
main() -> int
{
    // #tag::cluster[]
    auto cluster_options = couchbase::cluster_options(username, password);
    cluster_options.apply_profile("wan_development");
    auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, cluster_options).get();
    if (connect_err) {
        std::cout << "Unable to connect to the cluster: " << fmt::format("{}", connect_err) << "\n";
        return 1;
    }
    auto bucket = cluster.bucket(bucket_name);
    auto collection = bucket.scope(scope_name).collection(collection_name);
    // #end::cluster[]

    {
        // #tag::upsert[]
        auto content = tao::json::value{
            { "foo", "bar" },
            { "baz", "qux" },
        };
        auto [err, result] = collection.upsert("document-key", content).get();
        // #end::upsert[]
        // #tag::upsert-error[]
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Document upsert successful");
        }
        // #end::upsert-error[]
    }

    {
        // #tag::insert[]
        auto content = tao::json::value{
            { "foo", "bar" },
            { "baz", "qux" },
        };
        auto [err, result] = collection.insert("document-key2", content).get();
        if (err.ec() == couchbase::errc::key_value::document_exists) {
            fmt::println("The document already exists");
        } else if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Document inserted successfully");
        }
        // #end::insert[]
    }

    {
        // #tag::get-simple[]
        auto [err, result] = collection.get("document-key").get();
        if (err) {
            fmt::println("Error getting document: {}", err);
        }
        // #end::get-simple[]
    }

    {
        // #tag::get[]
        // Create some initial JSON
        auto content = tao::json::value{ { "status", "awesome!" } };

        // Upsert it
        auto [upsert_err, insert_result] = collection.upsert("document-key3", content).get();
        if (upsert_err) {
            fmt::println("Error inserting document: {}", upsert_err);
        } else {
            // Get it back
            auto [get_err, get_result] = collection.get("document-key3").get();

            // Get the content of the document as a tao::json::value
            auto res_content = get_result.content_as<tao::json::value>();

            // Pull out the JSON content's status field, if it exists
            if (const auto s = res_content.find("status"); s != nullptr) {
                fmt::println("Couchbase is {}", s->get_string());
            } else {
                fmt::println("Field 'status' does not exist");
            }
        }
        // #end::get[]
    }

    {
        // #tag::replace[]
        auto initial = tao::json::value{ { "status", "great" } };

        // Upsert a document
        auto [upsert_err, upsert_res] = collection.upsert("document-key4", initial).get();
        if (upsert_err) {
            fmt::println("Error upserting document: {}", upsert_err);
        } else {
            // Get the document back
            auto [get_err, get_res] = collection.get("document-key4").get();

            // Extract the document as tao::json::value
            auto content = get_res.content_as<tao::json::value>();

            // Modify the content
            content["status"] = "awesome!";

            // Replace the document with the updated content, and the document's CAS value
            // (which we'll cover in a moment)
            auto [replace_err, replace_res] =
              collection.replace("document-key4", content, couchbase::replace_options().cas(get_res.cas())).get();
            if (replace_err.ec() == couchbase::errc::common::cas_mismatch) {
                fmt::println("Could not write as another agent has concurrently modified the document");
            } else if (replace_err) {
                fmt::println("Error: {}", replace_err);
            } else {
                fmt::println("Document replaced successfully");
            }
        }
        // #end::replace[]
    }

    {
        get_and_replace(collection);
        collection.remove("document-key5").get();
    }

    {
        // #tag::remove[]
        auto [err, result] = collection.remove("document-key").get();
        if (err.ec() == couchbase::errc::key_value::document_not_found) {
            fmt::println("The document does not exist");
        } else if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Document removed successfully");
        }
        // #end::remove[]
    }

    {
        // #tag::durability[]
        auto options = couchbase::remove_options().durability(couchbase::durability_level::majority);
        auto [err, result] = collection.remove("document-key2", options).get();
        if (err.ec() == couchbase::errc::key_value::document_not_found) {
            fmt::println("The document does not exist");
        } else if (err) {
            fmt::println("Error: {}", err);
        } else {
            // The mutation is available in-memory on at least a majority of replicas
            fmt::println("Document removed successfully");
        }
        // #end::durability[]
    }

    {
        // #tag::durability-observed[]
        auto options = couchbase::remove_options().durability(couchbase::persist_to::none, couchbase::replicate_to::two);
        auto [err, result] = collection.remove("document-key2", options).get();
        if (err.ec() == couchbase::errc::key_value::document_not_found) {
            fmt::println("The document does not exist");
        } else if (err) {
            fmt::println("Error: {}", err);
        } else {
            // The mutation is available in-memory on at least two replicas
            fmt::println("Document removed successfully");
        }
        // #end::durability-observed[]
    }

    {
        // #tag::expiry-insert[]
        auto content = tao::json::value{ { "foo", "bar" }, { "baz", "qux" } };

        auto [err, result] = collection.insert("document-key", content, couchbase::insert_options().expiry(std::chrono::hours(2))).get();
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Document with expiry inserted successfully");
        }
        // #end::expiry-insert[]
    }

    {
        // #tag::expiry-get[]
        auto [err, result] = collection.get("document-key", couchbase::get_options().with_expiry(true)).get();
        if (err) {
            fmt::println("Error getting document: {}", err);
        } else if (result.expiry_time().has_value()) {
            fmt::println("Got expiry: {}", result.expiry_time().value());
        } else {
            fmt::println("Error: no expiration field");
        }
        // #end::expiry-get[]
    }

    {
        // #tag::expiry-replace[]
        auto new_content = tao::json::value{ { "foo", "bar" } };
        auto options = couchbase::replace_options().preserve_expiry(true);
        auto [err, result] = collection.replace("document-key", new_content, options).get();
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Document with expiry replaced successfully");
        }
        // #end::expiry-replace[]
    }

    {
        // #tag::expiry-get-and-touch[]
        auto [err, result] = collection.get_and_touch("document-key", std::chrono::hours(4)).get();
        if (err) {
            fmt::println("Error: {}", err);
        } else {
            fmt::println("Document fetched and expiry updated");
        }
        // #end::expiry-get-and-touch[]
    }

    {
        // #tag::counters[]
        {
            auto options = couchbase::increment_options().delta(1).initial(1);
            auto [err, result] = collection.binary().increment("document-key6", options).get();
            if (err) {
                fmt::println("Error: {}", err);
            } else {
                fmt::println("Counter now: {}", result.content());
            }
        }
        {
            auto options = couchbase::decrement_options().delta(1).initial(10);
            auto [err, result] = collection.binary().decrement("document-key6", options).get();
            if (err) {
                fmt::println("Error: {}", err);
            } else {
                fmt::println("Counter now: {}", result.content());
            }
        }
        // #end::counters[]
    }

    {
        // #tag::remove_with_durability[]
        std::string document_id{ "document_key" };
        auto options = couchbase::remove_options().durability(couchbase::durability_level::majority);

        auto [err, res] = collection.remove(document_id, options).get();

        if (err.ec() == couchbase::errc::key_value::document_not_found) {
            fmt::println("The document does not exist");
        } else if (err) {
            fmt::println("Got error: {}", err);
        } else {
            fmt::println("id: {}, CAS: {}", document_id, res.cas().value());
        }
        // #end::remove_with_durability[]
    }

    cluster.close().get();
    return 0;
}

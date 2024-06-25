#include <couchbase/codec/raw_json_transcoder.hxx>

// #tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/cas.hxx>
#include <couchbase/fmt/error.hxx>

#include <fmt/format.h>
#include <tao/json/value.hpp>

#include <iostream>
// #end::imports[]

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "default" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

auto
main() -> int
{
    // #tag::cluster[]
    auto options = couchbase::cluster_options(username, password);
    options.apply_profile("wan_development");
    auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
    if (connect_err) {
        std::cout << "Unable to connect to the cluster: " << fmt::format("{}", connect_err) << "\n";
        return 1;
    }
    auto bucket = cluster.bucket(bucket_name);
    auto collection = bucket.scope(scope_name).collection(collection_name);

    // #end::cluster[]

    {
        // #tag::get[]
        auto [err, result] = collection
                               .lookup_in(
                                 "customer123",
                                 couchbase::lookup_in_specs{
                                   couchbase::lookup_in_specs::get("addresses.delivery.country"),
                                 }
                               )
                               .get();
        if (err) {
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            auto country = result.template content_as<std::string>(0);
            std::cout << fmt::format("Country: {}\n", country);
        }

        // #end::get[]
    }

    {
        // #tag::get-callback[]
        collection.lookup_in(
          "customer123",
          couchbase::lookup_in_specs{
            couchbase::lookup_in_specs::get("addresses.delivery.country"),
          },
          {},
          [](auto err, auto result) {
              if (err) {
                  std::cout << fmt::format("Error: {}\n", err);
              } else {
                  auto country = result.template content_as<std::string>(0);
                  std::cout << fmt::format("Country: {}\n", country);
              }
          }
        );

        // #end::get-callback[]
    }

    {
        // #tag::exists[]
        auto [err, result] = collection
                               .lookup_in(
                                 "customer123",
                                 couchbase::lookup_in_specs{
                                   couchbase::lookup_in_specs::exists("addresses.delivery.does_not_exist"),
                                 }
                               )
                               .get();
        if (err) {
            std::cout << fmt::format("Error: {}\n", err) << "\n";
        } else {
            std::cout << fmt::format("Does the field exist? {}\n", result.exists(0));
        }
        // #end::exists[]
    }

    {
        // #tag::combine[]
        auto [err, result] = collection
                               .lookup_in(
                                 "customer123",
                                 couchbase::lookup_in_specs{
                                   couchbase::lookup_in_specs::get("addresses.delivery.country"),
                                   couchbase::lookup_in_specs::exists("addresses.delivery.does_not_exist"),
                                 }
                               )
                               .get();
        if (err) {
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            std::cout << fmt::format("Country: {}, Exists: {}\n", result.content_as<std::string>(0), result.exists(1));
        }
        // #end::combine[]
    }

    {
        // #tag::upsert[]
        auto [err, result] = collection
                               .mutate_in(
                                 "customer123",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::upsert("email", "dougr86@hotmail.com"),
                                 }
                               )
                               .get();
        if (err) {
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            std::cout << fmt::format("Success! (CAS: {})\n", result.cas());
        }
        // #end::upsert[]
    }

    {
        // #tag::insert[]
        auto [err, result] = collection
                               .mutate_in(
                                 "customer123",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::insert("email", "dougr86@hotmail.com"),
                                 }
                               )
                               .get();
        if (err.ec() == couchbase::errc::key_value::path_exists) {
            std::cout << "Error, path already exists" << "\n";
        } else if (err) {
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            std::cout << "Unexpected success..." << "\n";
        }
        // #end::insert[]
    }

    {
        // #tag::mutation-combine[]
        auto [err, result] = collection
                               .mutate_in(
                                 "customer123",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::remove("addresses.billing"),
                                   couchbase::mutate_in_specs::replace("email", "dougr96@hotmail.com"),
                                 }
                               )
                               .get();

        // Note: for brevity, checking the result will be skipped in subsequent examples, but
        // obviously is necessary in a production application. #end::mutation-combine[]
    }

    {
        // #tag::array-append[]
        auto [err, result] = collection
                               .mutate_in(
                                 "customer123",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::array_append("purchases.complete", 777),
                                 }
                               )
                               .get();

        // purchases.complete is now [339, 976, 442, 666, 777]
        // #end::array-append[]
    }

    {
        // #tag::array-prepend[]
        auto [err, result] = collection
                               .mutate_in(
                                 "customer123",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::array_prepend("purchases.complete", 18),
                                 }
                               )
                               .get();
        // purchases.abandoned is now [18, 157, 49, 999]
        // #end::array-prepend[]
    }

    {
        // #tag::array-create[]
        auto [err1, result1] = collection.upsert("my_array", tao::json::empty_array).get();

        auto [err2, result2] = collection
                                 .mutate_in(
                                   "my_array",
                                   couchbase::mutate_in_specs{
                                     couchbase::mutate_in_specs::array_append("", "some element"),
                                   }
                                 )
                                 .get();
        // the document my_array is now ["some element"]
        // #end::array-create[]
    }

    {
        // #tag::array-upsert[]
        auto [err, result] = collection
                               .mutate_in(
                                 "some_doc",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::array_append("some.array", "hello world").create_path(),
                                 }
                               )
                               .get();
        // #end::array-upsert[]
    }

    {
        // #tag::array-unique[]
        auto [err1, result1] =
          collection
            .mutate_in("customer123", couchbase::mutate_in_specs{ couchbase::mutate_in_specs::array_add_unique("purchases.complete", 95) })
            .get();

        // Just for demo, a production app should check the result properly
        assert(!err1);

        auto [err2, result2] = collection
                                 .mutate_in(
                                   "customer123",
                                   couchbase::mutate_in_specs{
                                     couchbase::mutate_in_specs::array_add_unique("purchases.complete", 95),
                                   }
                                 )
                                 .get();
        if (err2.ec() == couchbase::errc::key_value::path_exists) {
            std::cout << "Error, path already exists" << "\n";
        } else if (err2) {
            std::cout << fmt::format("Error: {}\n", err2);
        } else {
            std::cout << "Unexpected success..." << "\n";
        }
        // #end::array-unique[]

        // Just resetting the array...
        std::vector<tao::json::value> arr = { 339, 976, 442, 666 };
        tao::json::value arr_json{};
        arr_json.set_array(arr);

        auto [err, result] = collection
                               .mutate_in(
                                 "customer123",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::upsert("purchases.complete", arr_json),
                                 }
                               )
                               .get();
    }

    {
        std::string content = "{\"foo\": {\"bar\": [10, 20]}}";
        auto [err0, result0] = collection.upsert<couchbase::codec::raw_json_transcoder>("some_doc", content).get();

        // #tag::array-insert[]
        auto [err, result] = collection
                               .mutate_in(
                                 "some_doc",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::array_insert("foo.bar[1]", "cruel"),
                                 }
                               )
                               .get();
        // #end::array-insert[]
    }

    {
        // #tag::counter-inc[]
        auto [err, result] = collection
                               .mutate_in(
                                 "some_doc",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::increment("logins", 1),
                                 }
                               )
                               .get();

        if (err) {
            std::cout << fmt::format("Error: {}\n", err);
        } else {
            auto count = result.content_as<std::int32_t>(0);
            std::cout << fmt::format("After increment, counter is {}\n", count);
        }
        // #end::counter-inc[]
    }

    {
        // #tag::counter-dec[]
        auto [err1, result1] = collection.upsert("player432", tao::json::value{ { "gold", 1000 } }).get();
        assert(!err1);

        auto [err2, result2] = collection
                                 .mutate_in(
                                   "player432",
                                   couchbase::mutate_in_specs{
                                     couchbase::mutate_in_specs::decrement("gold", 150),
                                   }
                                 )
                                 .get();

        if (err2) {
            std::cout << fmt::format("Error: {}\n", err2);
        } else {
            auto count = result2.content_as<std::int32_t>(0);
            std::cout << fmt::format("After decerement, counter is {}\n", count);
        }
        // #end::counter-dec[]
    }

    {
        // #tag::create-path[]
        auto [err, result] = collection
                               .mutate_in(
                                 "customer123",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::upsert(
                                     "level_0.level_1.foo.bar.phone",
                                     tao::json::value{
                                       { "num", "311-555-0101" },
                                       { "ext", 16 },
                                     }
                                   )
                                     .create_path(),
                                 }
                               )
                               .get();
        // #end::create-path[]
    }

    {
        // #tag::concurrent[]
        auto future1 = collection.mutate_in(
          "customer123",
          couchbase::mutate_in_specs{
            couchbase::mutate_in_specs::array_append("purchases.complete", 99),
          }
        );
        auto future2 = collection.mutate_in(
          "customer123",
          couchbase::mutate_in_specs{
            couchbase::mutate_in_specs::array_append("purchases.abandoned", 101),
          }
        );

        future1.get();
        future2.get();
        // #end::concurrent[]
    }

    {
        // #tag::cas[]
        auto [err1, result1] = collection.get("player432").get();

        assert(!err1);

        auto [err2, result2] = collection
                                 .mutate_in(
                                   "player432",
                                   couchbase::mutate_in_specs{
                                     couchbase::mutate_in_specs::decrement("gold", 150),
                                   },
                                   couchbase::mutate_in_options().cas(result1.cas())
                                 )
                                 .get();
        // #end::cas[]
    }

    {
        // #tag::durability[]
        auto [err, result] = collection
                               .mutate_in(
                                 "key",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::insert("name", "andy"),
                                 },
                                 couchbase::mutate_in_options().durability(couchbase::persist_to::one, couchbase::replicate_to::one)
                               )
                               .get();
        // #end::durability[]
    }

    {
        // #tag::sync-durability[]
        auto [err, result] = collection
                               .mutate_in(
                                 "key",
                                 couchbase::mutate_in_specs{
                                   couchbase::mutate_in_specs::insert("name", "andy"),
                                 },
                                 couchbase::mutate_in_options().durability(couchbase::durability_level::majority)
                               )
                               .get();
        // #end::sync-durability[]
    }

    cluster.close().get();
    return 0;
}

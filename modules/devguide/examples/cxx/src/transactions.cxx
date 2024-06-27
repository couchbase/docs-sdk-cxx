#include <couchbase/codec/raw_json_transcoder.hxx>

// #tag::imports[]
#include <couchbase/cluster.hxx>
#include <couchbase/fmt/cas.hxx>
#include <couchbase/fmt/error.hxx>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

#include <functional>
#include <iostream>
// #end::imports[]

static constexpr auto connection_string{ "couchbase://127.0.0.1" };
static constexpr auto username{ "Administrator" };
static constexpr auto password{ "password" };
static constexpr auto bucket_name{ "travel-sample" };
static constexpr auto scope_name{ couchbase::scope::default_name };
static constexpr auto collection_name{ couchbase::collection::default_name };

auto
main() -> int
{
    // #tag::cluster[]
    auto cluster_options = couchbase::cluster_options(username, password);
    cluster_options.apply_profile("wan_development");
    auto [connect_err, cluster] =
      couchbase::cluster::connect(connection_string, cluster_options).get();
    if (connect_err) {
        std::cout << "Unable to connect to the cluster: " << fmt::format("{}", connect_err) << "\n";
        return 1;
    }
    auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);
    // #end::cluster[]

    {
        collection.upsert("doc-b", tao::json::value{ { "foo", "bar" } }).get();
        collection.upsert("doc-c", tao::json::value{ { "foo", "bar" } }).get();

        // #tag::examples[]
        auto inventory = cluster.bucket("travel-sample").scope("inventory");

        auto [err, result] = cluster.transactions()->run(
          [&collection, &inventory](std::shared_ptr<couchbase::transactions::attempt_context> ctx
          ) -> couchbase::error {
              // Inserting a document
              ctx->insert(collection, "doc-a", tao::json::value{ { "foo", "bar" } });

              // Getting the document
              auto [err_a, doc_a] = ctx->get(collection, "doc-a");
              if (err_a.ec() == couchbase::errc::key_value::document_not_found) {
                  // This will not fail the transaction
                  fmt::println("Could not find document with key: doc-a");
              } else if (err_a) {
                  // Any other error will fail the transaction
                  fmt::println("Error getting doc-a: {}", err_a);
              }

              // Replacing a document
              auto [err_b, doc_b] = ctx->get(collection, "doc-b");
              if (err_b) {
                  fmt::println("Error getting doc-b: {}", err_b);
                  return err_b; // Abort the transaction
              }
              auto content = doc_b.content_as<tao::json::value>();
              content["transactions"] = "are awesome";
              ctx->replace(doc_b, content);

              // Removing a document
              auto [err_c, doc_c] = ctx->get(collection, "doc-c");
              ctx->remove(doc_c);

              // Performing a SELECT SQL++ query against a scope
              auto [query_err, query_res] = ctx->query(
                inventory,
                "SELECT * FROM hotel WHERE country = $1",
                couchbase::transactions::transaction_query_options().positional_parameters(
                  "United Kingdom"
                )
              );
              if (!query_err) {
                  auto rows = query_res.rows_as_json();
                  fmt::println("Query returned {} rows", rows.size());
              }

              // Performing an UPDATE SQL++ query on multiple documents in a scope
              ctx->query(
                inventory,
                "UPDATE route SET airlineid = $1 WHERE airline = $2",
                couchbase::transactions::transaction_query_options().positional_parameters(
                  "airline_137", "AF"
                )
              );

              return {};
          }
        );

        if (err) {
            fmt::println("Transaction finished with error: {}", err);
        } else {
            fmt::println("Transaction finished successfully");
        }

        // #end::examples[]
    }

    {
        collection.upsert("doc-b", tao::json::value{ { "foo", "bar" } }).get();
        collection.upsert("doc-c", tao::json::value{ { "foo", "bar" } }).get();

        // #tag::examples-async[]
        auto inventory = cluster.bucket("travel-sample").scope("inventory");

        auto barrier = std::make_shared<
          std::promise<std::pair<couchbase::error, couchbase::transactions::transaction_result>>>();
        auto fut = barrier->get_future();

        cluster.transactions()->run(
          [&collection,
           &inventory](std::shared_ptr<couchbase::transactions::async_attempt_context> ctx
          ) -> couchbase::error {
              auto err_barrier = std::make_shared<std::promise<couchbase::error>>();
              auto err_fut = err_barrier->get_future();

              // Inserting a document
              ctx->insert(
                collection,
                "doc-a",
                tao::json::value{ { "foo", "bar" } },
                [ctx, err_barrier, &collection, &inventory](auto err, auto doc_a) {
                    // Getting the document
                    ctx->get(
                      collection,
                      "doc-a",
                      [ctx, err_barrier, &collection, &inventory](auto err, auto doc_a) {
                          if (err.ec() == couchbase::errc::key_value::document_not_found) {
                              // This will not fail the transaction
                              fmt::println("Could not find document with key: doc-a");
                          } else if (err) {
                              // Any other error will fail the transaction
                              fmt::println("Error getting doc-a: {}", err);
                          }

                          // Replacing a document
                          ctx->get(
                            collection,
                            "doc-b",
                            [ctx, err_barrier, &collection, &inventory](auto err, auto doc_b) {
                                if (err) {
                                    fmt::println("Error getting doc-b: {}", err);
                                    err_barrier->set_value(err); // Abort the transaction
                                    return;
                                }
                                auto content = doc_b.template content_as<tao::json::value>();
                                content["transactions"] = "are awesome";
                                ctx->replace(
                                  doc_b,
                                  content,
                                  [ctx, err_barrier, &collection, &inventory](
                                    auto err, auto doc_b
                                  ) {
                                      // Removing a document
                                      ctx->get(
                                        collection,
                                        "doc-c",
                                        [ctx, err_barrier, &collection, &inventory](
                                          auto err, auto doc_c
                                        ) {
                                            ctx->remove(
                                              doc_c,
                                              [ctx, err_barrier, &inventory](auto err) {
                                                  // Performing a SELECT SQL++ query against a scope
                                                  ctx->query(
                                                    inventory,
                                                    "SELECT * FROM hotel WHERE country = $1",
                                                    couchbase::transactions::
                                                      transaction_query_options()
                                                        .positional_parameters("United Kingdom"),
                                                    [ctx,
                                                     err_barrier,
                                                     &inventory](auto err, auto res) {
                                                        if (!err) {
                                                            auto rows = res.rows_as_json();
                                                            fmt::println(
                                                              "Query returned {} rows", rows.size()
                                                            );
                                                        }

                                                        // Performing an UPDATE SQL++ query on
                                                        // multiple documents in a scope
                                                        ctx->query(
                                                          inventory,
                                                          "UPDATE route SET airlineid = $1 WHERE "
                                                          "airline = $2",
                                                          couchbase::transactions::
                                                            transaction_query_options()
                                                              .positional_parameters(
                                                                "airline_137", "AF"
                                                              ),
                                                          [err_barrier](auto err, auto res) {
                                                              err_barrier->set_value({});
                                                          }
                                                        );
                                                    }
                                                  );
                                              }
                                            );
                                        }
                                      );
                                  }
                                );
                            }
                          );
                      }
                    );
                }
              );

              return err_fut.get();
          },
          [barrier](auto err, auto result) { barrier->set_value({ err, result }); }
        );

        auto [err, result] = fut.get();
        if (err) {
            fmt::println("Transaction finished with error: {}", err);
        } else {
            fmt::println("Transaction finished successfully");
        }
        // #end::examples-async[]
    }

    {
        // #tag::insert[]
        auto [err, result] = cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            ctx->insert(collection, "doc_id", tao::json::value{ { "foo", "bar" } });
            return {};
        });
        // #end::insert[]
    }

    {
        auto barrier = std::make_shared<std::promise<couchbase::error>>();
        auto complete_handler = [barrier](auto err, auto res) { barrier->set_value(err); };
        auto fut = barrier->get_future();

        // #tag::insert-async[]
        cluster.transactions()->run(
          [&](auto ctx) -> couchbase::error {
              ctx->insert(
                collection,
                "doc_id",
                tao::json::value{ { "foo", "bar" } },
                [](auto err, auto doc) {
                    // Handle operation result/err
                }
              );
              return {};
          },
          std::move(complete_handler)
        );
        // #end::insert-async[]

        auto err = fut.get();
    }

    {
        // #tag::get[]
        auto [err, result] = cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            auto [err, doc] = ctx->get(collection, "doc_id");
            return {};
        });
        // #end::get[]
    }

    {
        // #tag::get-propagate-failure[]
        auto [err, result] = cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            auto [err, doc] = ctx->get(collection, "doc_id");
            if (err.ec() == couchbase::errc::key_value::document_not_found) {
                // By propagating this failure, the application will cause the transaction to be
                // rolled back.
                return err;
            }
            return {};
        });
        // #end::get-propagate-failure[]
    }

    {
        // #tag::get-ryow[]
        auto [err, result] = cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            ctx->insert(collection, "doc_id2", tao::json::value{ { "foo", "bar" } });
            auto [err, doc] = ctx->get(collection, "doc_id2");
            auto content = doc.template content_as<tao::json::value>();
            fmt::println("Got document: {}", tao::json::to_string(content));
            return {};
        });
        // #end::get-ryow[]

        collection.remove("doc_id2").get();
    }

    {
        // #tag::replace[]
        auto [err, result] = cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            auto [err, doc] = ctx->get(collection, "doc_id");
            if (err) {
                return {};
            }
            auto content = doc.template content_as<tao::json::value>();
            content["transactions"] = "are awesome";
            ctx->replace(doc, content);
            return {};
        });
        // #end::replace[]
    }

    {
        auto barrier = std::make_shared<std::promise<couchbase::error>>();
        auto complete_handler = [barrier](auto err, auto res) { barrier->set_value(err); };
        auto fut = barrier->get_future();

        // #tag::replace-async[]
        cluster.transactions()->run(
          [&](auto ctx) -> couchbase::error {
              ctx->get(collection, "doc_id", [ctx](auto err, auto doc) {
                  if (err) {
                      return;
                  }
                  auto content = doc.template content_as<tao::json::value>();
                  content["transactions"] = "are awesome";
                  ctx->replace(doc, content, [](auto err, auto doc) {});
              });
              return {};
          },
          std::move(complete_handler)
        );
        // #end::replace-async[]

        auto err = fut.get();
    }

    {
        // #tag::remove[]
        auto [err, result] = cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            auto [err, doc] = ctx->get(collection, "doc_id");
            if (err) {
                return {};
            }
            ctx->remove(doc);
            return {};
        });
        // #end::remove[]
    }

    {
        auto barrier = std::make_shared<std::promise<couchbase::error>>();
        auto complete_handler = [barrier](auto err, auto res) { barrier->set_value(err); };
        auto fut = barrier->get_future();

        // #tag::remove-async[]
        cluster.transactions()->run(
          [&](auto ctx) -> couchbase::error {
              ctx->get(collection, "doc_id", [ctx](auto err, auto doc) {
                  if (err) {
                      return;
                  }
                  ctx->remove(doc, [](auto err) {});
              });
              return {};
          },
          std::move(complete_handler)
        );
        // #end::remove-async[]

        auto err = fut.get();
    }

    {
        // #tag::query-select[]
        cluster.transactions()->run([](auto ctx) -> couchbase::error {
            std::string statement{
                "SELECT * FROM `travel-sample`.inventory.hotel WHERE country = $1"
            };
            auto [err, result] = ctx->query(
              statement,
              couchbase::transactions::transaction_query_options().positional_parameters(
                "United Kingdom"
              )
            );
            auto rows = result.rows_as_json();
            return {};
        });
        // #end::query-select[]

        // #tag::query-select-scope[]
        auto travel_sample = cluster.bucket("travel-sample");
        auto inventory = travel_sample.scope("inventory");

        cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            std::string statement{ "SELECT * FROM hotel WHERE country = $1" };
            auto [err, result] = ctx->query(
              inventory,
              statement,
              couchbase::transactions::transaction_query_options().positional_parameters(
                "United States"
              )
            );
            auto rows = result.rows_as_json();
            return {};
        });
        // #end::query-select-scope[]

        // #tag::query-update[]
        std::string hotel_chain{ "http://marriot%" };
        std::string country{ "United States" };

        cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            std::string statement{
                "UPDATE hotel SET price = $1 WHERE url LIKE $2 AND country = $3"
            };
            auto options =
              couchbase::transactions::transaction_query_options().positional_parameters(
                99, 99, hotel_chain, country
              );
            auto [err, result] = ctx->query(inventory, statement, options);
            fmt::println(
              "Mutation Count = {}", result.meta_data().metrics().value().mutation_count()
            );
            return {};
        });
        // #end::query-update[]

        auto price_from_recent_reviews = [](couchbase::transactions::transaction_query_result res) {
            return double(1.23);
        };

        // #tag::query-complex[]
        cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            // Find all hotels of the chain
            auto [err, result] = ctx->query(
              inventory,
              "SELECT reviews FROM hotel WHERE url LIKE $1 AND country = $2",
              couchbase::transactions::transaction_query_options().positional_parameters(
                hotel_chain, country
              )
            );

            // This function (not provided here) will use a trained machine learning model to
            // provide a suitable price based on recent customer reviews.
            auto updated_price = price_from_recent_reviews(result);

            // Set the price of al hotels in the chain
            ctx->query(
              inventory,
              "UPDATE hotel SET price = $1 WHERE url LIKE $2 AND country = $3",
              couchbase::transactions::transaction_query_options().positional_parameters(
                updated_price, hotel_chain, country
              )
            );

            return {};
        });
        // #end::query-complex[]
    }

    {
        // #tag::query-insert[]
        cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            ctx->query("INSERT INTO `default` VALUES ('doc', {'hello':'world'})"); // <1>

            // Performing a 'Read Your Own Write'
            auto [err, result] =
              ctx->query("SELECT `default`.* FROM `default` WHERE META().id = 'doc'"); // <2>
            fmt::println("Result Count = {}", result.meta_data().metrics().value().result_count());

            return {};
        });
        // #end::query-insert[]
    }

    {
        // #tag::query-kv-mix[]
        cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            ctx->insert(collection, "doc", tao::json::value{ { "hello", "world" } }); // <1>

            // Performing a 'Read Your Own Write'
            auto [err, result] =
              ctx->query("SELECT `default`.* FROM `default` WHERE META().id = 'doc'"); // <2>
            fmt::println("Result Count = {}", result.meta_data().metrics().value().result_count());

            return {};
        });
        // #end::query-kv-mix[]
    }

    {
        auto barrier = std::make_shared<std::promise<couchbase::error>>();
        auto complete_handler = [barrier](auto err, auto res) { barrier->set_value(err); };
        auto fut = barrier->get_future();

        // #tag::concurrent-ops[]
        std::vector<std::string> doc_ids = { "doc1", "doc2", "doc3", "doc4", "doc5" };

        // #end::concurrent-ops[]

        for (auto doc_id : doc_ids) {
            collection.upsert(doc_id, tao::json::value{ { "foo", "bar" } }).get();
        }

        // #tag::concurrent-ops[]
        cluster.transactions()->run(
          [&](auto ctx) -> couchbase::error {
              for (auto doc_id : doc_ids) {
                  ctx->get(collection, doc_id, [ctx](auto err, auto doc) {
                      if (err) {
                          return;
                      }
                      auto content = doc.template content_as<tao::json::value>();
                      content["value"] = "updated";
                      ctx->replace(doc, content, [](auto err, auto doc) {
                          if (err) {
                              fmt::println("Error: {}", err);
                          } else {
                              fmt::println("Replaced document successfully");
                          }
                      });
                  });
              }

              return {};
          },
          std::move(complete_handler)
        );
        // #end::concurrent-ops[]
        fut.get();
    }

    {
        // #tag::config[]
        auto opts =
          couchbase::cluster_options(username, password)
            .transactions()
            .durability_level(couchbase::durability_level::persist_to_majority)
            .cleanup_config(couchbase::transactions::transactions_cleanup_config().cleanup_window(
              std::chrono::seconds(30)
            ));
        // #end::config[]
    }

    {
        tao::json::value doc1_content{ { "foo", "bar" } };
        tao::json::value doc2_content{ { "foo", "baz" } };
        // #tag::create-simple[]
        cluster.transactions()->run([&](auto ctx) -> couchbase::error {
            ctx->insert(collection, "doc1", doc1_content);
            auto [err, doc2] = ctx->get(collection, "doc2");
            ctx->replace(doc2, doc2_content);
            return {};
        });
        // #end::create-simple[]
    }

    {
        // #tag::custom-metadata[]
        auto opts = couchbase::cluster_options(username, password)
                      .transactions()
                      .metadata_collection(couchbase::transactions::transaction_keyspace{
                        "bucketName",
                        "scopeName",
                        "collectionName",
                      });
        // #end::custom-metadata[]
    }

    {
        // #tag::custom-metadata-per[]
        cluster.transactions()->run(
          [&](auto ctx) -> couchbase::error { return {}; },
          couchbase::transactions::transaction_options().metadata_collection(collection)
        );
        // #end::custom-metadata-per[]
    }

    {
        // #tag::config-cleanup[]
        auto opts = couchbase::cluster_options(username, password)
                      .transactions()
                      .cleanup_config(couchbase::transactions::transactions_cleanup_config()
                        .cleanup_client_attempts(true)
                        .cleanup_lost_attempts(true)
                        .cleanup_window(std::chrono::seconds(120))
                        .add_collection({ "bucketName", "scopeName", "collectionName" })
                      );
        // #end::config-cleanup[]
    }

    cluster.close().get();
    return 0;
}

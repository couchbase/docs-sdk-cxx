#include <couchbase/cluster.hxx>
#include <couchbase/codec/tao_json_serializer.hxx>

#include <couchbase_encryption/aead_aes_256_cbc_hmac_sha512_provider.hxx>
#include <couchbase_encryption/default_manager.hxx>
#include <couchbase_encryption/default_transcoder.hxx>
#include <couchbase_encryption/document.hxx>
#include <couchbase_encryption/insecure_keyring.hxx>

#include <fmt/format.h>
#include <tao/json/to_string.hpp>
#include <tao/json/value.hpp>

#include <couchbase/fmt/error.hxx>

#include <string>
#include <vector>

auto
make_bytes(std::vector<unsigned char> v) -> std::vector<std::byte>
{
    std::vector<std::byte> out{ v.size() };
    std::transform(v.begin(), v.end(), out.begin(), [](int c) {
        return static_cast<std::byte>(c);
    });
    return out;
}

// tag::person_struct[]
struct person {
    struct address {
        std::string street;
        std::string city;
        std::string state;
        std::string zip;
    };

    std::string first_name;
    std::string last_name;
    std::string password;
    address address;
    std::string phone_number;

    // Define the fields that should be encrypted, with the paths they will appear with in the
    // serialized JSON document. If no encrypter alias is specified, the field will be encrypted
    // with the default encrypter.
    static const inline std::vector<couchbase::crypto::encrypted_field> encrypted_fields{
        { /* .field_path = */ { "password" }, /* .encrypter_alias = */ { "my-other-encrypter" } },
        { /* .field_path = */ { "address", "street" },
          /* .encrypter_alias = */ { "my-encrypter" } },
        { /* .field_path = */ { "address", "zip" } },
        { /* .field_path = */ { "phone" } },
    };
};
// end::person_struct[]

/*
 Define serialization/deserialization with the tao json transcoder, as the default transcoder will
 be used.
 */
template<>
struct tao::json::traits<person> {
    template<template<typename...> class Traits>
    static void assign(tao::json::basic_value<Traits>& v, const person& p)
    {
        v["first_name"] = p.first_name;
        v["last_name"] = p.last_name;
        v["password"] = p.password;
        v["address"] = { { "street", p.address.street },
                         { "city", p.address.city },
                         { "state", p.address.state },
                         { "zip", p.address.zip } };
        v["phone"] = p.phone_number;
    }

    template<template<typename...> class Traits>
    static auto as(const tao::json::basic_value<Traits>& v) -> person
    {
        person result;

        result.first_name = v.at("first_name").get_string();
        result.last_name = v.at("last_name").get_string();
        result.password = v.at("password").get_string();
        result.address.street = v.at("address").at("street").get_string();
        result.address.city = v.at("address").at("city").get_string();
        result.address.state = v.at("address").at("state").get_string();
        result.address.zip = v.at("address").at("zip").get_string();
        result.phone_number = v.at("phone").get_string();

        return result;
    }
};

template<>
struct fmt::formatter<person> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const person& p, FormatContext& ctx) const
    {
        return format_to(
          ctx.out(),
          "First name:   {}\n"
          "Last name:    {}\n"
          "Password:     {}\n"
          "Address:      {}, {}, {}, {}\n"
          "Phone number: {}",
          p.first_name,
          p.last_name,
          p.password,
          p.address.street,
          p.address.city,
          p.address.state,
          p.address.zip,
          p.phone_number
        );
    }
};

int
main()
{
    std::string connection_string{ "couchbase://192.168.106.128" };
    std::string user_name{ "Administrator" };
    std::string password{ "password" };
    std::string bucket_name{ "default" };
    std::string scope_name{ couchbase::scope::default_name };
    std::string collection_name{ couchbase::collection::default_name };

    // tag::keyring[]
    const std::vector<std::byte> key_a = make_bytes(
      {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
        0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
        0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
      }
    );

    const std::vector<std::byte> key_b = make_bytes(
      {
        0x3f, 0x3e, 0x3d, 0x3c, 0x3b, 0x3a, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33,
        0x32, 0x31, 0x30, 0x2f, 0x2e, 0x2d, 0x2c, 0x2b, 0x2a, 0x29, 0x28, 0x27, 0x26,
        0x25, 0x24, 0x23, 0x22, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19,
        0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10, 0x0f, 0x0e, 0x0d, 0x0c,
        0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
      }
    );

    // Create an insecure keyring and add two keys.
    const auto keyring = std::make_shared<couchbase::crypto::insecure_keyring>();
    keyring->add_key(couchbase::crypto::key("my-key", key_a));
    keyring->add_key(couchbase::crypto::key("my-other-key", key_b));
    // end::keyring[]

    // tag::provider[]
    // Create a provider.
    // AES-256 authenticated with HMAC SHA-512. Requires a 64-byte key.
    const auto provider = couchbase::crypto::aead_aes_256_cbc_hmac_sha512_provider(keyring);

    // Create the crypto manager.
    auto manager = std::make_shared<couchbase::crypto::default_manager>();

    // Create encrypters and register them with the manager.
    // The key ID is used by the encrypter to look up the key from the keyring when encrypting a
    // document.
    auto encrypter_a = provider.encrypter_for_key("my-key");
    auto encrypter_b = provider.encrypter_for_key("my-other-key");
    manager->register_encrypter("my-encrypter", encrypter_a);
    manager->register_encrypter("my-other-encrypter", encrypter_b);

    // We don't need to specify a default encrypter, but if we do, then any fields that don't
    // specify an encrypter, will use this encrypter.
    manager->register_default_encrypter(provider.encrypter_for_key("my-key"));

    // We only set one decrypter per algorithm.
    // The crypto manager will work out which decrypter to use based on the `alg` field embedded in
    // the encrypter field data. The decrypter will fetch the necessary key from the keyring using
    // the key ID embedded in the encrypted field data.
    manager->register_decrypter(provider.decrypter());
    // end::provider[]

    // tag::cluster_options[]
    // Supply the crypto manager when connecting to the cluster.
    auto options = couchbase::cluster_options(user_name, password).crypto_manager(manager);
    auto [connect_err, cluster] = couchbase::cluster::connect(connection_string, options).get();
    // end::cluster_options[]
    if (connect_err) {
        fmt::println("Cluster connect failed {}", connect_err);
        return 1;
    }

    {
        // tag::person_upsert[]
        auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

        const auto person1 = person{
            /* .first_name = */ "John",
            /* .last_name = */ "Doe",
            /* .password = */ "password123",
            /* .address = */
            {
              /* .street = */ "999 Street St.",
              /* .city = */ "Some City",
              /* .state = */ "ST",
              /* .zip = */ "12345",
            },
            /* .phone_number = */ "12345678",
        };

        // To encrypt the document a crypto transcoder (such as the provided
        // `crypto::default_transcoder`) must be used. The crypto transcoder uses the crypto manager
        // specified via the cluster options to encrypt the fields that were specified.
        const auto [err, upsert_res] =
          collection.upsert<couchbase::crypto::default_transcoder>("person-1", person1).get();
        if (err) {
            fmt::println("Failed to upsert the document {}", err);
        }
        // end::person_upsert[]
    }

    auto collection = cluster.bucket(bucket_name).scope(scope_name).collection(collection_name);

    {
        // tag::person_get[]
        const auto [err, get_res] = collection.get("person-1").get();
        if (err) {
            fmt::println("Failed to get the document {}", err);
        }

        // Decoding the content with a standard non-crypto transcoder will return the encrypted
        // document
        const auto encrypted_document = get_res.content_as<tao::json::value>();
        fmt::println("{}", tao::json::to_string(encrypted_document, 2));
        // end::person_get[]

        // tag::person_get_decrypt[]
        // Decoding the content with the crypto transcoder will decrypt the encrypted fields.
        const auto decrypted_person =
          get_res.content_as<person, couchbase::crypto::default_transcoder>();
        fmt::println("{}", decrypted_person);

        // end::person_get_decrypt[]
    }

    {
        // tag::crypto_doc_upsert[]
        tao::json::value content{
            { "num", 20 },
            { "message", "This is a secret!" },
        };

        const auto crypto_doc =
          couchbase::crypto::document<tao::json::value>::from(std::move(content))
            .with_encrypted_field({ "message" }, "my-encrypter");

        const auto [err, upsert_res] =
          collection.upsert<couchbase::crypto::default_transcoder>("my-crypto-doc", crypto_doc)
            .get();
        if (err) {
            fmt::println("Failed to upsert the document {}", err);
        }
        // end::crypto_doc_upsert[]
    }
    {
        // tag::crypto_doc_get[]
        const auto [err, get_res] = collection.get("my-crypto-doc").get();
        if (err) {
            fmt::println("Failed to get the document {}", err);
        }

        // Decoding the content with a standard non-crypto transcoder will return the encrypted
        // document
        const auto encrypted_document = get_res.content_as<tao::json::value>();
        fmt::println("{}", tao::json::to_string(encrypted_document, 2));
        // end::crypto_doc_get[]

        // tag::crypto_doc_get_decrypt[]
        // Decoding the content with the crypto transcoder will decrypt the encrypted fields.
        const auto decrypted_document =
          get_res.content_as<tao::json::value, couchbase::crypto::default_transcoder>();
        fmt::println("{}", tao::json::to_string(decrypted_document, 2));
        // end::crypto_doc_get_decrypt[]
    }

    return 0;
}

// clang-format off
/*
Expected output
===============

// tag::encrypted_document[]
{
  "address": {
    "city": "Some City",
    "encrypted$street": {
      "alg": "AEAD_AES_256_CBC_HMAC_SHA512",
      "ciphertext": "dYo1KbbRTcvC3RgLh7slLaNXjSBT8BZ8tXVGHHbw1yi5r2OtiYmm5tagoOE61L6loCNUOqB5nr4ikDAeZb9ljSJBmIRB6p7ik29tt0MrvX8=",
      "kid": "my-key"
    },
    "encrypted$zip": {
      "alg": "AEAD_AES_256_CBC_HMAC_SHA512",
      "ciphertext": "4gMGy74p3xicKxLnRq2kiZjwolBeidA53oiQy+FLIhPUA93O7f+SiwcHBX5eWcu114O6dX2NOvq6ScIRGFwMhQ==",
      "kid": "my-key"
    },
    "state": "ST"
  },
  "encrypted$password": {
    "alg": "AEAD_AES_256_CBC_HMAC_SHA512",
    "ciphertext": "HKXofQdg7ghNM2htiMTzwWbcRRWCrKUApK202tAv8kQOENRsFlxCntM7EsJmnGTui8zYHNGbJcC+5kxDXw+XVA==",
    "kid": "my-other-key"
  },
  "encrypted$phone": {
    "alg": "AEAD_AES_256_CBC_HMAC_SHA512",
    "ciphertext": "JE5URIugqSEp9Yp56/lje4MpHOZyXHVX6V+/AcIndfJ9XmZCPGL9olXUdT2b0mFgnGXO8I6phJTfnTBpdqmh4Q==",
    "kid": "my-key"
  },
  "first_name": "John",
  "last_name": "Doe"
}
// end::encrypted_document[]

// tag::decrypted_document[]
First name:   John
Last name:    Doe
Password:     password123
Address:      999 Street St., Some City, ST, 12345
Phone number: 12345678
// end::decrypted_document[]

// tag::encrypted_crypto_doc[]
{
  "encrypted$message": {
    "alg": "AEAD_AES_256_CBC_HMAC_SHA512",
    "ciphertext": "DOdagMvAi8gE4LcNx++z8ghSavOL/WYQ5MlcHkVtzgT9MfZJB/brTdW14Ja851jZOD2GYt3mRJbM1nGIvU+iSYLcgPi45cMwvilqVI+2TiA=",
    "kid": "my-key"
  },
  "num": 20
}
// end::encrypted_crypto_doc[]
// tag::decrypted_crypto_doc[]
{
  "message": "This is a secret!",
  "num": 20
}
// end::decrypted_crypto_doc[]
*/
// clang-format on

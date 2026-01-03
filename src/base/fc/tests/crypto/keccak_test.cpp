#include <boost/test/unit_test.hpp>

#include <fc/crypto/keccak256.hpp>
#include <fc/exception/exception.hpp>

#include <iostream>

// Keccak-256 test vectors from Ethereum
// These are the original Keccak-256 values, NOT SHA3-256
static const std::string TEST1("abc");
static const std::string TEST2("");
static const std::string TEST3("The quick brown fox jumps over the lazy dog");
static const std::string TEST4("The quick brown fox jumps over the lazy dog.");

template<typename H>
void test_keccak( const char* to_hash, const std::string& expected ) {
    H hash = H::hash( to_hash, strlen( to_hash ) );
    BOOST_CHECK_EQUAL( expected, (std::string) hash );
    H hash2( expected );
    BOOST_CHECK( hash == hash2 );
}

template<typename H>
void test_keccak( const std::string& to_hash, const std::string& expected ) {
    H hash = H::hash( to_hash );
    BOOST_CHECK_EQUAL( expected, (std::string) hash );
    test_keccak<H>( to_hash.c_str(), expected );
}

template<typename H>
void test_keccak_stream( ) {
    H hash( TEST1 );
    std::stringstream stream;
    stream << hash;

    H other;
    stream >> other;
    BOOST_CHECK( hash == other );
}

template<typename H>
void test_keccak_encoder() {
    typename H::encoder enc;
    enc.write( TEST1.c_str(), TEST1.size() );
    H hash = enc.result();

    H expected = H::hash( TEST1 );
    BOOST_CHECK( hash == expected );

    // Test reset
    enc.reset();
    enc.write( TEST2.c_str(), TEST2.size() );
    H hash2 = enc.result();
    BOOST_CHECK( hash2 == H::hash( TEST2 ) );
}

template<typename H>
void test_keccak_operators() {
    H hash1 = H::hash( TEST1 );
    H hash2 = H::hash( TEST3 );

    // Test comparison operators
    BOOST_CHECK( hash1 != hash2 );
    BOOST_CHECK( hash1 == hash1 );

    // Test XOR operator
    H xor_result = hash1 ^ hash2;
    BOOST_CHECK( xor_result != hash1 );
    BOOST_CHECK( xor_result != hash2 );

    // Test shift operators
    H shifted_left = hash1 << 1;
    H shifted_right = hash1 >> 1;
    BOOST_CHECK( shifted_left != hash1 );
    BOOST_CHECK( shifted_right != hash1 );
}

BOOST_AUTO_TEST_SUITE( fc_keccak )

BOOST_AUTO_TEST_CASE( keccak256_hashing )
{
    test_keccak<fc::keccak256>( TEST2, "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470" );
    test_keccak<fc::keccak256>( TEST1, "4e03657aea45a94fc7d47ba826c8d667c0d1e6e33a64a036ec44f58fa12d6c45" );
    test_keccak<fc::keccak256>( TEST3, "4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15" );
    test_keccak<fc::keccak256>( TEST4, "578951e24efd62a3d63a86f7cd19aaa53c898fe287d2552133220370240b572d" );
    test_keccak_encoder<fc::keccak256>();
    test_keccak_operators<fc::keccak256>();
    test_keccak_stream<fc::keccak256>();

    // Test constructor from data
    fc::keccak256 digest = fc::keccak256::hash( TEST1 );
    fc::keccak256 other( digest.data(), digest.data_size() );
    BOOST_CHECK( digest == other );

    // Test exception on wrong size
    fc::keccak256 yet_another = fc::keccak256::hash( TEST3 );
    try {
        fc::keccak256 bad_hash( yet_another.data(), yet_another.data_size() / 2 );
        BOOST_FAIL( "Expected exception!" );
    } catch ( fc::exception& expected ) {}

    // Test variant conversion
    fc::variant v;
    to_variant( digest, v );
    from_variant( v, other );
    BOOST_CHECK( digest == other );
}

BOOST_AUTO_TEST_SUITE_END()

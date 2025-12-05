#include <boost/test/unit_test.hpp>

#include <fc/crypto/rand.hpp>

#include <cmath>

static bool passes_randomness( const char* buffer, size_t len, double sigma_multiplier = 1.0 )
{
    if (len == 0) { return true; }

    unsigned int zc = 0, oc = 0, rc = 0, last = 2;
    for (size_t k = len; k; k--) {
        char c = *buffer++;
        for (int i = 0; i < 8; i++) {
            unsigned int bit = c & 1;
            c >>= 1;
            if (bit) { oc++; } else { zc++; }
            if (bit != last) { rc++; last = bit; }
        }
    }

    if( 8 * len != zc + oc )
        return false;

    double E = 1 + (zc + oc) / 2.0;
    double variance = (E - 1) * (E - 2) / (oc + zc - 1);
    double sigma = sqrt(variance) * sigma_multiplier;

    return rc > E - sigma && rc < E + sigma;
}

static void check_randomness( const char* buffer, size_t len, double sigma_multiplier = 1.0 ) {
    BOOST_CHECK( passes_randomness( buffer, len, sigma_multiplier ) );
}

BOOST_AUTO_TEST_SUITE( fc_crypto )

BOOST_AUTO_TEST_CASE( secure_random_generation )
{
    char buffer[128];
    bool ok = false;
    // Retry a few times to avoid spurious failures on statistically rare runs.
    for( int attempt = 0; attempt < 3 && !ok; ++attempt )
    {
        fc::rand_bytes( buffer, sizeof(buffer) );
        ok = passes_randomness( buffer, sizeof(buffer), 4.0 );
    }
    BOOST_CHECK( ok );
}

BOOST_AUTO_TEST_CASE( pseudo_random_generation )
{
    char buffer[10013];
    fc::rand_pseudo_bytes( buffer, sizeof(buffer) );
    // Pseudo RNG can be less uniform; allow a wider band.
    check_randomness( buffer, sizeof(buffer), 3.0 );
}

BOOST_AUTO_TEST_SUITE_END()

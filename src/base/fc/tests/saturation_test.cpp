
#include <boost/test/unit_test.hpp>

#include <fc/saturation.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace {

template< typename Signed, typename BiggerSigned >
Signed clamp_to_range( BiggerSigned value )
{
   const Signed min = std::numeric_limits< Signed >::min();
   const Signed max = std::numeric_limits< Signed >::max();

   if( value < min )
      return min;
   if( value > max )
      return max;
   return static_cast< Signed >( value );
}

template< typename Signed >
std::vector< Signed > edge_values()
{
   const Signed min = std::numeric_limits< Signed >::min();
   const Signed max = std::numeric_limits< Signed >::max();

   return {
      min,
      static_cast< Signed >( min + 1 ),
      static_cast< Signed >( -100 ),
      static_cast< Signed >( -10 ),
      static_cast< Signed >( -1 ),
      static_cast< Signed >( 0 ),
      static_cast< Signed >( 1 ),
      static_cast< Signed >( 10 ),
      static_cast< Signed >( 100 ),
      static_cast< Signed >( max - 1 ),
      max
   };
}

template< typename Signed, typename BiggerSigned >
void verify_pairs( const std::vector< Signed >& values )
{
   for( Signed x : values )
   {
      for( Signed y : values )
      {
         const BiggerSigned add = static_cast< BiggerSigned >( x ) + static_cast< BiggerSigned >( y );
         const BiggerSigned sub = static_cast< BiggerSigned >( x ) - static_cast< BiggerSigned >( y );

         const Signed expected_add = clamp_to_range< Signed, BiggerSigned >( add );
         const Signed expected_sub = clamp_to_range< Signed, BiggerSigned >( sub );

         BOOST_CHECK_EQUAL( expected_add, fc::signed_sat_add( x, y ) );
         BOOST_CHECK_EQUAL( expected_sub, fc::signed_sat_sub( x, y ) );
      }
   }
}

} // namespace

BOOST_AUTO_TEST_SUITE( fc_saturation )

BOOST_AUTO_TEST_CASE( int8_add_and_subtract )
{
   verify_pairs< int8_t, int16_t >( edge_values< int8_t >() );
}

BOOST_AUTO_TEST_CASE( int16_add_and_subtract )
{
   verify_pairs< int16_t, int32_t >( edge_values< int16_t >() );
}

BOOST_AUTO_TEST_SUITE_END()

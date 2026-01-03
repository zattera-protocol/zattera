#pragma once
#include <fc/fwd.hpp>
#include <fc/string.hpp>
#include <fc/platform_independence.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc
{

/**
 * Keccak-256 hash (Ethereum compatible)
 *
 * This is the original Keccak-256 algorithm used by Ethereum,
 * NOT the finalized SHA3-256 standard.
 */
class keccak256
{
  public:
    keccak256();
    explicit keccak256( const string& hex_str );
    explicit keccak256( const char *data, size_t size );

    string str()const;
    operator string()const;

    char*    data()const;
    size_t data_size()const { return 256 / 8; }

    static keccak256 hash( const char* d, uint32_t dlen );
    static keccak256 hash( const string& );
    static keccak256 hash( const keccak256& );

    class encoder
    {
      public:
        encoder();
        ~encoder();

        void write( const char* d, uint32_t dlen );
        void put( char c ) { write( &c, 1 ); }
        void reset();
        keccak256 result();

      private:
        struct      impl;
        fc::fwd<impl,256> my;
    };

    template<typename T>
    inline friend T& operator<<( T& ds, const keccak256& ep ) {
      ds.write( ep.data(), sizeof(ep) );
      return ds;
    }

    template<typename T>
    inline friend T& operator>>( T& ds, keccak256& ep ) {
      ds.read( ep.data(), sizeof(ep) );
      return ds;
    }
    friend keccak256 operator << ( const keccak256& h1, uint32_t i       );
    friend keccak256 operator >> ( const keccak256& h1, uint32_t i       );
    friend bool   operator == ( const keccak256& h1, const keccak256& h2 );
    friend bool   operator != ( const keccak256& h1, const keccak256& h2 );
    friend keccak256 operator ^  ( const keccak256& h1, const keccak256& h2 );
    friend bool   operator >= ( const keccak256& h1, const keccak256& h2 );
    friend bool   operator >  ( const keccak256& h1, const keccak256& h2 );
    friend bool   operator <  ( const keccak256& h1, const keccak256& h2 );

    uint64_t _hash[4];
};

  typedef keccak256 uint256_keccak;

  class variant;
  void to_variant( const keccak256& bi, variant& v );
  void from_variant( const variant& v, keccak256& bi );

} // fc

namespace std
{
    template<>
    struct hash<fc::keccak256>
    {
       size_t operator()( const fc::keccak256& s )const
       {
           return  *((size_t*)&s);
       }
    };
}

#include <fc/reflect/reflect.hpp>
FC_REFLECT_TYPENAME( fc::keccak256 )

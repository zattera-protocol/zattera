#include <fc/crypto/hex.hpp>
#include <fc/fwd_impl.hpp>
#include <string.h>
#include <fc/crypto/keccak256.hpp>
#include <fc/variant.hpp>
#include <fc/exception/exception.hpp>
#include "_digest_common.hpp"

extern "C" {
#include "../vendor/tiny-keccak/keccak.h"
}

namespace fc {

    keccak256::keccak256() { memset( _hash, 0, sizeof(_hash) ); }

    keccak256::keccak256( const char *data, size_t size ) {
       if (size != sizeof(_hash))
          FC_THROW_EXCEPTION( exception, "keccak256: size mismatch" );
       memcpy(_hash, data, size );
    }

    keccak256::keccak256( const string& hex_str ) {
      fc::from_hex( hex_str, (char*)_hash, sizeof(_hash) );
    }

    string keccak256::str()const {
      return fc::to_hex( (char*)_hash, sizeof(_hash) );
    }

    keccak256::operator string()const { return  str(); }

    char* keccak256::data()const { return (char*)&_hash[0]; }


    struct keccak256::encoder::impl {
       std::vector<char> buffer;
    };

    keccak256::encoder::~encoder() {}

    keccak256::encoder::encoder() {
      reset();
    }

    keccak256 keccak256::hash( const char* d, uint32_t dlen ) {
      encoder e;
      e.write(d,dlen);
      return e.result();
    }

    keccak256 keccak256::hash( const string& s ) {
      return hash( s.c_str(), s.size() );
    }

    keccak256 keccak256::hash( const keccak256& s )
    {
        return hash( s.data(), sizeof( s._hash ) );
    }

    void keccak256::encoder::write( const char* d, uint32_t dlen ) {
      my->buffer.insert(my->buffer.end(), d, d + dlen);
    }

    keccak256 keccak256::encoder::result() {
      keccak256 h;
      keccak_256((const uint8_t*)my->buffer.data(), my->buffer.size(), (uint8_t*)h.data());
      return h;
    }

    void keccak256::encoder::reset() {
      my->buffer.clear();
    }

    keccak256 operator << ( const keccak256& h1, uint32_t i ) {
      keccak256 result;
      fc::detail::shift_l( h1.data(), result.data(), result.data_size(), i );
      return result;
    }

    keccak256 operator >> ( const keccak256& h1, uint32_t i ) {
      keccak256 result;
      fc::detail::shift_r( h1.data(), result.data(), result.data_size(), i );
      return result;
    }

    keccak256 operator ^ ( const keccak256& h1, const keccak256& h2 ) {
      keccak256 result;
      result._hash[0] = h1._hash[0] ^ h2._hash[0];
      result._hash[1] = h1._hash[1] ^ h2._hash[1];
      result._hash[2] = h1._hash[2] ^ h2._hash[2];
      result._hash[3] = h1._hash[3] ^ h2._hash[3];
      return result;
    }

    bool operator >= ( const keccak256& h1, const keccak256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) >= 0;
    }

    bool operator > ( const keccak256& h1, const keccak256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) > 0;
    }

    bool operator < ( const keccak256& h1, const keccak256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) < 0;
    }

    bool operator != ( const keccak256& h1, const keccak256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) != 0;
    }

    bool operator == ( const keccak256& h1, const keccak256& h2 ) {
      return memcmp( h1._hash, h2._hash, sizeof(h1._hash) ) == 0;
    }

  void to_variant( const keccak256& bi, variant& v )
  {
     v = std::vector<char>( (const char*)&bi, ((const char*)&bi) + sizeof(bi) );
  }

  void from_variant( const variant& v, keccak256& bi )
  {
    std::vector<char> ve = v.as< std::vector<char> >();
    if( ve.size() )
    {
        memcpy(&bi, ve.data(), fc::min<size_t>(ve.size(),sizeof(bi)) );
    }
    else
        memset( &bi, char(0), sizeof(bi) );
  }

} //end namespace fc

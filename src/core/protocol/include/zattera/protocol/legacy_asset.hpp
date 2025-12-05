#pragma once

#include <zattera/protocol/asset.hpp>

#define ZTR_SYMBOL_LEGACY_SER_1   (uint64_t(1) | (ZTR_SYMBOL_U64 << 8))
#define ZTR_SYMBOL_LEGACY_SER_2   (uint64_t(2) | (ZTR_SYMBOL_U64 << 8))
#define ZTR_SYMBOL_LEGACY_SER_3   (uint64_t(5) | (ZTR_SYMBOL_U64 << 8))
#define ZTR_SYMBOL_LEGACY_SER_4   (uint64_t(3) | (uint64_t('0') << 8) | (uint64_t('.') << 16) | (uint64_t('0') << 24) | (uint64_t('0') << 32) | (uint64_t('1') << 40))
#define ZTR_SYMBOL_LEGACY_SER_5   (uint64_t(3) | (uint64_t('6') << 8) | (uint64_t('.') << 16) | (uint64_t('0') << 24) | (uint64_t('0') << 32) | (uint64_t('0') << 40))

namespace zattera { namespace protocol {

class legacy_zattera_asset_symbol_type
{
   public:
      legacy_zattera_asset_symbol_type() {}

      bool is_canon()const
      {   return ( ser == ZTR_SYMBOL_SER );    }

      uint64_t ser = ZTR_SYMBOL_SER;
};

struct legacy_zattera_asset
{
   public:
      legacy_zattera_asset() {}

      template< bool force_canon >
      asset to_asset()const
      {
         if( force_canon )
         {
            FC_ASSERT( symbol.is_canon(), "Must use canonical ZTR symbol serialization" );
         }
         return asset( amount, ZTR_SYMBOL );
      }

      static legacy_zattera_asset from_amount( share_type amount )
      {
         legacy_zattera_asset leg;
         leg.amount = amount;
         return leg;
      }

      static legacy_zattera_asset from_asset( const asset& a )
      {
         FC_ASSERT( a.symbol == ZTR_SYMBOL );
         return from_amount( a.amount );
      }

      share_type                       amount;
      legacy_zattera_asset_symbol_type symbol;
};

} }

namespace fc { namespace raw {

template< typename Stream >
inline void pack( Stream& s, const zattera::protocol::legacy_zattera_asset_symbol_type& sym )
{
   switch( sym.ser )
   {
      case ZTR_SYMBOL_LEGACY_SER_1:
      case ZTR_SYMBOL_LEGACY_SER_2:
      case ZTR_SYMBOL_LEGACY_SER_3:
      case ZTR_SYMBOL_LEGACY_SER_4:
      case ZTR_SYMBOL_LEGACY_SER_5:
         wlog( "pack legacy serialization ${s}", ("s", sym.ser) );
      case ZTR_SYMBOL_SER:
         pack( s, sym.ser );
         break;
      default:
         FC_ASSERT( false, "Cannot serialize legacy symbol ${s}", ("s", sym.ser) );
   }
}

template< typename Stream >
inline void unpack( Stream& s, zattera::protocol::legacy_zattera_asset_symbol_type& sym )
{
   //  994240:        "account_creation_fee": "0.1 ZTR"
   // 1021529:        "account_creation_fee": "10.0 ZTR"
   // 3143833:        "account_creation_fee": "3.00000 ZTR"
   // 3208405:        "account_creation_fee": "2.00000 ZTR"
   // 3695672:        "account_creation_fee": "3.00 ZTR"
   // 4338089:        "account_creation_fee": "0.001 0.001"
   // 4626205:        "account_creation_fee": "6.000 6.000"
   // 4632595:        "account_creation_fee": "6.000 6.000"

   uint64_t ser = 0;

   fc::raw::unpack( s, ser );
   switch( ser )
   {
      case ZTR_SYMBOL_LEGACY_SER_1:
      case ZTR_SYMBOL_LEGACY_SER_2:
      case ZTR_SYMBOL_LEGACY_SER_3:
      case ZTR_SYMBOL_LEGACY_SER_4:
      case ZTR_SYMBOL_LEGACY_SER_5:
         wlog( "unpack legacy serialization ${s}", ("s", ser) );
      case ZTR_SYMBOL_SER:
         sym.ser = ser;
         break;
      default:
         FC_ASSERT( false, "Cannot deserialize legacy symbol ${s}", ("s", ser) );
   }
}

} // fc::raw

inline void to_variant( const zattera::protocol::legacy_zattera_asset& leg, fc::variant& v )
{
   to_variant( leg.to_asset<false>(), v );
}

inline void from_variant( const fc::variant& v, zattera::protocol::legacy_zattera_asset& leg )
{
   zattera::protocol::asset a;
   from_variant( v, a );
   leg = zattera::protocol::legacy_zattera_asset::from_asset( a );
}

} // fc

FC_REFLECT( zattera::protocol::legacy_zattera_asset,
   (amount)
   (symbol)
   )

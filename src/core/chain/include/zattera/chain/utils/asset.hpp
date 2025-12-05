#pragma once

#include <zattera/protocol/asset.hpp>

namespace zattera { namespace chain { namespace util {

using zattera::protocol::asset;
using zattera::protocol::price;

inline asset to_zbd( const price& p, const asset& ztr )
{
   FC_ASSERT( ztr.symbol == ZTR_SYMBOL );
   if( p.is_null() )
      return asset( 0, ZBD_SYMBOL );
   return ztr * p;
}

inline asset to_ztr( const price& p, const asset& zbd )
{
   FC_ASSERT( zbd.symbol == ZBD_SYMBOL );
   if( p.is_null() )
      return asset( 0, ZTR_SYMBOL );
   return zbd * p;
}

} } }

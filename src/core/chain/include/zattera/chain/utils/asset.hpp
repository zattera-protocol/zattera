#pragma once

#include <zattera/protocol/asset.hpp>

namespace zattera { namespace chain { namespace util {

using zattera::protocol::asset;
using zattera::protocol::price;

inline asset to_dollar( const price& p, const asset& liquid )
{
   FC_ASSERT( liquid.symbol == LIQUID_SYMBOL );
   if( p.is_null() )
      return asset( 0, DOLLAR_SYMBOL );
   return liquid * p;
}

inline asset to_liquid( const price& p, const asset& dollar )
{
   FC_ASSERT( dollar.symbol == DOLLAR_SYMBOL );
   if( p.is_null() )
      return asset( 0, LIQUID_SYMBOL );
   return dollar * p;
}

} } }

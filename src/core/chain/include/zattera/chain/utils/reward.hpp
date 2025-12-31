#pragma once

#include <zattera/chain/utils/asset.hpp>
#include <zattera/chain/zattera_objects.hpp>

#include <zattera/protocol/asset.hpp>
#include <zattera/protocol/config.hpp>
#include <zattera/protocol/types.hpp>
#include <zattera/protocol/misc_utilities.hpp>

#include <fc/reflect/reflect.hpp>

#include <fc/uint128.hpp>

namespace zattera { namespace chain { namespace util {

using zattera::protocol::asset;
using zattera::protocol::price;
using zattera::protocol::share_type;

using fc::uint128_t;

struct comment_reward_context
{
   share_type rshares;
   uint16_t   reward_weight = 0;
   asset      max_dollars;
   uint128_t  total_reward_shares2;
   asset      total_reward_fund_liquid;
   price      current_liquid_price;
   protocol::curve_id   reward_curve = protocol::quadratic;
   uint128_t  content_constant = ZATTERA_CONTENT_CONSTANT;
};

uint64_t get_rshare_reward( const comment_reward_context& ctx );

inline uint128_t get_content_constant_s()
{
   return ZATTERA_CONTENT_CONSTANT; // looking good for posters
}

uint128_t evaluate_reward_curve( const uint128_t& rshares, const protocol::curve_id& curve = protocol::quadratic, const uint128_t& content_constant = ZATTERA_CONTENT_CONSTANT );

inline bool is_comment_payout_dust( const price& p, uint64_t liquid_payout )
{
   return to_dollar( p, asset( liquid_payout, LIQUID_SYMBOL ) ) < ZATTERA_MIN_PAYOUT_DOLLAR;
}

} } } // zattera::chain::util

FC_REFLECT( zattera::chain::util::comment_reward_context,
   (rshares)
   (reward_weight)
   (max_dollars)
   (total_reward_shares2)
   (total_reward_fund_liquid)
   (current_liquid_price)
   (reward_curve)
   (content_constant)
   )

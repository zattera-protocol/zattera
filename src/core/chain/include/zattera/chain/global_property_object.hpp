#pragma once
#include <fc/uint128.hpp>

#include <zattera/chain/zattera_object_types.hpp>

#include <zattera/protocol/asset.hpp>

namespace zattera { namespace chain {

   using zattera::protocol::asset;
   using zattera::protocol::price;

   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public object< dynamic_global_property_object_type, dynamic_global_property_object>
   {
      public:
         template< typename Constructor, typename Allocator >
         dynamic_global_property_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         dynamic_global_property_object(){}

         id_type           id;

         uint32_t          head_block_number = 0;
         block_id_type     head_block_id;
         time_point_sec    time;
         account_name_type current_witness;


         asset       virtual_liquid_supply             = asset( 0, ZTR_SYMBOL );
         asset       current_liquid_supply             = asset( 0, ZTR_SYMBOL );
         asset       confidential_liquid_supply        = asset( 0, ZTR_SYMBOL ); ///< total asset held in confidential balances
         asset       current_dollar_supply         = asset( 0, ZBD_SYMBOL );
         asset       confidential_dollar_supply    = asset( 0, ZBD_SYMBOL ); ///< total asset held in confidential balances
         asset       total_vesting_fund_liquid   = asset( 0, ZTR_SYMBOL );
         asset       total_vesting_shares       = asset( 0, VESTS_SYMBOL );
         asset       total_reward_fund_liquid    = asset( 0, ZTR_SYMBOL );
         fc::uint128 total_reward_shares2; ///< the running total of REWARD^2
         asset       pending_rewarded_vesting_shares = asset( 0, VESTS_SYMBOL );
         asset       pending_rewarded_vesting_liquid = asset( 0, ZTR_SYMBOL );

         price       get_vesting_share_price() const
         {
            if ( total_vesting_fund_liquid.amount == 0 || total_vesting_shares.amount == 0 )
               return price ( asset( 1000, ZTR_SYMBOL ), asset( 1000000, VESTS_SYMBOL ) );

            return price( total_vesting_shares, total_vesting_fund_liquid );
         }

         price get_reward_vesting_share_price() const
         {
            return price( total_vesting_shares + pending_rewarded_vesting_shares,
               total_vesting_fund_liquid + pending_rewarded_vesting_liquid );
         }

         /**
          *  This property defines the interest rate that dollar deposits receive.
          */
         uint16_t dollar_interest_rate = 0;

         uint16_t dollar_print_rate = ZATTERA_100_PERCENT;

         /**
          *  Maximum block size is decided by the set of active witnesses which change every round.
          *  Each witness posts what they think the maximum size should be as part of their witness
          *  properties, the median size is chosen to be the maximum block size for the round.
          *
          *  @note the minimum value for maximum_block_size is defined by the protocol to prevent the
          *  network from getting stuck by witnesses attempting to set this too low.
          */
         uint32_t     maximum_block_size = 0;

         /**
          * The current absolute slot number.  Equal to the total
          * number of slots since genesis.  Also equal to the total
          * number of missed slots plus head_block_number.
          */
         uint64_t      current_aslot = 0;

         /**
          * used to compute witness participation.
          */
         fc::uint128_t recent_slots_filled;
         uint8_t       participation_count = 0; ///< Divide by 128 to compute participation percentage

         uint32_t last_irreversible_block_num = 0;

         /**
          * The number of votes regenerated per day.  Any user voting slower than this rate will be
          * "wasting" voting power through spillover; any user voting faster than this rate will have
          * their votes reduced.
          */
         uint32_t vote_power_reserve_rate = ZATTERA_INITIAL_VOTE_POWER_RATE;

         uint32_t delegation_return_period = ZATTERA_DELEGATION_RETURN_PERIOD;
   };

   typedef multi_index_container<
      dynamic_global_property_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< dynamic_global_property_object, dynamic_global_property_object::id_type, &dynamic_global_property_object::id > >
      >,
      allocator< dynamic_global_property_object >
   > dynamic_global_property_index;

} } // zattera::chain

FC_REFLECT( zattera::chain::dynamic_global_property_object,
             (id)
             (head_block_number)
             (head_block_id)
             (time)
             (current_witness)
             (virtual_liquid_supply)
             (current_liquid_supply)
             (confidential_liquid_supply)
             (current_dollar_supply)
             (confidential_dollar_supply)
             (total_vesting_fund_liquid)
             (total_vesting_shares)
             (total_reward_fund_liquid)
             (total_reward_shares2)
             (pending_rewarded_vesting_shares)
             (pending_rewarded_vesting_liquid)
             (dollar_interest_rate)
             (dollar_print_rate)
             (maximum_block_size)
             (current_aslot)
             (recent_slots_filled)
             (participation_count)
             (last_irreversible_block_num)
             (vote_power_reserve_rate)
             (delegation_return_period)
          )
CHAINBASE_SET_INDEX_TYPE( zattera::chain::dynamic_global_property_object, zattera::chain::dynamic_global_property_index )

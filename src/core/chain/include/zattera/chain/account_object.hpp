#pragma once
#include <fc/fixed_string.hpp>

#include <zattera/protocol/authority.hpp>
#include <zattera/protocol/zattera_operations.hpp>

#include <zattera/chain/zattera_object_types.hpp>
#include <zattera/chain/witness_objects.hpp>
#include <zattera/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace zattera { namespace chain {

   using zattera::protocol::authority;

   class account_object : public object< account_object_type, account_object >
   {
      account_object() = delete;

      public:
         template<typename Constructor, typename Allocator>
         account_object( Constructor&& c, allocator< Allocator > a )
            :json_metadata( a )
         {
            c(*this);
         };

         id_type           id;

         account_name_type name;
         public_key_type   memo_key;
         shared_string     json_metadata;
         account_name_type proxy;

         time_point_sec    last_account_update;

         time_point_sec    created;
         bool              mined = true;
         account_name_type recovery_account;
         account_name_type reset_account = ZATTERA_NULL_ACCOUNT;
         time_point_sec    last_account_recovery;
         uint32_t          comment_count = 0;
         uint32_t          lifetime_vote_count = 0;
         uint32_t          post_count = 0;

         bool              can_vote = true;
         uint16_t          voting_power = ZATTERA_100_PERCENT;   ///< current voting power of this account, it falls after every vote
         time_point_sec    last_vote_time; ///< used to increase the voting power of this account the longer it goes without voting.

         asset             liquid_balance = asset( 0, LIQUID_SYMBOL );  ///< total liquid shares held by this account
         asset             savings_liquid_balance = asset( 0, LIQUID_SYMBOL );  ///< total liquid shares held by this account

         /**
          *  Dollar Deposits pay interest based upon the interest rate set by witnesses. The purpose of these
          *  fields is to track the total (time * dollar_balance) that it is held. Then at the appointed time
          *  interest can be paid using the following equation:
          *
          *  interest = interest_rate * dollar_seconds / seconds_per_year
          *
          *  Every time the dollar_balance is updated the dollar_seconds is also updated. If at least
          *  ZATTERA_MIN_COMPOUNDING_INTERVAL_SECONDS has past since dollar_last_interest_payment then
          *  interest is added to dollar_balance.
          *
          *  @defgroup dollar_data Dollar Balance Data
          */
         ///@{
         asset             dollar_balance = asset( 0, DOLLAR_SYMBOL ); /// total dollars balance
         uint128_t         dollar_seconds; ///< total dollars * how long it has been hel
         time_point_sec    dollar_seconds_last_update; ///< the last time the dollar_seconds was updated
         time_point_sec    dollar_last_interest_payment; ///< used to pay interest at most once per month


         asset             savings_dollar_balance = asset( 0, DOLLAR_SYMBOL ); /// total dollars balance
         uint128_t         savings_dollar_seconds; ///< total dollars * how long it has been hel
         time_point_sec    savings_dollar_seconds_last_update; ///< the last time the dollar_seconds was updated
         time_point_sec    savings_dollar_last_interest_payment; ///< used to pay interest at most once per month

         uint8_t           savings_withdraw_requests = 0;
         ///@}

         asset             reward_dollar_balance = asset( 0, DOLLAR_SYMBOL );
         asset             reward_liquid_balance = asset( 0, LIQUID_SYMBOL );
         asset             reward_vesting_share_balance = asset( 0, VESTS_SYMBOL );
         asset             reward_vesting_liquid_balance = asset( 0, LIQUID_SYMBOL );

         share_type        curation_rewards = 0;
         share_type        posting_rewards = 0;

         asset             vesting_share_balance = asset( 0, VESTS_SYMBOL ); ///< total vesting shares held by this account, controls its voting power
         asset             delegated_vesting_share_balance = asset( 0, VESTS_SYMBOL );
         asset             received_vesting_share_balance = asset( 0, VESTS_SYMBOL );

         asset             vesting_withdraw_rate = asset( 0, VESTS_SYMBOL ); ///< at the time this is updated it can be at most vesting_shares/104
         time_point_sec    next_vesting_withdrawal = fc::time_point_sec::maximum(); ///< after every withdrawal this is incremented by 1 week
         share_type        withdrawn = 0; /// Track how many shares have been withdrawn
         share_type        to_withdraw = 0; /// Might be able to look this up with operation history.
         uint16_t          withdraw_routes = 0;

         fc::array<share_type, ZATTERA_MAX_PROXY_RECURSION_DEPTH> proxied_vsf_votes;// = std::vector<share_type>( ZATTERA_MAX_PROXY_RECURSION_DEPTH, 0 ); ///< the total VFS votes proxied to this account

         uint16_t          witnesses_voted_for = 0;

         time_point_sec    last_post;
         time_point_sec    last_root_post = fc::time_point_sec::min();
         uint32_t          post_bandwidth = 0;

         share_type        pending_claimed_accounts = 0;

         /// This function should be used only when the account votes for a witness directly
         share_type        witness_vote_weight()const {
            return std::accumulate( proxied_vsf_votes.begin(),
                                    proxied_vsf_votes.end(),
                                    vesting_share_balance.amount );
         }
         share_type        proxied_vsf_votes_total()const {
            return std::accumulate( proxied_vsf_votes.begin(),
                                    proxied_vsf_votes.end(),
                                    share_type() );
         }
   };

   class account_authority_object : public object< account_authority_object_type, account_authority_object >
   {
      account_authority_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         account_authority_object( Constructor&& c, allocator< Allocator > a )
            : owner( a ), active( a ), posting( a )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account;

         shared_authority  owner;   ///< used for backup control, can set owner or active
         shared_authority  active;  ///< used for all monetary operations, can set active or posting
         shared_authority  posting; ///< used for voting and posting

         time_point_sec    last_owner_update;
   };

   class vesting_delegation_object : public object< vesting_delegation_object_type, vesting_delegation_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         vesting_delegation_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         vesting_delegation_object() {}

         id_type           id;
         account_name_type delegator;
         account_name_type delegatee;
         asset             vesting_shares;
         time_point_sec    min_delegation_time;
   };

   class vesting_delegation_expiration_object : public object< vesting_delegation_expiration_object_type, vesting_delegation_expiration_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         vesting_delegation_expiration_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         vesting_delegation_expiration_object() {}

         id_type           id;
         account_name_type delegator;
         asset             vesting_shares;
         time_point_sec    expiration;
   };

   class owner_authority_history_object : public object< owner_authority_history_object_type, owner_authority_history_object >
   {
      owner_authority_history_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         owner_authority_history_object( Constructor&& c, allocator< Allocator > a )
            :previous_owner_authority( allocator< shared_authority >( a ) )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account;
         shared_authority  previous_owner_authority;
         time_point_sec    last_valid_time;
   };

   class account_recovery_request_object : public object< account_recovery_request_object_type, account_recovery_request_object >
   {
      account_recovery_request_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         account_recovery_request_object( Constructor&& c, allocator< Allocator > a )
            :new_owner_authority( allocator< shared_authority >( a ) )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account_to_recover;
         shared_authority  new_owner_authority;
         time_point_sec    expires;
   };

   class change_recovery_account_request_object : public object< change_recovery_account_request_object_type, change_recovery_account_request_object >
   {
      public:
         template< typename Constructor, typename Allocator >
         change_recovery_account_request_object( Constructor&& c, allocator< Allocator > a )
         {
            c( *this );
         }

         id_type           id;

         account_name_type account_to_recover;
         account_name_type recovery_account;
         time_point_sec    effective_on;
   };

   struct by_proxy;
   struct by_next_vesting_withdrawal;

   /**
    * @ingroup object_index
    */
   typedef multi_index_container<
      account_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< account_object, account_id_type, &account_object::id > >,
         ordered_unique< tag< by_name >,
            member< account_object, account_name_type, &account_object::name > >,
         ordered_unique< tag< by_proxy >,
            composite_key< account_object,
               member< account_object, account_name_type, &account_object::proxy >,
               member< account_object, account_name_type, &account_object::name >
            > /// composite key by proxy
         >,
         ordered_unique< tag< by_next_vesting_withdrawal >,
            composite_key< account_object,
               member< account_object, time_point_sec, &account_object::next_vesting_withdrawal >,
               member< account_object, account_name_type, &account_object::name >
            > /// composite key by_next_vesting_withdrawal
         >
      >,
      allocator< account_object >
   > account_index;

   struct by_account;

   typedef multi_index_container <
      owner_authority_history_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< owner_authority_history_object, owner_authority_history_id_type, &owner_authority_history_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< owner_authority_history_object,
               member< owner_authority_history_object, account_name_type, &owner_authority_history_object::account >,
               member< owner_authority_history_object, time_point_sec, &owner_authority_history_object::last_valid_time >,
               member< owner_authority_history_object, owner_authority_history_id_type, &owner_authority_history_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< time_point_sec >, std::less< owner_authority_history_id_type > >
         >
      >,
      allocator< owner_authority_history_object >
   > owner_authority_history_index;

   struct by_last_owner_update;

   typedef multi_index_container <
      account_authority_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< account_authority_object, account_authority_id_type, &account_authority_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< account_authority_object,
               member< account_authority_object, account_name_type, &account_authority_object::account >,
               member< account_authority_object, account_authority_id_type, &account_authority_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_authority_id_type > >
         >,
         ordered_unique< tag< by_last_owner_update >,
            composite_key< account_authority_object,
               member< account_authority_object, time_point_sec, &account_authority_object::last_owner_update >,
               member< account_authority_object, account_authority_id_type, &account_authority_object::id >
            >,
            composite_key_compare< std::greater< time_point_sec >, std::less< account_authority_id_type > >
         >
      >,
      allocator< account_authority_object >
   > account_authority_index;

   struct by_delegation;

   typedef multi_index_container <
      vesting_delegation_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< vesting_delegation_object, vesting_delegation_id_type, &vesting_delegation_object::id > >,
         ordered_unique< tag< by_delegation >,
            composite_key< vesting_delegation_object,
               member< vesting_delegation_object, account_name_type, &vesting_delegation_object::delegator >,
               member< vesting_delegation_object, account_name_type, &vesting_delegation_object::delegatee >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< account_name_type > >
         >
      >,
      allocator< vesting_delegation_object >
   > vesting_delegation_index;

   struct by_expiration;
   struct by_account_expiration;

   typedef multi_index_container <
      vesting_delegation_expiration_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< vesting_delegation_expiration_object, vesting_delegation_expiration_id_type, &vesting_delegation_expiration_object::id > >,
         ordered_unique< tag< by_expiration >,
            composite_key< vesting_delegation_expiration_object,
               member< vesting_delegation_expiration_object, time_point_sec, &vesting_delegation_expiration_object::expiration >,
               member< vesting_delegation_expiration_object, vesting_delegation_expiration_id_type, &vesting_delegation_expiration_object::id >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< vesting_delegation_expiration_id_type > >
         >,
         ordered_unique< tag< by_account_expiration >,
            composite_key< vesting_delegation_expiration_object,
               member< vesting_delegation_expiration_object, account_name_type, &vesting_delegation_expiration_object::delegator >,
               member< vesting_delegation_expiration_object, time_point_sec, &vesting_delegation_expiration_object::expiration >,
               member< vesting_delegation_expiration_object, vesting_delegation_expiration_id_type, &vesting_delegation_expiration_object::id >
            >,
            composite_key_compare< std::less< account_name_type >, std::less< time_point_sec >, std::less< vesting_delegation_expiration_id_type > >
         >
      >,
      allocator< vesting_delegation_expiration_object >
   > vesting_delegation_expiration_index;

   struct by_expiration;

   typedef multi_index_container <
      account_recovery_request_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< account_recovery_request_object, account_recovery_request_id_type, &account_recovery_request_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< account_recovery_request_object,
               member< account_recovery_request_object, account_name_type, &account_recovery_request_object::account_to_recover >
            >,
            composite_key_compare< std::less< account_name_type > >
         >,
         ordered_unique< tag< by_expiration >,
            composite_key< account_recovery_request_object,
               member< account_recovery_request_object, time_point_sec, &account_recovery_request_object::expires >,
               member< account_recovery_request_object, account_name_type, &account_recovery_request_object::account_to_recover >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< account_name_type > >
         >
      >,
      allocator< account_recovery_request_object >
   > account_recovery_request_index;

   struct by_effective_date;

   typedef multi_index_container <
      change_recovery_account_request_object,
      indexed_by <
         ordered_unique< tag< by_id >,
            member< change_recovery_account_request_object, change_recovery_account_request_id_type, &change_recovery_account_request_object::id > >,
         ordered_unique< tag< by_account >,
            composite_key< change_recovery_account_request_object,
               member< change_recovery_account_request_object, account_name_type, &change_recovery_account_request_object::account_to_recover >
            >,
            composite_key_compare< std::less< account_name_type > >
         >,
         ordered_unique< tag< by_effective_date >,
            composite_key< change_recovery_account_request_object,
               member< change_recovery_account_request_object, time_point_sec, &change_recovery_account_request_object::effective_on >,
               member< change_recovery_account_request_object, account_name_type, &change_recovery_account_request_object::account_to_recover >
            >,
            composite_key_compare< std::less< time_point_sec >, std::less< account_name_type > >
         >
      >,
      allocator< change_recovery_account_request_object >
   > change_recovery_account_request_index;
} }

FC_REFLECT( zattera::chain::account_object,
             (id)(name)(memo_key)(json_metadata)(proxy)(last_account_update)
             (created)(mined)
             (recovery_account)(last_account_recovery)(reset_account)
             (comment_count)(lifetime_vote_count)(post_count)(can_vote)(voting_power)(last_vote_time)
             (liquid_balance)
             (savings_liquid_balance)
             (dollar_balance)(dollar_seconds)(dollar_seconds_last_update)(dollar_last_interest_payment)
             (savings_dollar_balance)(savings_dollar_seconds)(savings_dollar_seconds_last_update)(savings_dollar_last_interest_payment)(savings_withdraw_requests)
             (reward_liquid_balance)(reward_dollar_balance)(reward_vesting_share_balance)(reward_vesting_liquid_balance)
             (vesting_share_balance)(delegated_vesting_share_balance)(received_vesting_share_balance)
             (vesting_withdraw_rate)(next_vesting_withdrawal)(withdrawn)(to_withdraw)(withdraw_routes)
             (curation_rewards)
             (posting_rewards)
             (proxied_vsf_votes)(witnesses_voted_for)
             (last_post)(last_root_post)(post_bandwidth)
             (pending_claimed_accounts)
          )

CHAINBASE_SET_INDEX_TYPE( zattera::chain::account_object, zattera::chain::account_index )

FC_REFLECT( zattera::chain::account_authority_object,
             (id)(account)(owner)(active)(posting)(last_owner_update)
)
CHAINBASE_SET_INDEX_TYPE( zattera::chain::account_authority_object, zattera::chain::account_authority_index )

FC_REFLECT( zattera::chain::vesting_delegation_object,
            (id)(delegator)(delegatee)(vesting_shares)(min_delegation_time) )
CHAINBASE_SET_INDEX_TYPE( zattera::chain::vesting_delegation_object, zattera::chain::vesting_delegation_index )

FC_REFLECT( zattera::chain::vesting_delegation_expiration_object,
            (id)(delegator)(vesting_shares)(expiration) )
CHAINBASE_SET_INDEX_TYPE( zattera::chain::vesting_delegation_expiration_object, zattera::chain::vesting_delegation_expiration_index )

FC_REFLECT( zattera::chain::owner_authority_history_object,
             (id)(account)(previous_owner_authority)(last_valid_time)
          )
CHAINBASE_SET_INDEX_TYPE( zattera::chain::owner_authority_history_object, zattera::chain::owner_authority_history_index )

FC_REFLECT( zattera::chain::account_recovery_request_object,
             (id)(account_to_recover)(new_owner_authority)(expires)
          )
CHAINBASE_SET_INDEX_TYPE( zattera::chain::account_recovery_request_object, zattera::chain::account_recovery_request_index )

FC_REFLECT( zattera::chain::change_recovery_account_request_object,
             (id)(account_to_recover)(recovery_account)(effective_on)
          )
CHAINBASE_SET_INDEX_TYPE( zattera::chain::change_recovery_account_request_object, zattera::chain::change_recovery_account_request_index )

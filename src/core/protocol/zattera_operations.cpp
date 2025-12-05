#include <zattera/protocol/zattera_operations.hpp>

#include <fc/macros.hpp>
#include <fc/io/json.hpp>
#include <fc/macros.hpp>

#include <locale>

namespace zattera { namespace protocol {

   void validate_auth_size( const authority& a )
   {
      size_t size = a.account_auths.size() + a.key_auths.size();
      FC_ASSERT( size <= ZATTERA_MAX_AUTHORITY_MEMBERSHIP, "Authority membership exceeded. Max: 10 Current: ${n}", ("n", size) );
   }

   void account_create_operation::validate() const
   {
      validate_account_name( new_account_name );
      FC_ASSERT( is_asset_type( fee, ZTR_SYMBOL ), "Account creation fee must be ZTR" );
      owner.validate();
      active.validate();

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
      FC_ASSERT( fee >= asset( 0, ZTR_SYMBOL ), "Account creation fee cannot be negative" );
   }

   void account_create_with_delegation_operation::validate() const
   {
      validate_account_name( new_account_name );
      validate_account_name( creator );
      FC_ASSERT( is_asset_type( fee, ZTR_SYMBOL ), "Account creation fee must be ZTR" );
      FC_ASSERT( is_asset_type( delegation, VESTS_SYMBOL ), "Delegation must be VESTS" );

      owner.validate();
      active.validate();
      posting.validate();

      if( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }

      FC_ASSERT( fee >= asset( 0, ZTR_SYMBOL ), "Account creation fee cannot be negative" );
      FC_ASSERT( delegation >= asset( 0, VESTS_SYMBOL ), "Delegation cannot be negative" );
   }

   void account_update_operation::validate() const
   {
      validate_account_name( account );
      /*if( owner )
         owner->validate();
      if( active )
         active->validate();
      if( posting )
         posting->validate();*/

      if ( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   void comment_operation::validate() const
   {
      FC_ASSERT( title.size() < 256, "Title larger than size limit" );
      FC_ASSERT( fc::is_utf8( title ), "Title not formatted in UTF8" );
      FC_ASSERT( body.size() > 0, "Body is empty" );
      FC_ASSERT( fc::is_utf8( body ), "Body not formatted in UTF8" );

      if( parent_author.size() )
         validate_account_name( parent_author );
      validate_account_name( author );
      validate_permlink( parent_permlink );
      validate_permlink( permlink );

      if( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }
   }

   struct comment_options_extension_validate_visitor
   {
      typedef void result_type;

      void operator()( const comment_payout_beneficiaries& cpb ) const
      {
         cpb.validate();
      }
   };

   void comment_payout_beneficiaries::validate()const
   {
      uint32_t sum = 0;

      FC_ASSERT( beneficiaries.size(), "Must specify at least one beneficiary" );
      FC_ASSERT( beneficiaries.size() < 128, "Cannot specify more than 127 beneficiaries." ); // Require size serializtion fits in one byte.

      validate_account_name( beneficiaries[0].account );
      FC_ASSERT( beneficiaries[0].weight <= ZATTERA_100_PERCENT, "Cannot allocate more than 100% of rewards to one account" );
      sum += beneficiaries[0].weight;
      FC_ASSERT( sum <= ZATTERA_100_PERCENT, "Cannot allocate more than 100% of rewards to a comment" ); // Have to check incrementally to avoid overflow

      for( size_t i = 1; i < beneficiaries.size(); i++ )
      {
         validate_account_name( beneficiaries[i].account );
         FC_ASSERT( beneficiaries[i].weight <= ZATTERA_100_PERCENT, "Cannot allocate more than 100% of rewards to one account" );
         sum += beneficiaries[i].weight;
         FC_ASSERT( sum <= ZATTERA_100_PERCENT, "Cannot allocate more than 100% of rewards to a comment" ); // Have to check incrementally to avoid overflow
         FC_ASSERT( beneficiaries[i - 1] < beneficiaries[i], "Benficiaries must be specified in sorted order (account ascending)" );
      }
   }

   void comment_options_operation::validate()const
   {
      validate_account_name( author );
      FC_ASSERT( percent_zattera_dollars <= ZATTERA_100_PERCENT, "Percent cannot exceed 100%" );
      FC_ASSERT( max_accepted_payout.symbol == ZBD_SYMBOL, "Max accepted payout must be in ZBD" );
      FC_ASSERT( max_accepted_payout.amount.value >= 0, "Cannot accept less than 0 payout" );
      validate_permlink( permlink );
      for( auto& e : extensions )
         e.visit( comment_options_extension_validate_visitor() );
   }

   void delete_comment_operation::validate()const
   {
      validate_permlink( permlink );
      validate_account_name( author );
   }

   void claim_account_operation::validate()const
   {
      validate_account_name( creator );
      FC_ASSERT( is_asset_type( fee, ZTR_SYMBOL ), "Account creation fee must be ZTR" );
      FC_ASSERT( fee >= asset( 0, ZTR_SYMBOL ), "Account creation fee cannot be negative" );
      FC_ASSERT( extensions.size() == 0, "There are no extensions for claim_account_operation." );
   }

   void create_claimed_account_operation::validate()const
   {
      validate_account_name( creator );
      validate_account_name( new_account_name );
      owner.validate();
      active.validate();
      posting.validate();
      validate_auth_size( owner );
      validate_auth_size( active );
      validate_auth_size( posting );

      if( json_metadata.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_metadata), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_metadata), "JSON Metadata not valid JSON" );
      }

      FC_ASSERT( extensions.size() == 0, "There are no extensions for create_claimed_account_operation." );
   }

   void vote_operation::validate() const
   {
      validate_account_name( voter );
      validate_account_name( author );\
      FC_ASSERT( abs(weight) <= ZATTERA_100_PERCENT, "Weight is not a ZATTERA percentage" );
      validate_permlink( permlink );
   }

   void transfer_operation::validate() const
   { try {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.symbol != VESTS_SYMBOL, "transferring of Zattera Power (VESTS) is not allowed." );
      FC_ASSERT( amount.amount > 0, "Cannot transfer a negative amount (aka: stealing)" );
      FC_ASSERT( memo.size() < ZATTERA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   } FC_CAPTURE_AND_RETHROW( (*this) ) }

   void transfer_to_vesting_operation::validate() const
   {
      validate_account_name( from );
      FC_ASSERT( amount.symbol == ZTR_SYMBOL, "Amount must be ZTR" );
      if ( to != account_name_type() ) validate_account_name( to );
      FC_ASSERT( amount.amount > 0, "Must transfer a nonzero amount" );
   }

   void withdraw_vesting_operation::validate() const
   {
      validate_account_name( account );
      FC_ASSERT( is_asset_type( vesting_shares, VESTS_SYMBOL), "Amount must be VESTS"  );
   }

   void set_withdraw_vesting_route_operation::validate() const
   {
      validate_account_name( from_account );
      validate_account_name( to_account );
      FC_ASSERT( 0 <= percent && percent <= ZATTERA_100_PERCENT, "Percent must be valid zattera percent" );
   }

   void witness_update_operation::validate() const
   {
      validate_account_name( owner );

      FC_ASSERT( url.size() <= ZATTERA_MAX_WITNESS_URL_LENGTH, "URL is too long" );

      FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
      FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );
      FC_ASSERT( fee >= asset( 0, ZTR_SYMBOL ), "Fee cannot be negative" );
      props.validate< false >();
   }

   void witness_set_properties_operation::validate() const
   {
      validate_account_name( owner );

      // current signing key must be present
      FC_ASSERT( props.find( "key" ) != props.end(), "No signing key provided" );

      auto itr = props.find( "account_creation_fee" );
      if( itr != props.end() )
      {
         asset account_creation_fee;
         fc::raw::unpack_from_vector( itr->second, account_creation_fee );
         FC_ASSERT( account_creation_fee.symbol == ZTR_SYMBOL, "account_creation_fee must be in ZTR" );
         FC_ASSERT( account_creation_fee.amount >= ZATTERA_MIN_ACCOUNT_CREATION_FEE , "account_creation_fee smaller than minimum account creation fee" );
      }

      itr = props.find( "maximum_block_size" );
      if( itr != props.end() )
      {
         uint32_t maximum_block_size;
         fc::raw::unpack_from_vector( itr->second, maximum_block_size );
         FC_ASSERT( maximum_block_size >= ZATTERA_MIN_BLOCK_SIZE_LIMIT, "maximum_block_size smaller than minimum max block size" );
      }

      itr = props.find( "zbd_interest_rate" );
      if( itr != props.end() )
      {
         uint16_t zbd_interest_rate;
         fc::raw::unpack_from_vector( itr->second, zbd_interest_rate );
         FC_ASSERT( zbd_interest_rate >= 0, "zbd_interest_rate must be positive" );
         FC_ASSERT( zbd_interest_rate <= ZATTERA_100_PERCENT, "zbd_interest_rate must not exceed 100%" );
      }

      itr = props.find( "new_signing_key" );
      if( itr != props.end() )
      {
         public_key_type signing_key;
         fc::raw::unpack_from_vector( itr->second, signing_key );
         FC_UNUSED( signing_key ); // This tests the deserialization of the key
      }

      itr = props.find( "zbd_exchange_rate" );
      if( itr != props.end() )
      {
         price zbd_exchange_rate;
         fc::raw::unpack_from_vector( itr->second, zbd_exchange_rate );
         FC_ASSERT( ( is_asset_type( zbd_exchange_rate.base, ZBD_SYMBOL ) && is_asset_type( zbd_exchange_rate.quote, ZTR_SYMBOL ) ),
            "Price feed must be a ZTR/ZBD price" );
         zbd_exchange_rate.validate();
      }

      itr = props.find( "url" );
      if( itr != props.end() )
      {
         std::string url;
         fc::raw::unpack_from_vector< std::string >( itr->second, url );

         FC_ASSERT( url.size() <= ZATTERA_MAX_WITNESS_URL_LENGTH, "URL is too long" );
         FC_ASSERT( url.size() > 0, "URL size must be greater than 0" );
         FC_ASSERT( fc::is_utf8( url ), "URL is not valid UTF8" );
      }

      itr = props.find( "account_subsidy_limit" );
      if( itr != props.end() )
      {
         uint32_t account_subsidy_limit;
         fc::raw::unpack_from_vector( itr->second, account_subsidy_limit ); // Checks that the value can be deserialized
         FC_UNUSED( account_subsidy_limit );
      }
   }

   void account_witness_vote_operation::validate() const
   {
      validate_account_name( account );
      validate_account_name( witness );
   }

   void account_witness_proxy_operation::validate() const
   {
      validate_account_name( account );
      if( proxy.size() )
         validate_account_name( proxy );
      FC_ASSERT( proxy != account, "Cannot proxy to self" );
   }

   void custom_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( required_auths.size() > 0, "at least one account must be specified" );
   }
   void custom_json_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( (required_auths.size() + required_posting_auths.size()) > 0, "at least one account must be specified" );
      FC_ASSERT( id.size() <= 32, "id is too long" );
      FC_ASSERT( fc::is_utf8(json), "JSON Metadata not formatted in UTF8" );
      FC_ASSERT( fc::json::is_valid(json), "JSON Metadata not valid JSON" );
   }
   void custom_binary_operation::validate() const {
      /// required auth accounts are the ones whose bandwidth is consumed
      FC_ASSERT( (required_owner_auths.size() + required_active_auths.size() + required_posting_auths.size()) > 0, "at least one account must be specified" );
      FC_ASSERT( id.size() <= 32, "id is too long" );
      for( const auto& a : required_auths ) a.validate();
   }

   void feed_publish_operation::validate()const
   {
      validate_account_name( publisher );
      FC_ASSERT( ( is_asset_type( exchange_rate.base, ZTR_SYMBOL ) && is_asset_type( exchange_rate.quote, ZBD_SYMBOL ) )
         || ( is_asset_type( exchange_rate.base, ZBD_SYMBOL ) && is_asset_type( exchange_rate.quote, ZTR_SYMBOL ) ),
         "Price feed must be a ZTR/ZBD price" );
      exchange_rate.validate();
   }

   void limit_order_create_operation::validate()const
   {
      validate_account_name( owner );

      FC_ASSERT(  ( is_asset_type( amount_to_sell, ZTR_SYMBOL ) && is_asset_type( min_to_receive, ZBD_SYMBOL ) )
               || ( is_asset_type( amount_to_sell, ZBD_SYMBOL ) && is_asset_type( min_to_receive, ZTR_SYMBOL ) ),
               "Limit order must be for the ZTR:ZBD market" );

      (amount_to_sell / min_to_receive).validate();
   }

   void limit_order_create2_operation::validate()const
   {
      validate_account_name( owner );

      FC_ASSERT( amount_to_sell.symbol == exchange_rate.base.symbol, "Sell asset must be the base of the price" );
      exchange_rate.validate();

      FC_ASSERT(  ( is_asset_type( amount_to_sell, ZTR_SYMBOL ) && is_asset_type( exchange_rate.quote, ZBD_SYMBOL ) )
               || ( is_asset_type( amount_to_sell, ZBD_SYMBOL ) && is_asset_type( exchange_rate.quote, ZTR_SYMBOL ) ),
               "Limit order must be for the ZTR:ZBD market" );

      FC_ASSERT( (amount_to_sell * exchange_rate).amount > 0, "Amount to sell cannot round to 0 when traded" );
   }

   void limit_order_cancel_operation::validate()const
   {
      validate_account_name( owner );
   }

   void convert_operation::validate()const
   {
      validate_account_name( owner );
      /// only allow conversion from ZBD to ZTR, allowing the opposite can enable traders to abuse
      /// market fluxuations through converting large quantities without moving the price.
      FC_ASSERT( is_asset_type( amount, ZBD_SYMBOL ), "Can only convert ZBD to ZTR" );
      FC_ASSERT( amount.amount > 0, "Must convert some ZBD" );
   }

   void report_over_production_operation::validate()const
   {
      validate_account_name( reporter );
      validate_account_name( first_block.witness );
      FC_ASSERT( first_block.witness   == second_block.witness );
      FC_ASSERT( first_block.timestamp == second_block.timestamp );
      FC_ASSERT( first_block.signee()  == second_block.signee() );
      FC_ASSERT( first_block.id() != second_block.id() );
   }

   void escrow_transfer_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      FC_ASSERT( fee.amount >= 0, "fee cannot be negative" );
      FC_ASSERT( zbd_amount.amount >= 0, "zbd amount cannot be negative" );
      FC_ASSERT( ztr_amount.amount >= 0, "ztr amount cannot be negative" );
      FC_ASSERT( zbd_amount.amount > 0 || ztr_amount.amount > 0, "escrow must transfer a non-zero amount" );
      FC_ASSERT( from != agent && to != agent, "agent must be a third party" );
      FC_ASSERT( (fee.symbol == ZTR_SYMBOL) || (fee.symbol == ZBD_SYMBOL), "fee must be ZTR or ZBD" );
      FC_ASSERT( zbd_amount.symbol == ZBD_SYMBOL, "zbd amount must contain ZBD" );
      FC_ASSERT( ztr_amount.symbol == ZTR_SYMBOL, "ztr amount must contain ZTR" );
      FC_ASSERT( ratification_deadline < escrow_expiration, "ratification deadline must be before escrow expiration" );
      if ( json_meta.size() > 0 )
      {
         FC_ASSERT( fc::is_utf8(json_meta), "JSON Metadata not formatted in UTF8" );
         FC_ASSERT( fc::json::is_valid(json_meta), "JSON Metadata not valid JSON" );
      }
   }

   void escrow_approve_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      FC_ASSERT( who == to || who == agent, "to or agent must approve escrow" );
   }

   void escrow_dispute_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      FC_ASSERT( who == from || who == to, "who must be from or to" );
   }

   void escrow_release_operation::validate()const
   {
      validate_account_name( from );
      validate_account_name( to );
      validate_account_name( agent );
      validate_account_name( who );
      validate_account_name( receiver );
      FC_ASSERT( who == from || who == to || who == agent, "who must be from or to or agent" );
      FC_ASSERT( receiver == from || receiver == to, "receiver must be from or to" );
      FC_ASSERT( zbd_amount.amount >= 0, "zbd amount cannot be negative" );
      FC_ASSERT( ztr_amount.amount >= 0, "ztr amount cannot be negative" );
      FC_ASSERT( zbd_amount.amount > 0 || ztr_amount.amount > 0, "escrow must release a non-zero amount" );
      FC_ASSERT( zbd_amount.symbol == ZBD_SYMBOL, "zbd amount must contain ZBD" );
      FC_ASSERT( ztr_amount.symbol == ZTR_SYMBOL, "ztr amount must contain ZTR" );
   }

   void request_account_recovery_operation::validate()const
   {
      validate_account_name( recovery_account );
      validate_account_name( account_to_recover );
      new_owner_authority.validate();
   }

   void recover_account_operation::validate()const
   {
      validate_account_name( account_to_recover );
      FC_ASSERT( !( new_owner_authority == recent_owner_authority ), "Cannot set new owner authority to the recent owner authority" );
      FC_ASSERT( !new_owner_authority.is_impossible(), "new owner authority cannot be impossible" );
      FC_ASSERT( !recent_owner_authority.is_impossible(), "recent owner authority cannot be impossible" );
      FC_ASSERT( new_owner_authority.weight_threshold, "new owner authority cannot be trivial" );
      new_owner_authority.validate();
      recent_owner_authority.validate();
   }

   void change_recovery_account_operation::validate()const
   {
      validate_account_name( account_to_recover );
      validate_account_name( new_recovery_account );
   }

   void transfer_to_savings_operation::validate()const {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.amount > 0 );
      FC_ASSERT( amount.symbol == ZTR_SYMBOL || amount.symbol == ZBD_SYMBOL );
      FC_ASSERT( memo.size() < ZATTERA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   }
   void transfer_from_savings_operation::validate()const {
      validate_account_name( from );
      validate_account_name( to );
      FC_ASSERT( amount.amount > 0 );
      FC_ASSERT( amount.symbol == ZTR_SYMBOL || amount.symbol == ZBD_SYMBOL );
      FC_ASSERT( memo.size() < ZATTERA_MAX_MEMO_SIZE, "Memo is too large" );
      FC_ASSERT( fc::is_utf8( memo ), "Memo is not UTF8" );
   }
   void cancel_transfer_from_savings_operation::validate()const {
      validate_account_name( from );
   }

   void decline_voting_rights_operation::validate()const
   {
      validate_account_name( account );
   }

   void reset_account_operation::validate()const
   {
      validate_account_name( reset_account );
      validate_account_name( account_to_reset );
      FC_ASSERT( !new_owner_authority.is_impossible(), "new owner authority cannot be impossible" );
      FC_ASSERT( new_owner_authority.weight_threshold, "new owner authority cannot be trivial" );
      new_owner_authority.validate();
   }

   void set_reset_account_operation::validate()const
   {
      validate_account_name( account );
      if( current_reset_account.size() )
         validate_account_name( current_reset_account );
      validate_account_name( reset_account );
      FC_ASSERT( current_reset_account != reset_account, "new reset account cannot be current reset account" );
   }

   void claim_reward_balance_operation::validate()const
   {
      validate_account_name( account );
      FC_ASSERT( is_asset_type( reward_ztr, ZTR_SYMBOL ), "Reward Zattera must be ZTR" );
      FC_ASSERT( is_asset_type( reward_zbd, ZBD_SYMBOL ), "Reward Zattera must be ZBD" );
      FC_ASSERT( is_asset_type( reward_vests, VESTS_SYMBOL ), "Reward Zattera must be VESTS" );
      FC_ASSERT( reward_ztr.amount >= 0, "Cannot claim a negative amount" );
      FC_ASSERT( reward_zbd.amount >= 0, "Cannot claim a negative amount" );
      FC_ASSERT( reward_vests.amount >= 0, "Cannot claim a negative amount" );
      FC_ASSERT( reward_ztr.amount > 0 || reward_zbd.amount > 0 || reward_vests.amount > 0, "Must claim something." );
   }

   void delegate_vesting_shares_operation::validate()const
   {
      validate_account_name( delegator );
      validate_account_name( delegatee );
      FC_ASSERT( delegator != delegatee, "You cannot delegate VESTS to yourself" );
      FC_ASSERT( is_asset_type( vesting_shares, VESTS_SYMBOL ), "Delegation must be VESTS" );
      FC_ASSERT( vesting_shares >= asset( 0, VESTS_SYMBOL ), "Delegation cannot be negative" );
   }

} } // zattera::protocol

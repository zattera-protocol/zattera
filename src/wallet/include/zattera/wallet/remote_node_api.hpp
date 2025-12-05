#pragma once

#include <zattera/plugins/database_api/database_api.hpp>
#include <zattera/plugins/block_api/block_api.hpp>
#include <zattera/plugins/account_history_api/account_history_api.hpp>
#include <zattera/plugins/account_by_key_api/account_by_key_api.hpp>
#include <zattera/plugins/network_broadcast_api/network_broadcast_api.hpp>
#include <zattera/plugins/tags_api/tags_api.hpp>
#include <zattera/plugins/follow_api/follow_api.hpp>
#include <zattera/plugins/reputation_api/reputation_api.hpp>
#include <zattera/plugins/market_history_api/market_history_api.hpp>
#include <zattera/plugins/witness_api/witness_api.hpp>

#include <fc/api.hpp>

namespace zattera { namespace wallet {

using std::vector;
using std::map;
using std::set;
using std::string;
using namespace chain;
using namespace plugins;

struct broadcast_transaction_synchronous_return
{
   transaction_id_type id;
   int32_t             block_num = 0;
   int32_t             trx_num = 0;
   bool                expired = false;
};

struct scheduled_hardfork
{
   hardfork_version     hf_version;
   fc::time_point_sec   live_time;
};

enum withdraw_route_type
{
   incoming,
   outgoing,
   all
};

/**
 * Remote node API wrapper that provides wallet-friendly interface
 * to modular APIs via explicit RPC calls
 */
class remote_node_api
{
public:
   remote_node_api( fc::api_connection& conn );

   // Database API - Blockchain State
   fc::variant_object get_config() const;
   database_api::api_dynamic_global_property_object get_dynamic_global_properties() const;
   chain_properties get_chain_properties() const;
   price get_current_median_history_price() const;
   database_api::api_feed_history_object get_feed_history() const;
   database_api::api_witness_schedule_object get_witness_schedule() const;
   hardfork_version get_hardfork_version() const;
   scheduled_hardfork get_next_scheduled_hardfork() const;
   vector<database_api::api_reward_fund_object> get_reward_funds() const;

   // Block API
   fc::optional<block_header> get_block_header( uint32_t block_num ) const;
   fc::optional<signed_block> get_block( uint32_t block_num ) const;

   // Account History API
   vector<account_history::api_operation_object> get_ops_in_block( uint32_t block_num, bool only_virtual = true ) const;
   signed_transaction get_transaction( transaction_id_type tx_id ) const;
   map<uint32_t, account_history::api_operation_object> get_account_history( account_name_type account, uint64_t start, uint32_t limit ) const;

   // Account By Key API
   vector<vector<account_name_type>> get_key_references( vector<public_key_type> keys ) const;

   // Database API - Accounts
   vector<database_api::api_account_object> get_accounts( vector<account_name_type> names ) const;
   vector<account_name_type> list_accounts( account_name_type lower_bound_name, uint32_t limit ) const;
   vector<database_api::api_owner_authority_history_object> get_owner_history( account_name_type account ) const;
   fc::optional<database_api::api_account_recovery_request_object> get_recovery_request( account_name_type account ) const;
   fc::optional<database_api::api_escrow_object> get_escrow( account_name_type from, uint32_t escrow_id ) const;
   vector<database_api::api_withdraw_vesting_route_object> get_withdraw_routes( account_name_type account, withdraw_route_type type ) const;
   fc::optional<witness::api_account_bandwidth_object> get_account_bandwidth( account_name_type account, witness::bandwidth_type type ) const;
   vector<database_api::api_savings_withdraw_object> get_savings_withdraw_from( account_name_type account ) const;
   vector<database_api::api_savings_withdraw_object> get_savings_withdraw_to( account_name_type account ) const;
   vector<database_api::api_vesting_delegation_object> get_vesting_delegations( account_name_type account, account_name_type start, uint32_t limit ) const;
   vector<database_api::api_vesting_delegation_expiration_object> get_expiring_vesting_delegations( account_name_type account, fc::time_point_sec start, uint32_t limit ) const;

   // Database API - Witnesses
   vector<account_name_type> get_active_witnesses() const;
   vector<database_api::api_convert_request_object> get_conversion_requests( account_name_type account ) const;
   fc::optional<database_api::api_witness_object> get_witness_by_account( account_name_type account ) const;
   vector<database_api::api_witness_object> get_witnesses_by_vote( account_name_type start, uint32_t limit ) const;
   vector<account_name_type> list_witness_accounts( string lower_bound_name, uint32_t limit ) const;

   // Database API - Market
   vector<database_api::api_limit_order_object> get_open_orders( account_name_type account ) const;

   // Database API - Authority/Validation
   string get_transaction_hex( signed_transaction trx ) const;
   set<public_key_type> get_required_signatures( signed_transaction trx, fc::flat_set<public_key_type> available_keys ) const;
   set<public_key_type> get_potential_signatures( signed_transaction trx ) const;
   bool verify_authority( signed_transaction trx ) const;
   bool verify_account_authority( string account, fc::flat_set<public_key_type> signers ) const;

   // Tags API
   vector<tags::api_tag_object> get_trending_tags( string start_tag, uint32_t limit ) const;
   vector<tags::tag_count_object> get_tags_used_by_author( account_name_type author ) const;
   vector<tags::vote_state> get_active_votes( account_name_type author, string permlink ) const;
   tags::discussion get_content( account_name_type author, string permlink ) const;
   vector<tags::discussion> get_content_replies( account_name_type author, string permlink ) const;
   vector<tags::discussion> get_discussions_by_payout( tags::discussion_query query ) const;
   vector<tags::discussion> get_post_discussions_by_payout( tags::discussion_query query ) const;
   vector<tags::discussion> get_comment_discussions_by_payout( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_trending( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_created( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_active( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_cashout( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_votes( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_children( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_hot( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_feed( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_blog( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_comments( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_promoted( tags::discussion_query query ) const;
   vector<tags::discussion> get_replies_by_last_update( tags::discussion_query query ) const;
   vector<tags::discussion> get_discussions_by_author_before_date( tags::discussion_query query ) const;

   // Follow API
   vector<follow::api_follow_object> get_followers( account_name_type account, account_name_type start, follow::follow_type type, uint32_t limit ) const;
   vector<follow::api_follow_object> get_following( account_name_type account, account_name_type start, follow::follow_type type, uint32_t limit ) const;
   follow::get_follow_count_return get_follow_count( account_name_type account ) const;
   vector<follow::feed_entry> get_feed_entries( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const;
   vector<follow::comment_feed_entry> get_feed( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const;
   vector<follow::blog_entry> get_blog_entries( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const;
   vector<follow::comment_blog_entry> get_blog( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const;
   vector<account_name_type> get_reblogged_by( account_name_type author, string permlink ) const;
   vector<follow::reblog_count> get_blog_authors( account_name_type account ) const;

   // Reputation API
   vector<reputation::account_reputation> get_account_reputations( account_name_type lower_bound_name, uint32_t limit ) const;

   // Market History API
   market_history::get_ticker_return get_ticker() const;
   market_history::get_volume_return get_volume() const;
   market_history::get_order_book_return get_order_book( uint32_t limit ) const;
   vector<market_history::market_trade> get_trade_history( fc::time_point_sec start, fc::time_point_sec end, uint32_t limit ) const;
   vector<market_history::market_trade> get_recent_trades( uint32_t limit ) const;
   vector<market_history::bucket_object> get_market_history( uint32_t bucket_seconds, fc::time_point_sec start, fc::time_point_sec end ) const;
   fc::flat_set<uint32_t> get_market_history_buckets() const;

   // Network Broadcast API
   void broadcast_transaction( signed_transaction trx ) const;
   broadcast_transaction_synchronous_return broadcast_transaction_synchronous( signed_transaction trx ) const;
   void broadcast_block( signed_block block ) const;

private:
   fc::api_connection& _connection;

   /**
    * @brief Helper function to simplify API calls
    * @param api_name Name of the API (e.g., "database_api")
    * @param method_name Name of the method (e.g., "get_config")
    * @param args Arguments to pass to the API method
    * @return Variant result from the API call
    */
   fc::variant send_call( const string& api_name, const string& method_name, const fc::variant& args ) const;
};

} } // zattera::wallet

FC_REFLECT( zattera::wallet::broadcast_transaction_synchronous_return, (id)(block_num)(trx_num)(expired) )
FC_REFLECT( zattera::wallet::scheduled_hardfork, (hf_version)(live_time) )
FC_REFLECT_ENUM( zattera::wallet::withdraw_route_type, (incoming)(outgoing)(all) )

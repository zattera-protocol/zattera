#include <zattera/wallet/remote_node_api.hpp>
#include <fc/rpc/api_connection.hpp>
#include <zattera/plugins/json_rpc/utility.hpp>

namespace zattera { namespace wallet {

remote_node_api::remote_node_api( fc::api_connection& conn )
   : _connection(conn)
{
}

// ============================================================================
// Database API - Blockchain State
// ============================================================================

/**
 * @brief Retrieves blockchain configuration information
 * @return Configuration object including chain ID, block interval, reward parameters, etc.
 */
fc::variant_object remote_node_api::get_config() const
{
   database_api::get_config_args args;
   return send_call( "database_api", "get_config", fc::variant(args) )
      .as<database_api::get_config_return>();
}

/**
 * @brief Retrieves dynamic global properties
 * @return Dynamic information including current block number, head block time, total supply, witness count, etc.
 */
database_api::api_dynamic_global_property_object remote_node_api::get_dynamic_global_properties() const
{
   database_api::get_dynamic_global_properties_args args;
   return send_call( "database_api", "get_dynamic_global_properties", fc::variant(args) )
      .as<database_api::get_dynamic_global_properties_return>();
}

/**
 * @brief Retrieves the median of chain properties voted by witnesses
 * @return Chain parameters including account creation fee, maximum block size, ZBD interest rate, etc.
 */
chain_properties remote_node_api::get_chain_properties() const
{
   database_api::get_witness_schedule_args args;
   auto schedule = send_call( "database_api", "get_witness_schedule", fc::variant(args) )
      .as<database_api::get_witness_schedule_return>();
   return schedule.median_props;
}

/**
 * @brief Retrieves the current median of ZTR/ZBD price feed
 * @return Median of price feeds provided by witnesses (ZTR/ZBD exchange rate)
 */
price remote_node_api::get_current_median_history_price() const
{
   database_api::get_feed_history_args args;
   return send_call( "database_api", "get_feed_history", fc::variant(args) )
      .as<database_api::get_feed_history_return>().current_median_history;
}

/**
 * @brief Retrieves the complete price feed history
 * @return Current and historical price feed data (median values, price history, etc.)
 */
database_api::api_feed_history_object remote_node_api::get_feed_history() const
{
   database_api::get_feed_history_args args;
   return send_call( "database_api", "get_feed_history", fc::variant(args) )
      .as<database_api::get_feed_history_return>();
}

/**
 * @brief Retrieves witness schedule information
 * @return Current active witness list, median of chain properties voted by witnesses, etc.
 */
database_api::api_witness_schedule_object remote_node_api::get_witness_schedule() const
{
   database_api::get_witness_schedule_args args;
   return send_call( "database_api", "get_witness_schedule", fc::variant(args) )
      .as<database_api::get_witness_schedule_return>();
}

/**
 * @brief Retrieves the currently active hardfork version
 * @return Hardfork version currently running on the blockchain
 */
hardfork_version remote_node_api::get_hardfork_version() const
{
   database_api::get_hardfork_properties_args args;
   auto props = send_call( "database_api", "get_hardfork_properties", fc::variant(args) )
      .as<database_api::get_hardfork_properties_return>();
   return props.current_hardfork_version;
}

/**
 * @brief Retrieves information about the next scheduled hardfork
 * @return Next hardfork version and scheduled activation time
 */
scheduled_hardfork remote_node_api::get_next_scheduled_hardfork() const
{
   database_api::get_hardfork_properties_args args;
   auto props = send_call( "database_api", "get_hardfork_properties", fc::variant(args) )
      .as<database_api::get_hardfork_properties_return>();

   scheduled_hardfork result;

   result.hf_version = props.next_hardfork;
   result.live_time  = props.next_hardfork_time;

   return result;
}

/**
 * @brief Retrieves information about all reward funds
 * @return Vector of all reward funds
 */
vector<database_api::api_reward_fund_object> remote_node_api::get_reward_funds() const
{
   database_api::get_reward_funds_args args;
   return send_call( "database_api", "get_reward_funds", fc::variant(args) )
      .as<database_api::get_reward_funds_return>().funds;
}

// ============================================================================
// Block API
// ============================================================================

/**
 * @brief Retrieves a block header by block number
 * @param block_num Block number to retrieve
 * @return Block header containing previous block hash, timestamp, witness, etc.
 */
fc::optional<block_header> remote_node_api::get_block_header( uint32_t block_num ) const
{
   block_api::get_block_header_args args{ .block_num = block_num };
   return send_call( "block_api", "get_block_header", fc::variant(args) )
      .as<block_api::get_block_header_return>().header;
}

/**
 * @brief Retrieves a complete block by block number
 * @param block_num Block number to retrieve
 * @return Signed block containing header, transactions, and witness signature
 */
fc::optional<signed_block> remote_node_api::get_block( uint32_t block_num ) const
{
   block_api::get_block_args args{ .block_num = block_num };
   return send_call( "block_api", "get_block", fc::variant(args) )
      .as<block_api::get_block_return>().block;
}

// ============================================================================
// Account History API
// ============================================================================

/**
 * @brief Retrieves all operations in a specific block
 * @param block_num Block number to retrieve operations from
 * @param only_virtual If true, only return virtual operations (operations not explicitly submitted by users)
 * @return Vector of operation objects including operation type, timestamp, and data
 */
vector<account_history::api_operation_object> remote_node_api::get_ops_in_block( uint32_t block_num, bool only_virtual ) const
{
   account_history::get_ops_in_block_args args{
      .block_num    = block_num,
      .only_virtual = only_virtual
   };
   auto result = send_call( "account_history_api", "get_ops_in_block", fc::variant(args) )
      .as<account_history::get_ops_in_block_return>();

   // Convert multiset to vector
   return vector<account_history::api_operation_object>( result.ops.begin(), result.ops.end() );
}

/**
 * @brief Retrieves a transaction by transaction ID
 * @param tx_id Transaction ID to retrieve
 * @return Signed transaction containing operations and signatures
 */
signed_transaction remote_node_api::get_transaction( transaction_id_type tx_id ) const
{
   account_history::get_transaction_args args{ .id = tx_id };
   return send_call( "account_history_api", "get_transaction", fc::variant(args) )
      .as<account_history::get_transaction_return>();
}

/**
 * @brief Retrieves account history (operations affecting the account)
 * @param account Account name to retrieve history for
 * @param start Starting sequence number (use -1 to start from most recent)
 * @param limit Maximum number of operations to return
 * @return Map of sequence numbers to operation objects
 */
map<uint32_t, account_history::api_operation_object> remote_node_api::get_account_history( account_name_type account, uint64_t start, uint32_t limit ) const
{
   account_history::get_account_history_args args{ .account = account, .start = start, .limit = limit };
   return send_call( "account_history_api", "get_account_history", fc::variant(args) )
      .as<account_history::get_account_history_return>().history;
}

// ============================================================================
// Account By Key API
// ============================================================================

/**
 * @brief Retrieves accounts that reference the given public keys
 * @param keys Vector of public keys to look up
 * @return Vector of account name vectors - each element corresponds to a key and contains all accounts using that key
 */
vector<vector<account_name_type>> remote_node_api::get_key_references( vector<public_key_type> keys ) const
{
   account_by_key::get_key_references_args args{ .keys = keys };
   return send_call( "account_by_key_api", "get_key_references", fc::variant(args) )
      .as<account_by_key::get_key_references_return>().accounts;
}

// ============================================================================
// Database API - Accounts
// ============================================================================

/**
 * @brief Retrieves account information for the specified accounts
 * @param names Vector of account names to retrieve
 * @return Vector of account objects containing balances, voting power, bandwidth, etc.
 */
vector<database_api::api_account_object> remote_node_api::get_accounts( vector<account_name_type> names ) const
{
   database_api::find_accounts_args args{ .accounts = names };
   return send_call( "database_api", "find_accounts", fc::variant(args) )
      .as<database_api::find_accounts_return>().accounts;
}

/**
 * @brief Lists accounts starting from a lower bound name
 * @param lower_bound_name Starting account name (inclusive)
 * @param limit Maximum number of accounts to return
 * @return Vector of account names in alphabetical order
 */
vector<account_name_type> remote_node_api::list_accounts( account_name_type lower_bound_name, uint32_t limit ) const
{
   database_api::list_accounts_args args{ 
      .start = fc::variant(lower_bound_name),
      .limit = limit,
      .order = database_api::by_name
   };
   auto result = send_call( "database_api", "list_accounts", fc::variant(args) )
      .as<database_api::list_accounts_return>();

   // Extract account names from account objects
   vector<account_name_type> names;
   names.reserve( result.accounts.size() );
   for( const auto& account : result.accounts )
      names.push_back( account.name );
   return names;
}

/**
 * @brief Retrieves owner authority change history for an account
 * @param account Account name to retrieve history for
 * @return Vector of owner authority history objects showing when owner keys were changed
 */
vector<database_api::api_owner_authority_history_object> remote_node_api::get_owner_history( account_name_type account ) const
{
   database_api::find_owner_histories_args args{ .owner = account };
   return send_call( "database_api", "find_owner_histories", fc::variant(args) )
      .as<database_api::find_owner_histories_return>().owner_auths;
}

/**
 * @brief Retrieves active account recovery request for an account
 * @param account Account name to check for recovery requests
 * @return Optional recovery request object, empty if no active request exists
 */
fc::optional<database_api::api_account_recovery_request_object> remote_node_api::get_recovery_request( account_name_type account ) const
{
   database_api::find_account_recovery_requests_args args{ .accounts = { account } };
   auto requests = send_call( "database_api", "find_account_recovery_requests", fc::variant(args) )
      .as<database_api::find_account_recovery_requests_return>().requests;

   if( !requests.empty() )
      return requests[0];

   return fc::optional<database_api::api_account_recovery_request_object>();
}

/**
 * @brief Retrieves an escrow agreement by sender and escrow ID
 * @param from Account that initiated the escrow
 * @param escrow_id Escrow identifier
 * @return Optional escrow object, empty if not found
 */
fc::optional<database_api::api_escrow_object> remote_node_api::get_escrow( account_name_type from, uint32_t escrow_id ) const
{
   database_api::find_escrows_args args{ .from = from };
   auto escrows = send_call( "database_api", "find_escrows", fc::variant(args) )
      .as<database_api::list_escrows_return>().escrows;

   for( const auto& escrow : escrows )
   {
      if( escrow.escrow_id == escrow_id )
         return escrow;
   }

   return fc::optional<database_api::api_escrow_object>();
}

/**
 * @brief Retrieves vesting withdrawal routes for an account
 * @param account Account to retrieve routes for
 * @param type Route type filter: outgoing, incoming, or all
 * @return Vector of withdraw vesting route objects showing automatic payment destinations
 */
vector<database_api::api_withdraw_vesting_route_object> remote_node_api::get_withdraw_routes( account_name_type account, withdraw_route_type type ) const
{
   database_api::list_withdraw_vesting_routes_args args{
      .start = fc::variant(account),
      .limit = 100,
      .order = database_api::by_withdraw_route
   };
   auto all_routes = send_call( "database_api", "list_withdraw_vesting_routes", fc::variant(args) )
      .as<database_api::list_withdraw_vesting_routes_return>().routes;

   vector<database_api::api_withdraw_vesting_route_object> result;

   for( const auto& route : all_routes )
   {
      if( route.from_account != account )
         break;

      if( type == outgoing && route.from_account == account )
         result.push_back( route );
      else if( type == incoming && route.to_account == account )
         result.push_back( route );
      else if( type == all )
         result.push_back( route );
   }

   return result;
}

/**
 * @brief Retrieves account bandwidth usage for rate limiting
 * @param account Account to check bandwidth for
 * @param type Bandwidth type (post, forum, market, etc.)
 * @return Optional bandwidth object showing current usage and limits
 */
fc::optional<witness::api_account_bandwidth_object> remote_node_api::get_account_bandwidth( account_name_type account, witness::bandwidth_type type ) const
{
   witness::get_account_bandwidth_args args{
      .account = account,
      .type    = type
   };
   return send_call( "witness_api", "get_account_bandwidth", fc::variant(args) )
      .as<witness::get_account_bandwidth_return>().bandwidth;
}

/**
 * @brief Retrieves pending savings withdrawals initiated by an account
 * @param account Account that initiated the withdrawals
 * @return Vector of savings withdrawal objects with withdrawal amounts and completion times
 */
vector<database_api::api_savings_withdraw_object> remote_node_api::get_savings_withdraw_from( account_name_type account ) const
{
   database_api::find_savings_withdrawals_args args{ .account = account };
   return send_call( "database_api", "find_savings_withdrawals", fc::variant(args) )
      .as<database_api::list_savings_withdrawals_return>().withdrawals;
}

/**
 * @brief Retrieves pending savings withdrawals destined to an account
 * @param account Account receiving the withdrawals
 * @return Vector of savings withdrawal objects where the account is the recipient
 * @note This lists all withdrawals and filters manually since the API only supports searching by sender
 */
vector<database_api::api_savings_withdraw_object> remote_node_api::get_savings_withdraw_to( account_name_type account ) const
{
   // find_savings_withdrawals only searches by from account
   // We need to use list and filter manually
   database_api::list_savings_withdrawals_args args{
      .start = fc::variant(),
      .limit = 1000,
      .order = database_api::by_from_id
   };
   auto all_withdrawals = send_call( "database_api", "list_savings_withdrawals", fc::variant(args) )
      .as<database_api::list_savings_withdrawals_return>().withdrawals;

   vector<database_api::api_savings_withdraw_object> result;
   for( const auto& withdrawal : all_withdrawals )
   {
      if( withdrawal.to == account )
         result.push_back( withdrawal );
   }

   return result;
}

/**
 * @brief Retrieves vesting delegations made by an account
 * @param account Account that delegated vesting shares
 * @param start Starting delegatee account name for pagination
 * @param limit Maximum number of delegations to return
 * @return Vector of vesting delegation objects showing delegated amounts and recipients
 */
vector<database_api::api_vesting_delegation_object> remote_node_api::get_vesting_delegations( account_name_type account, account_name_type start, uint32_t limit ) const
{
   database_api::find_vesting_delegations_args args{ .account = account };
   return send_call( "database_api", "find_vesting_delegations", fc::variant(args) )
      .as<database_api::list_vesting_delegations_return>().delegations;
}

/**
 * @brief Retrieves expiring vesting delegations for an account
 * @param account Account to check for expiring delegations
 * @param start Starting time for pagination
 * @param limit Maximum number of expirations to return
 * @return Vector of delegation expiration objects showing when delegated vesting will be returned
 */
vector<database_api::api_vesting_delegation_expiration_object> remote_node_api::get_expiring_vesting_delegations( account_name_type account, fc::time_point_sec start, uint32_t limit ) const
{
   database_api::find_vesting_delegation_expirations_args args{ .account = account };
   return send_call( "database_api", "find_vesting_delegation_expirations", fc::variant(args) )
      .as<database_api::list_vesting_delegation_expirations_return>().delegations;
}

// ============================================================================
// Database API - Witnesses
// ============================================================================

/**
 * @brief Retrieves the list of currently active witnesses
 * @return Vector of witness account names currently producing blocks
 */
vector<account_name_type> remote_node_api::get_active_witnesses() const
{
   auto schedule = get_witness_schedule();

   vector<account_name_type> result;

   result.reserve( schedule.current_shuffled_witnesses.size() );
   for( const auto& w : schedule.current_shuffled_witnesses )
      result.push_back( account_name_type(w) );

   return result;
}

/**
 * @brief Retrieves pending ZBD to ZTR conversion requests for an account
 * @param account Account to retrieve conversion requests for
 * @return Vector of conversion request objects with conversion amount and completion time
 */
vector<database_api::api_convert_request_object> remote_node_api::get_conversion_requests( account_name_type account ) const
{
   database_api::find_zbd_conversion_requests_args args{ .account = account };
   return send_call( "database_api", "find_zbd_conversion_requests", fc::variant(args) )
      .as<database_api::find_zbd_conversion_requests_return>().requests;
}

/**
 * @brief Retrieves witness information by account name
 * @param account Account name of the witness
 * @return Optional witness object containing vote count, signing key, price feed, etc.
 */
fc::optional<database_api::api_witness_object> remote_node_api::get_witness_by_account( account_name_type account ) const
{
   database_api::find_witnesses_args args{ .owners = { account } };
   auto witnesses = send_call( "database_api", "find_witnesses", fc::variant(args) )
      .as<database_api::find_witnesses_return>().witnesses;

   if( !witnesses.empty() )
      return witnesses[0];

   return fc::optional<database_api::api_witness_object>();
}

/**
 * @brief Retrieves witnesses sorted by vote count
 * @param start Starting witness account name for pagination
 * @param limit Maximum number of witnesses to return
 * @return Vector of witness objects sorted by descending vote count
 */
vector<database_api::api_witness_object> remote_node_api::get_witnesses_by_vote( account_name_type start, uint32_t limit ) const
{
   database_api::list_witnesses_args args{
      .start = fc::variant(start),
      .limit = limit,
      .order = database_api::by_vote_name
   };
   return send_call( "database_api", "list_witnesses", fc::variant(args) )
      .as<database_api::list_witnesses_return>().witnesses;
}

/**
 * @brief Looks up witness accounts starting from a lower bound name
 * @param lower_bound_name Starting witness account name (inclusive)
 * @param limit Maximum number of witness accounts to return
 * @return Vector of witness account names in alphabetical order
 */
vector<account_name_type> remote_node_api::list_witness_accounts( string lower_bound_name, uint32_t limit ) const
{
   database_api::list_witnesses_args args{
      .start = lower_bound_name,
      .limit = limit,
      .order = database_api::by_name
   };
   auto witnesses = send_call( "database_api", "list_witnesses", fc::variant(args) )
      .as<database_api::list_witnesses_return>().witnesses;

   vector<account_name_type> result;

   result.reserve( witnesses.size() );
   for( const auto& w : witnesses )
      result.push_back( w.owner );

   return result;
}

// ============================================================================
// Database API - Market
// ============================================================================

/**
 * @brief Retrieves open limit orders for an account
 * @param account Account to retrieve open orders for
 * @return Vector of limit order objects showing order price, amount, and expiration
 */
vector<database_api::api_limit_order_object> remote_node_api::get_open_orders( account_name_type account ) const
{
   database_api::find_limit_orders_args args{ .account = account };
   return send_call( "database_api", "find_limit_orders", fc::variant(args) )
      .as<database_api::list_limit_orders_return>().orders;
}

// ============================================================================
// Database API - Authority/Validation
// ============================================================================

/**
 * @brief Converts a transaction to hexadecimal string representation
 * @param trx Transaction to convert
 * @return Hex-encoded transaction string
 */
string remote_node_api::get_transaction_hex( signed_transaction trx ) const
{
   database_api::get_transaction_hex_args args{ .trx = trx };
   return send_call( "database_api", "get_transaction_hex", fc::variant(args) )
      .as<database_api::get_transaction_hex_return>().hex;
}

/**
 * @brief Determines which keys from a set are required to sign a transaction
 * @param trx Transaction to check
 * @param available_keys Set of available public keys
 * @return Subset of available keys that are required to authorize the transaction
 */
set<public_key_type> remote_node_api::get_required_signatures( signed_transaction trx, fc::flat_set<public_key_type> available_keys ) const
{
   database_api::get_required_signatures_args args{
      .trx            = trx,
      .available_keys = available_keys
   };
   return send_call( "database_api", "get_required_signatures", fc::variant(args) )
      .as<database_api::get_required_signatures_return>().keys;
}

/**
 * @brief Retrieves all keys that could potentially sign a transaction
 * @param trx Transaction to check
 * @return Set of public keys that could provide valid signatures for the transaction
 */
set<public_key_type> remote_node_api::get_potential_signatures( signed_transaction trx ) const
{
   database_api::get_potential_signatures_args args{ .trx = trx };
   return send_call( "database_api", "get_potential_signatures", fc::variant(args) )
      .as<database_api::get_potential_signatures_return>().keys;
}

/**
 * @brief Verifies that a transaction has sufficient authority to execute
 * @param trx Transaction to verify
 * @return True if the transaction has sufficient signatures, false otherwise
 */
bool remote_node_api::verify_authority( signed_transaction trx ) const
{
   database_api::verify_authority_args args{ .trx = trx };
   return send_call( "database_api", "verify_authority", fc::variant(args) )
      .as<database_api::verify_authority_return>().valid;
}

/**
 * @brief Verifies that given signers have authority over an account
 * @param account Account name to check authority for
 * @param signers Set of public keys to verify
 * @return True if the signers have authority over the account, false otherwise
 */
bool remote_node_api::verify_account_authority( string account, fc::flat_set<public_key_type> signers ) const
{
   database_api::verify_account_authority_args args{
      .account = account,
      .signers = signers
   };
   return send_call( "database_api", "verify_account_authority", fc::variant(args) )
      .as<database_api::verify_account_authority_return>().valid;
}

// ============================================================================
// Tags API
// ============================================================================

/**
 * @brief Retrieves trending tags ordered by activity
 * @param start_tag Starting tag for pagination
 * @param limit Maximum number of tags to return
 * @return Vector of tag objects with names and trending scores
 */
vector<tags::api_tag_object> remote_node_api::get_trending_tags( string start_tag, uint32_t limit ) const
{
   tags::get_trending_tags_args args{
      .start_tag = start_tag,
      .limit = limit
   };
   return send_call( "tags_api", "get_trending_tags", fc::variant(args) )
      .as<tags::get_trending_tags_return>().tags;
}

/**
 * @brief Retrieves tags used by a specific author
 * @param author Author account to retrieve tags for
 * @return Vector of tags and their usage counts by the author
 */
vector<tags::tag_count_object> remote_node_api::get_tags_used_by_author( account_name_type author ) const
{
   tags::get_tags_used_by_author_args args{ .author = author };
   return send_call( "tags_api", "get_tags_used_by_author", fc::variant(args) )
      .as<tags::get_tags_used_by_author_return>().tags;
}

/**
 * @brief Retrieves all votes on a specific post or comment
 * @param author Author of the post/comment
 * @param permlink Permlink of the post/comment
 * @return Vector of vote states including voter, weight, and time
 */
vector<tags::vote_state> remote_node_api::get_active_votes( account_name_type author, string permlink ) const
{
   tags::get_active_votes_args args{
      .author   = author,
      .permlink = permlink
   };
   return send_call( "tags_api", "get_active_votes", fc::variant(args) )
      .as<tags::get_active_votes_return>().votes;
}

/**
 * @brief Retrieves a post or comment with metadata
 * @param author Author of the content
 * @param permlink Permlink of the content
 * @return Discussion object containing the post/comment and its metadata
 */
tags::discussion remote_node_api::get_content( account_name_type author, string permlink ) const
{
   tags::get_discussion_args args{
      .author   = author,
      .permlink = permlink
   };
   return send_call( "tags_api", "get_discussion", fc::variant(args) )
      .as<tags::get_discussion_return>();
}

/**
 * @brief Retrieves all replies to a post or comment
 * @param author Author of the parent post/comment
 * @param permlink Permlink of the parent post/comment
 * @return Vector of discussion objects representing the replies
 */
vector<tags::discussion> remote_node_api::get_content_replies( account_name_type author, string permlink ) const
{
   tags::get_content_replies_args args{
      .author   = author,
      .permlink = permlink
   };
   return send_call( "tags_api", "get_content_replies", fc::variant(args) )
      .as<tags::get_content_replies_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by pending payout
 * @param query Discussion query parameters (tag, limit, start_author, start_permlink)
 * @return Vector of discussions ordered by pending payout amount
 */
vector<tags::discussion> remote_node_api::get_discussions_by_payout( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_payout", fc::variant(query) )
      .as<vector<tags::discussion>>();
}

/**
 * @brief Retrieves posts (not comments) sorted by pending payout
 * @param query Discussion query parameters
 * @return Vector of post discussions ordered by pending payout amount
 */
vector<tags::discussion> remote_node_api::get_post_discussions_by_payout( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_post_discussions_by_payout", fc::variant(query) )
      .as<tags::get_post_discussions_by_payout_return>().discussions;
}

/**
 * @brief Retrieves comments (not posts) sorted by pending payout
 * @param query Discussion query parameters
 * @return Vector of comment discussions ordered by pending payout amount
 */
vector<tags::discussion> remote_node_api::get_comment_discussions_by_payout( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_comment_discussions_by_payout", fc::variant(query) )
      .as<tags::get_comment_discussions_by_payout_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by trending score
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by trending algorithm
 */
vector<tags::discussion> remote_node_api::get_discussions_by_trending( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_trending", fc::variant(query) )
      .as<tags::get_discussions_by_trending_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by creation time
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by newest first
 */
vector<tags::discussion> remote_node_api::get_discussions_by_created( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_created", fc::variant(query) )
      .as<tags::get_discussions_by_created_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by last activity time
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by most recently active
 */
vector<tags::discussion> remote_node_api::get_discussions_by_active( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_active", fc::variant(query) )
      .as<tags::get_discussions_by_active_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by cashout time
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by earliest cashout time
 */
vector<tags::discussion> remote_node_api::get_discussions_by_cashout( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_cashout", fc::variant(query) )
      .as<tags::get_discussions_by_cashout_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by net votes
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by highest net votes
 */
vector<tags::discussion> remote_node_api::get_discussions_by_votes( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_votes", fc::variant(query) )
      .as<tags::get_discussions_by_votes_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by number of children (replies)
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by most replies
 */
vector<tags::discussion> remote_node_api::get_discussions_by_children( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_children", fc::variant(query) )
      .as<tags::get_discussions_by_children_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by "hot" algorithm
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by hot score (combines votes and time)
 */
vector<tags::discussion> remote_node_api::get_discussions_by_hot( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_hot", fc::variant(query) )
      .as<tags::get_discussions_by_hot_return>().discussions;
}

/**
 * @brief Retrieves discussions from an account's feed
 * @param query Discussion query parameters (must specify account in tag field)
 * @return Vector of discussions from accounts the user follows
 */
vector<tags::discussion> remote_node_api::get_discussions_by_feed( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_feed", fc::variant(query) )
      .as<tags::get_discussions_by_feed_return>().discussions;
}

/**
 * @brief Retrieves discussions from an account's blog
 * @param query Discussion query parameters (must specify account in tag field)
 * @return Vector of discussions authored or reblogged by the account
 */
vector<tags::discussion> remote_node_api::get_discussions_by_blog( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_blog", fc::variant(query) )
      .as<tags::get_discussions_by_blog_return>().discussions;
}

/**
 * @brief Retrieves comments made by an account
 * @param query Discussion query parameters (must specify account in start_author field)
 * @return Vector of comment discussions authored by the account
 */
vector<tags::discussion> remote_node_api::get_discussions_by_comments( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_comments", fc::variant(query) )
      .as<tags::get_discussions_by_comments_return>().discussions;
}

/**
 * @brief Retrieves discussions sorted by promoted amount
 * @param query Discussion query parameters
 * @return Vector of discussions ordered by promotion (burned ZBD for visibility)
 */
vector<tags::discussion> remote_node_api::get_discussions_by_promoted( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_promoted", fc::variant(query) )
      .as<tags::get_discussions_by_promoted_return>().discussions;
}

/**
 * @brief Retrieves replies sorted by last update time
 * @param query Discussion query parameters
 * @return Vector of reply discussions ordered by most recently updated
 */
vector<tags::discussion> remote_node_api::get_replies_by_last_update( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_replies_by_last_update", fc::variant(query) )
      .as<tags::get_replies_by_last_update_return>().discussions;
}

/**
 * @brief Retrieves discussions by author before a specific date
 * @param query Discussion query parameters (must specify author and before_date)
 * @return Vector of discussions by the author created before the specified date
 */
vector<tags::discussion> remote_node_api::get_discussions_by_author_before_date( tags::discussion_query query ) const
{
   return send_call( "tags_api", "get_discussions_by_author_before_date", fc::variant(query) )
      .as<tags::get_discussions_by_author_before_date_return>().discussions;
}

// ============================================================================
// Follow API
// ============================================================================

/**
 * @brief Retrieves accounts following a specific account
 * @param account Account to retrieve followers for
 * @param start Starting follower account name for pagination
 * @param type Follow type filter (blog, ignore, etc.)
 * @param limit Maximum number of followers to return
 * @return Vector of follow objects showing who follows the account
 */
vector<follow::api_follow_object> remote_node_api::get_followers( account_name_type account, account_name_type start, follow::follow_type type, uint32_t limit ) const
{
   follow::get_followers_args args{
      .account = account,
      .start   = start,
      .type    = type,
      .limit   = limit
   };
   return send_call( "follow_api", "get_followers", fc::variant(args) )
      .as<follow::get_followers_return>().followers;
}

/**
 * @brief Retrieves accounts that a specific account is following
 * @param account Account to retrieve following list for
 * @param start Starting following account name for pagination
 * @param type Follow type filter (blog, ignore, etc.)
 * @param limit Maximum number of following accounts to return
 * @return Vector of follow objects showing who the account follows
 */
vector<follow::api_follow_object> remote_node_api::get_following( account_name_type account, account_name_type start, follow::follow_type type, uint32_t limit ) const
{
   follow::get_following_args args{
      .account = account,
      .start   = start,
      .type    = type,
      .limit   = limit
   };
   return send_call( "follow_api", "get_following", fc::variant(args) )
      .as<follow::get_following_return>().following;
}

/**
 * @brief Retrieves follow counts for an account
 * @param account Account to retrieve counts for
 * @return Follow count object with follower count, following count, etc.
 */
follow::get_follow_count_return remote_node_api::get_follow_count( account_name_type account ) const
{
   follow::get_follow_count_args args{ .account = account };
   return send_call( "follow_api", "get_follow_count", fc::variant(args) )
      .as<follow::get_follow_count_return>();
}

/**
 * @brief Retrieves feed entries (author/permlink pairs) for an account
 * @param account Account to retrieve feed for
 * @param start_entry_id Starting entry ID for pagination
 * @param limit Maximum number of entries to return
 * @return Vector of feed entry objects (metadata only, no content)
 */
vector<follow::feed_entry> remote_node_api::get_feed_entries( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const
{
   follow::get_feed_entries_args args{
      .account        = account,
      .start_entry_id = start_entry_id,
      .limit          = limit
   };
   return send_call( "follow_api", "get_feed_entries", fc::variant(args) )
      .as<follow::get_feed_entries_return>().feed;
}

/**
 * @brief Retrieves feed with full comment content for an account
 * @param account Account to retrieve feed for
 * @param start_entry_id Starting entry ID for pagination
 * @param limit Maximum number of entries to return
 * @return Vector of comment feed entries with full content from followed accounts
 */
vector<follow::comment_feed_entry> remote_node_api::get_feed( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const
{
   follow::get_feed_args args{
      .account        = account,
      .start_entry_id = start_entry_id,
      .limit          = limit
   };
   return send_call( "follow_api", "get_feed", fc::variant(args) )
      .as<follow::get_feed_return>().feed;
}

/**
 * @brief Retrieves blog entries (author/permlink pairs) for an account
 * @param account Account to retrieve blog for
 * @param start_entry_id Starting entry ID for pagination
 * @param limit Maximum number of entries to return
 * @return Vector of blog entry objects (metadata only, no content)
 */
vector<follow::blog_entry> remote_node_api::get_blog_entries( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const
{
   follow::get_blog_entries_args args{
      .account        = account,
      .start_entry_id = start_entry_id,
      .limit          = limit
   };
   return send_call( "follow_api", "get_blog_entries", fc::variant(args) )
      .as<follow::get_blog_entries_return>().blog;
}

/**
 * @brief Retrieves blog with full comment content for an account
 * @param account Account to retrieve blog for
 * @param start_entry_id Starting entry ID for pagination
 * @param limit Maximum number of entries to return
 * @return Vector of comment blog entries with full content authored or reblogged by the account
 */
vector<follow::comment_blog_entry> remote_node_api::get_blog( account_name_type account, uint32_t start_entry_id, uint32_t limit ) const
{
   follow::get_blog_args args{
      .account        = account,
      .start_entry_id = start_entry_id,
      .limit          = limit
   };
   return send_call( "follow_api", "get_blog", fc::variant(args) )
      .as<follow::get_blog_return>().blog;
}

/**
 * @brief Retrieves accounts that have reblogged a specific post
 * @param author Author of the post
 * @param permlink Permlink of the post
 * @return Vector of account names that reblogged the post
 */
vector<account_name_type> remote_node_api::get_reblogged_by( account_name_type author, string permlink ) const
{
   follow::get_reblogged_by_args args{
      .author   = author,
      .permlink = permlink
   };
   return send_call( "follow_api", "get_reblogged_by", fc::variant(args) )
      .as<follow::get_reblogged_by_return>().accounts;
}

/**
 * @brief Retrieves authors that appear in an account's blog with reblog counts
 * @param account Account to retrieve blog authors for
 * @return Vector of reblog count objects showing authors and how many times they appear in the blog
 */
vector<follow::reblog_count> remote_node_api::get_blog_authors( account_name_type account ) const
{
   follow::get_blog_authors_args args{ .blog_account = account };
   return send_call( "follow_api", "get_blog_authors", fc::variant(args) )
      .as<follow::get_blog_authors_return>().blog_authors;
}

// ============================================================================
// Reputation API
// ============================================================================

/**
 * @brief Retrieves reputation scores for accounts
 * @param lower_bound_name Starting account name for pagination
 * @param limit Maximum number of reputations to return
 * @return Vector of account reputation objects with account names and reputation scores
 */
vector<reputation::account_reputation> remote_node_api::get_account_reputations( account_name_type lower_bound_name, uint32_t limit ) const
{
   reputation::get_account_reputations_args args{
      .account_lower_bound = lower_bound_name,
      .limit               = limit
   };
   return send_call( "reputation_api", "get_account_reputations", fc::variant(args) )
      .as<reputation::get_account_reputations_return>().reputations;
}

// ============================================================================
// Market History API
// ============================================================================

/**
 * @brief Retrieves current market ticker information
 * @return Ticker data including latest price, highest bid, lowest ask, volume, etc.
 */
market_history::get_ticker_return remote_node_api::get_ticker() const
{
   market_history::get_ticker_args args;
   return send_call( "market_history_api", "get_ticker", fc::variant(args) )
      .as<market_history::get_ticker_return>();
}

/**
 * @brief Retrieves 24-hour trading volume
 * @return Volume data for ZTR and ZBD traded in the last 24 hours
 */
market_history::get_volume_return remote_node_api::get_volume() const
{
   market_history::get_volume_args args;
   return send_call( "market_history_api", "get_volume", fc::variant(args) )
      .as<market_history::get_volume_return>();
}

/**
 * @brief Retrieves the current order book (bids and asks)
 * @param limit Maximum number of orders to return per side (bids and asks)
 * @return Order book with bids and asks up to the specified limit
 */
market_history::get_order_book_return remote_node_api::get_order_book( uint32_t limit ) const
{
   market_history::get_order_book_args args{ .limit = limit };
   return send_call( "market_history_api", "get_order_book", fc::variant(args) )
      .as<market_history::get_order_book_return>();
}

/**
 * @brief Retrieves trade history within a time range
 * @param start Starting timestamp
 * @param end Ending timestamp
 * @param limit Maximum number of trades to return
 * @return Vector of market trades executed between start and end times
 */
vector<market_history::market_trade> remote_node_api::get_trade_history( fc::time_point_sec start, fc::time_point_sec end, uint32_t limit ) const
{
   market_history::get_trade_history_args args{
      .start = start,
      .end   = end,
      .limit = limit
   };
   return send_call( "market_history_api", "get_trade_history", fc::variant(args) )
      .as<market_history::get_trade_history_return>().trades;
}

/**
 * @brief Retrieves most recent trades
 * @param limit Maximum number of recent trades to return
 * @return Vector of most recent market trades
 */
vector<market_history::market_trade> remote_node_api::get_recent_trades( uint32_t limit ) const
{
   market_history::get_recent_trades_args args{ .limit = limit };
   return send_call( "market_history_api", "get_recent_trades", fc::variant(args) )
      .as<market_history::get_recent_trades_return>().trades;
}

/**
 * @brief Retrieves market history aggregated into time buckets
 * @param bucket_seconds Bucket size in seconds (e.g., 60 for 1-minute buckets)
 * @param start Starting timestamp
 * @param end Ending timestamp
 * @return Vector of bucket objects containing OHLCV (Open, High, Low, Close, Volume) data
 */
vector<market_history::bucket_object> remote_node_api::get_market_history( uint32_t bucket_seconds, fc::time_point_sec start, fc::time_point_sec end ) const
{
   market_history::get_market_history_args args{
      .bucket_seconds = bucket_seconds,
      .start          = start,
      .end            = end
   };
   return send_call( "market_history_api", "get_market_history", fc::variant(args) )
      .as<market_history::get_market_history_return>().buckets;
}

/**
 * @brief Retrieves available bucket sizes for market history
 * @return Set of bucket sizes (in seconds) supported by the API
 */
fc::flat_set<uint32_t> remote_node_api::get_market_history_buckets() const
{
   market_history::get_market_history_buckets_args args;
   return send_call( "market_history_api", "get_market_history_buckets", fc::variant(args) )
      .as<market_history::get_market_history_buckets_return>().bucket_sizes;
}

// ============================================================================
// Network Broadcast API
// ============================================================================

/**
 * @brief Broadcasts a signed transaction to the network
 * @param trx Signed transaction to broadcast
 * @note This is asynchronous - does not wait for transaction confirmation
 */
void remote_node_api::broadcast_transaction( signed_transaction trx ) const
{
   network_broadcast_api::broadcast_transaction_args args{ .trx = trx };
   send_call( "network_broadcast_api", "broadcast_transaction", fc::variant(args) );
}

/**
 * @brief Broadcasts a signed transaction and waits for confirmation
 * @param trx Signed transaction to broadcast
 * @return Transaction result with transaction ID, block number, and transaction number
 * @note Currently behaves the same as broadcast_transaction (asynchronous) as the synchronous variant is not available
 */
broadcast_transaction_synchronous_return remote_node_api::broadcast_transaction_synchronous( signed_transaction trx ) const
{
   network_broadcast_api::broadcast_transaction_args args{ .trx = trx };
   // network_broadcast_api doesn't have synchronous variant
   // Just broadcast and return a result based on transaction
   send_call( "network_broadcast_api", "broadcast_transaction", fc::variant(args) );

   broadcast_transaction_synchronous_return result;
   
   result.id        = trx.id();
   result.block_num = 0; // Unknown until confirmed
   result.trx_num   = 0;
   result.expired   = false;

   return result;
}

/**
 * @brief Broadcasts a signed block to the network
 * @param block Signed block to broadcast (typically used by witnesses)
 * @note This is primarily used by witnesses to publish blocks they have produced
 */
void remote_node_api::broadcast_block( signed_block block ) const
{
   network_broadcast_api::broadcast_block_args args{ .block = block };
   send_call( "network_broadcast_api", "broadcast_block", fc::variant(args) );
}

// ============================================================================
// Helper Functions
// ============================================================================

fc::variant remote_node_api::send_call( const string& api_name, const string& method_name, const fc::variant& args ) const
{
   return _connection.send_call( api_name, method_name, args );
}

} } // zattera::wallet

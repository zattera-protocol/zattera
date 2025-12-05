#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <zattera/protocol/exceptions.hpp>
#include <zattera/protocol/hardfork.hpp>

#include <zattera/chain/block_summary_object.hpp>
#include <zattera/chain/database.hpp>
#include <zattera/chain/database_exceptions.hpp>
#include <zattera/chain/history_object.hpp>
#include <zattera/chain/zattera_objects.hpp>

#include <zattera/chain/utils/reward.hpp>

#include <zattera/plugins/debug_node/debug_node_plugin.hpp>
#include <zattera/plugins/witness/witness_objects.hpp>

#include <fc/macros.hpp>
#include <fc/crypto/digest.hpp>

#include "../../fixtures/database_fixture.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace zattera;
using namespace zattera::chain;
using namespace zattera::protocol;
using fc::string;

BOOST_FIXTURE_TEST_SUITE( operation_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( validate_comment )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: comment_validate" );


      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_comment_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: comment_authorities" );

      ACTORS( (alice)(bob) );
      generate_blocks( 60 / ZATTERA_BLOCK_INTERVAL );

      comment_operation op;
      op.author = "alice";
      op.permlink = "lorem";
      op.parent_author = "";
      op.parent_permlink = "ipsum";
      op.title = "Lorem Ipsum";
      op.body = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
      op.json_metadata = "{\"foo\":\"bar\"}";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_posting_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.sign( alice_post_key, db->get_chain_id() );
      tx.sign( alice_post_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test success with post signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_posting_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_comment )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: comment_apply" );

      ACTORS( (alice)(bob)(sam) )
      generate_blocks( 60 / ZATTERA_BLOCK_INTERVAL );

      comment_operation op;
      op.author = "alice";
      op.permlink = "lorem";
      op.parent_author = "";
      op.parent_permlink = "ipsum";
      op.title = "Lorem Ipsum";
      op.body = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
      op.json_metadata = "{\"foo\":\"bar\"}";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test Alice posting a root comment" );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const comment_object& alice_comment = db->get_comment( "alice", string( "lorem" ) );

      BOOST_REQUIRE( alice_comment.author == op.author );
      BOOST_REQUIRE( to_string( alice_comment.permlink ) == op.permlink );
      BOOST_REQUIRE( to_string( alice_comment.parent_permlink ) == op.parent_permlink );
      BOOST_REQUIRE( alice_comment.last_update == db->head_block_time() );
      BOOST_REQUIRE( alice_comment.created == db->head_block_time() );
      BOOST_REQUIRE( alice_comment.net_rshares.value == 0 );
      BOOST_REQUIRE( alice_comment.abs_rshares.value == 0 );
      BOOST_REQUIRE( alice_comment.cashout_time == fc::time_point_sec( db->head_block_time() + fc::seconds( ZATTERA_CASHOUT_WINDOW_SECONDS ) ) );

      #ifndef IS_LOW_MEM
         const auto& alice_comment_content = db->get< comment_content_object, by_comment >( alice_comment.id );
         BOOST_REQUIRE( to_string( alice_comment_content.title ) == op.title );
         BOOST_REQUIRE( to_string( alice_comment_content.body ) == op.body );
         BOOST_REQUIRE( to_string( alice_comment_content.json_metadata ) == op.json_metadata );
      #else
         const auto* alice_comment_content = db->find< comment_content_object, by_comment >( alice_comment.id );
         BOOST_REQUIRE( alice_comment_content == nullptr );
      #endif

      validate_database();

      BOOST_TEST_MESSAGE( "--- Test Bob posting a comment on a non-existent comment" );
      op.author = "bob";
      op.permlink = "ipsum";
      op.parent_author = "alice";
      op.parent_permlink = "foobar";

      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test Bob posting a comment on Alice's comment" );
      op.parent_permlink = "lorem";

      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const comment_object& bob_comment = db->get_comment( "bob", string( "ipsum" ) );

      BOOST_REQUIRE( bob_comment.author == op.author );
      BOOST_REQUIRE( to_string( bob_comment.permlink ) == op.permlink );
      BOOST_REQUIRE( bob_comment.parent_author == op.parent_author );
      BOOST_REQUIRE( to_string( bob_comment.parent_permlink ) == op.parent_permlink );
      BOOST_REQUIRE( bob_comment.last_update == db->head_block_time() );
      BOOST_REQUIRE( bob_comment.created == db->head_block_time() );
      BOOST_REQUIRE( bob_comment.net_rshares.value == 0 );
      BOOST_REQUIRE( bob_comment.abs_rshares.value == 0 );
      BOOST_REQUIRE( bob_comment.cashout_time == bob_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
      BOOST_REQUIRE( bob_comment.root_comment == alice_comment.id );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test Sam posting a comment on Bob's comment" );

      op.author = "sam";
      op.permlink = "dolor";
      op.parent_author = "bob";
      op.parent_permlink = "ipsum";

      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const comment_object& sam_comment = db->get_comment( "sam", string( "dolor" ) );

      BOOST_REQUIRE( sam_comment.author == op.author );
      BOOST_REQUIRE( to_string( sam_comment.permlink ) == op.permlink );
      BOOST_REQUIRE( sam_comment.parent_author == op.parent_author );
      BOOST_REQUIRE( to_string( sam_comment.parent_permlink ) == op.parent_permlink );
      BOOST_REQUIRE( sam_comment.last_update == db->head_block_time() );
      BOOST_REQUIRE( sam_comment.created == db->head_block_time() );
      BOOST_REQUIRE( sam_comment.net_rshares.value == 0 );
      BOOST_REQUIRE( sam_comment.abs_rshares.value == 0 );
      BOOST_REQUIRE( sam_comment.cashout_time == sam_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
      BOOST_REQUIRE( sam_comment.root_comment == alice_comment.id );
      validate_database();

      generate_blocks( 60 * 5 / ZATTERA_BLOCK_INTERVAL + 1 );

      BOOST_TEST_MESSAGE( "--- Test modifying a comment" );
      const auto& mod_sam_comment = db->get_comment( "sam", string( "dolor" ) );
      const auto& mod_bob_comment = db->get_comment( "bob", string( "ipsum" ) );
      const auto& mod_alice_comment = db->get_comment( "alice", string( "lorem" ) );

      FC_UNUSED(mod_bob_comment, mod_alice_comment);

      fc::time_point_sec created = mod_sam_comment.created;

      db->modify( mod_sam_comment, [&]( comment_object& com )
      {
         com.net_rshares = 10;
         com.abs_rshares = 10;
      });

      db->modify( db->get_dynamic_global_properties(), [&]( dynamic_global_property_object& o)
      {
         o.total_reward_shares2 = zattera::chain::util::evaluate_reward_curve( 10 );
      });

      tx.signatures.clear();
      tx.operations.clear();
      op.title = "foo";
      op.body = "bar";
      op.json_metadata = "{\"bar\":\"foo\"}";
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( mod_sam_comment.author == op.author );
      BOOST_REQUIRE( to_string( mod_sam_comment.permlink ) == op.permlink );
      BOOST_REQUIRE( mod_sam_comment.parent_author == op.parent_author );
      BOOST_REQUIRE( to_string( mod_sam_comment.parent_permlink ) == op.parent_permlink );
      BOOST_REQUIRE( mod_sam_comment.last_update == db->head_block_time() );
      BOOST_REQUIRE( mod_sam_comment.created == created );
      BOOST_REQUIRE( mod_sam_comment.cashout_time == mod_sam_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure posting withing 1 minute" );

      op.permlink = "sit";
      op.parent_author = "";
      op.parent_permlink = "test";
      tx.operations.clear();
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( 60 * 5 / ZATTERA_BLOCK_INTERVAL );

      op.permlink = "amet";
      tx.operations.clear();
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      validate_database();

      generate_block();
      db->push_transaction( tx, 0 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_comment_delete )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: comment_delete_apply" );
      ACTORS( (alice) )
      generate_block();

      vest( "alice", ASSET( "1000.000 TTR" ) );

      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      signed_transaction tx;
      comment_operation comment;
      vote_operation vote;

      comment.author = "alice";
      comment.permlink = "test1";
      comment.title = "test";
      comment.body = "foo bar";
      comment.parent_permlink = "test";
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test1";
      vote.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failue deleting a comment with positive rshares" );

      delete_comment_operation op;
      op.author = "alice";
      op.permlink = "test1";
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test success deleting a comment with negative rshares" );

      generate_block();
      vote.weight = -1 * ZATTERA_100_PERCENT;
      tx.clear();
      tx.operations.push_back( vote );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto test_comment = db->find< comment_object, by_permlink >( boost::make_tuple( "alice", string( "test1" ) ) );
      BOOST_REQUIRE( test_comment == nullptr );


      BOOST_TEST_MESSAGE( "--- Test failure deleting a comment past cashout" );
      generate_blocks( ZATTERA_MIN_ROOT_COMMENT_INTERVAL.to_seconds() / ZATTERA_BLOCK_INTERVAL );

      tx.clear();
      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( ZATTERA_CASHOUT_WINDOW_SECONDS / ZATTERA_BLOCK_INTERVAL );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test1" ) ).cashout_time == fc::time_point_sec::maximum() );

      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test failure deleting a comment with a reply" );

      comment.permlink = "test2";
      comment.parent_author = "alice";
      comment.parent_permlink = "test1";
      tx.clear();
      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( ZATTERA_MIN_ROOT_COMMENT_INTERVAL.to_seconds() / ZATTERA_BLOCK_INTERVAL );
      comment.permlink = "test3";
      comment.parent_permlink = "test2";
      tx.clear();
      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.permlink = "test2";
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.set_reference_block( db->head_block_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_vote )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: vote_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_vote_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: vote_authorities" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_vote )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: vote_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      generate_block();

      vest( "alice", ASSET( "10.000 TTR" ) );
      validate_database();
      vest( "bob" , ASSET( "10.000 TTR" ) );
      vest( "sam" , ASSET( "10.000 TTR" ) );
      vest( "dave" , ASSET( "10.000 TTR" ) );
      generate_block();

      const auto& vote_idx = db->get_index< comment_vote_index >().indices().get< by_comment_voter >();

      {
         const auto& alice = db->get_account( "alice" );

         signed_transaction tx;
         comment_operation comment_op;
         comment_op.author = "alice";
         comment_op.permlink = "foo";
         comment_op.parent_permlink = "test";
         comment_op.title = "bar";
         comment_op.body = "foo bar";
         tx.operations.push_back( comment_op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         BOOST_TEST_MESSAGE( "--- Testing voting on a non-existent comment" );

         tx.operations.clear();
         tx.signatures.clear();

         vote_operation op;
         op.voter = "alice";
         op.author = "bob";
         op.permlink = "foo";
         op.weight = ZATTERA_100_PERCENT;
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );

         ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

         validate_database();

         BOOST_TEST_MESSAGE( "--- Testing voting with a weight of 0" );

         op.weight = (int16_t) 0;
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );

         ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

         validate_database();

         BOOST_TEST_MESSAGE( "--- Testing success" );

         auto old_voting_power = alice.voting_power;

         op.weight = ZATTERA_100_PERCENT;
         op.author = "alice";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );

         db->push_transaction( tx, 0 );

         auto& alice_comment = db->get_comment( "alice", string( "foo" ) );
         auto itr = vote_idx.find( std::make_tuple( alice_comment.id, alice.id ) );
         int64_t max_vote_denom = ( db->get_dynamic_global_properties().vote_power_reserve_rate * ZATTERA_VOTE_REGENERATION_SECONDS ) / (60*60*24);

         BOOST_REQUIRE( alice.voting_power == old_voting_power - ( ( old_voting_power + max_vote_denom - 1 ) / max_vote_denom ) );
         BOOST_REQUIRE( alice.last_vote_time == db->head_block_time() );
         BOOST_REQUIRE( alice_comment.net_rshares.value == alice.vesting_shares.amount.value * ( old_voting_power - alice.voting_power ) / ZATTERA_100_PERCENT - ZATTERA_VOTE_DUST_THRESHOLD );
         BOOST_REQUIRE( alice_comment.cashout_time == alice_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( itr->rshares == alice.vesting_shares.amount.value * ( old_voting_power - alice.voting_power ) / ZATTERA_100_PERCENT - ZATTERA_VOTE_DUST_THRESHOLD );
         BOOST_REQUIRE( itr != vote_idx.end() );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test reduced power for quick voting" );

         generate_blocks( db->head_block_time() + ZATTERA_MIN_VOTE_INTERVAL_SEC );

         old_voting_power = db->get_account( "alice" ).voting_power;

         comment_op.author = "bob";
         comment_op.permlink = "foo";
         comment_op.title = "bar";
         comment_op.body = "foo bar";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( comment_op );
         tx.sign( bob_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         op.weight = ZATTERA_100_PERCENT / 2;
         op.voter = "alice";
         op.author = "bob";
         op.permlink = "foo";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         const auto& bob_comment = db->get_comment( "bob", string( "foo" ) );
         itr = vote_idx.find( std::make_tuple( bob_comment.id, alice.id ) );

         BOOST_REQUIRE( db->get_account( "alice" ).voting_power == old_voting_power - ( ( old_voting_power + max_vote_denom - 1 ) * ZATTERA_100_PERCENT / ( 2 * max_vote_denom * ZATTERA_100_PERCENT ) ) );
         BOOST_REQUIRE( bob_comment.net_rshares.value == alice.vesting_shares.amount.value * ( old_voting_power - db->get_account( "alice" ).voting_power ) / ZATTERA_100_PERCENT - ZATTERA_VOTE_DUST_THRESHOLD );
         BOOST_REQUIRE( bob_comment.cashout_time == bob_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( itr != vote_idx.end() );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test payout time extension on vote" );

         old_voting_power = db->get_account( "bob" ).voting_power;
         auto old_abs_rshares = db->get_comment( "alice", string( "foo" ) ).abs_rshares.value;

         generate_blocks( db->head_block_time() + fc::seconds( ( ZATTERA_CASHOUT_WINDOW_SECONDS / 2 ) ), true );

         const auto& new_bob = db->get_account( "bob" );
         const auto& new_alice_comment = db->get_comment( "alice", string( "foo" ) );

         op.weight = ZATTERA_100_PERCENT;
         op.voter = "bob";
         op.author = "alice";
         op.permlink = "foo";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( bob_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         itr = vote_idx.find( std::make_tuple( new_alice_comment.id, new_bob.id ) );
         uint128_t new_cashout_time = db->head_block_time().sec_since_epoch() + ZATTERA_CASHOUT_WINDOW_SECONDS;

         BOOST_REQUIRE( new_bob.voting_power == ZATTERA_100_PERCENT - ( ( ZATTERA_100_PERCENT + max_vote_denom - 1 ) / max_vote_denom ) );
         BOOST_REQUIRE( new_alice_comment.net_rshares.value == old_abs_rshares + new_bob.vesting_shares.amount.value * ( old_voting_power - new_bob.voting_power ) / ZATTERA_100_PERCENT - ZATTERA_VOTE_DUST_THRESHOLD );
         BOOST_REQUIRE( new_alice_comment.cashout_time == new_alice_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( itr != vote_idx.end() );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test negative vote" );

         const auto& new_sam = db->get_account( "sam" );
         const auto& new_bob_comment = db->get_comment( "bob", string( "foo" ) );

         old_abs_rshares = new_bob_comment.abs_rshares.value;

         op.weight = -1 * ZATTERA_100_PERCENT / 2;
         op.voter = "sam";
         op.author = "bob";
         op.permlink = "foo";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( sam_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         itr = vote_idx.find( std::make_tuple( new_bob_comment.id, new_sam.id ) );
         new_cashout_time = db->head_block_time().sec_since_epoch() + ZATTERA_CASHOUT_WINDOW_SECONDS;
         auto sam_weight /*= ( ( uint128_t( new_sam.vesting_shares.amount.value ) ) / 400 + 1 ).to_uint64();*/
                         = ( ( uint128_t( new_sam.vesting_shares.amount.value ) * ( ( ZATTERA_100_PERCENT + max_vote_denom - 1 ) / ( 2 * max_vote_denom ) ) ) / ZATTERA_100_PERCENT ).to_uint64() - ZATTERA_VOTE_DUST_THRESHOLD;

         BOOST_REQUIRE( new_sam.voting_power == ZATTERA_100_PERCENT - ( ( ZATTERA_100_PERCENT + max_vote_denom - 1 ) / ( 2 * max_vote_denom ) ) );
         BOOST_REQUIRE( static_cast<uint64_t>(new_bob_comment.net_rshares.value) == old_abs_rshares - sam_weight );
         BOOST_REQUIRE( static_cast<uint64_t>(new_bob_comment.abs_rshares.value) == old_abs_rshares + sam_weight );
         BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( itr != vote_idx.end() );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test nested voting on nested comments" );

         old_abs_rshares = new_alice_comment.children_abs_rshares.value;
         int64_t regenerated_power = (ZATTERA_100_PERCENT * ( db->head_block_time() - db->get_account( "alice").last_vote_time ).to_seconds() ) / ZATTERA_VOTE_REGENERATION_SECONDS;
         int64_t used_power = ( db->get_account( "alice" ).voting_power + regenerated_power + max_vote_denom - 1 ) / max_vote_denom;

         comment_op.author = "sam";
         comment_op.permlink = "foo";
         comment_op.title = "bar";
         comment_op.body = "foo bar";
         comment_op.parent_author = "alice";
         comment_op.parent_permlink = "foo";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( comment_op );
         tx.sign( sam_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         op.weight = ZATTERA_100_PERCENT;
         op.voter = "alice";
         op.author = "sam";
         op.permlink = "foo";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         auto new_rshares = ( ( fc::uint128_t( db->get_account( "alice" ).vesting_shares.amount.value ) * used_power ) / ZATTERA_100_PERCENT ).to_uint64() - ZATTERA_VOTE_DUST_THRESHOLD;

         BOOST_REQUIRE( db->get_comment( "alice", string( "foo" ) ).cashout_time == db->get_comment( "alice", string( "foo" ) ).created + ZATTERA_CASHOUT_WINDOW_SECONDS );

         validate_database();

         BOOST_TEST_MESSAGE( "--- Test increasing vote rshares" );

         generate_blocks( db->head_block_time() + ZATTERA_MIN_VOTE_INTERVAL_SEC );

         auto new_alice = db->get_account( "alice" );
         auto alice_bob_vote = vote_idx.find( std::make_tuple( new_bob_comment.id, new_alice.id ) );
         auto old_vote_rshares = alice_bob_vote->rshares;
         auto old_net_rshares = new_bob_comment.net_rshares.value;
         old_abs_rshares = new_bob_comment.abs_rshares.value;
         used_power = ( ( ZATTERA_1_PERCENT * 25 * ( new_alice.voting_power ) / ZATTERA_100_PERCENT ) + max_vote_denom - 1 ) / max_vote_denom;
         auto alice_voting_power = new_alice.voting_power - used_power;

         op.voter = "alice";
         op.weight = ZATTERA_1_PERCENT * 25;
         op.author = "bob";
         op.permlink = "foo";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
         alice_bob_vote = vote_idx.find( std::make_tuple( new_bob_comment.id, new_alice.id ) );

         new_rshares = ( ( fc::uint128_t( new_alice.vesting_shares.amount.value ) * used_power ) / ZATTERA_100_PERCENT ).to_uint64() - ZATTERA_VOTE_DUST_THRESHOLD;

         BOOST_REQUIRE( new_bob_comment.net_rshares == old_net_rshares - old_vote_rshares + new_rshares );
         BOOST_REQUIRE( new_bob_comment.abs_rshares == old_abs_rshares + new_rshares );
         BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( static_cast<uint64_t>(alice_bob_vote->rshares) == new_rshares );
         BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );
         BOOST_REQUIRE( alice_bob_vote->vote_percent == op.weight );
         BOOST_REQUIRE( db->get_account( "alice" ).voting_power == alice_voting_power );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test decreasing vote rshares" );

         generate_blocks( db->head_block_time() + ZATTERA_MIN_VOTE_INTERVAL_SEC );

         old_vote_rshares = new_rshares;
         old_net_rshares = new_bob_comment.net_rshares.value;
         old_abs_rshares = new_bob_comment.abs_rshares.value;
         used_power = ( uint64_t( ZATTERA_1_PERCENT ) * 75 * uint64_t( alice_voting_power ) ) / ZATTERA_100_PERCENT;
         used_power = ( used_power + max_vote_denom - 1 ) / max_vote_denom;
         alice_voting_power -= used_power;

         op.weight = ZATTERA_1_PERCENT * -75;
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
         alice_bob_vote = vote_idx.find( std::make_tuple( new_bob_comment.id, new_alice.id ) );

         new_rshares = ( ( fc::uint128_t( new_alice.vesting_shares.amount.value ) * used_power ) / ZATTERA_100_PERCENT ).to_uint64() - ZATTERA_VOTE_DUST_THRESHOLD;

         BOOST_REQUIRE( new_bob_comment.net_rshares == old_net_rshares - old_vote_rshares - new_rshares );
         BOOST_REQUIRE( new_bob_comment.abs_rshares == old_abs_rshares + new_rshares );
         BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( alice_bob_vote->rshares == -1 * static_cast<int64_t>(new_rshares) );
         BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );
         BOOST_REQUIRE( alice_bob_vote->vote_percent == op.weight );
         BOOST_REQUIRE( db->get_account( "alice" ).voting_power == alice_voting_power );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test changing a vote to 0 weight (aka: removing a vote)" );

         generate_blocks( db->head_block_time() + ZATTERA_MIN_VOTE_INTERVAL_SEC );

         old_vote_rshares = alice_bob_vote->rshares;
         old_net_rshares = new_bob_comment.net_rshares.value;
         old_abs_rshares = new_bob_comment.abs_rshares.value;

         op.weight = 0;
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
         alice_bob_vote = vote_idx.find( std::make_tuple( new_bob_comment.id, new_alice.id ) );

         BOOST_REQUIRE( new_bob_comment.net_rshares == old_net_rshares - old_vote_rshares );
         BOOST_REQUIRE( new_bob_comment.abs_rshares == old_abs_rshares );
         BOOST_REQUIRE( new_bob_comment.cashout_time == new_bob_comment.created + ZATTERA_CASHOUT_WINDOW_SECONDS );
         BOOST_REQUIRE( alice_bob_vote->rshares == 0 );
         BOOST_REQUIRE( alice_bob_vote->last_update == db->head_block_time() );
         BOOST_REQUIRE( alice_bob_vote->vote_percent == op.weight );
         BOOST_REQUIRE( db->get_account( "alice" ).voting_power == alice_voting_power );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test failure when increasing rshares within lockout period" );

         generate_blocks( fc::time_point_sec( ( new_bob_comment.cashout_time - ZATTERA_UPVOTE_LOCKOUT ).sec_since_epoch() + ZATTERA_BLOCK_INTERVAL ), true );

         op.weight = ZATTERA_100_PERCENT;
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( alice_private_key, db->get_chain_id() );

         ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test success when reducing rshares within lockout period" );

         op.weight = -1 * ZATTERA_100_PERCENT;
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
         validate_database();

         BOOST_TEST_MESSAGE( "--- Test failure with a new vote within lockout period" );

         op.weight = ZATTERA_100_PERCENT;
         op.voter = "dave";
         tx.operations.clear();
         tx.signatures.clear();
         tx.operations.push_back( op );
         tx.sign( dave_private_key, db->get_chain_id() );
         ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( equalize_comment_payout )
{
   try
   {
      ACTORS( (alice)(bob)(dave)
              (ulysses)(vivian)(wendy) )

      struct author_actor
      {
         author_actor(
            const std::string& n,
            fc::ecc::private_key pk,
            fc::optional<asset> mpay = fc::optional<asset>() )
            : name(n), private_key(pk), max_accepted_payout(mpay) {}
         std::string             name;
         fc::ecc::private_key    private_key;
         fc::optional< asset >   max_accepted_payout;
      };

      struct voter_actor
      {
         voter_actor( const std::string& n, fc::ecc::private_key pk, std::string fa )
            : name(n), private_key(pk), favorite_author(fa) {}
         std::string             name;
         fc::ecc::private_key    private_key;
         std::string             favorite_author;
      };


      std::vector< author_actor > authors;
      std::vector< voter_actor > voters;

      authors.emplace_back( "alice", alice_private_key );
      authors.emplace_back( "bob"  , bob_private_key, ASSET( "0.000 TBD" ) );
      authors.emplace_back( "dave" , dave_private_key );
      voters.emplace_back( "ulysses", ulysses_private_key, "alice");
      voters.emplace_back( "vivian" , vivian_private_key , "bob"  );
      voters.emplace_back( "wendy"  , wendy_private_key  , "dave" );

      // A,B,D : posters
      // U,V,W : voters

      // set a ridiculously high ZTR price ($1 / satoshi) to disable dust threshold
      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "0.001 TTR" ) ) );

      for( const auto& voter : voters )
      {
         fund( voter.name, 10000 );
         vest( voter.name, 10000 );
      }

      // authors all write in the same block, but Bob declines payout
      for( const auto& author : authors )
      {
         signed_transaction tx;
         comment_operation com;
         com.author = author.name;
         com.permlink = "mypost";
         com.parent_author = ZATTERA_ROOT_POST_PARENT;
         com.parent_permlink = "test";
         com.title = "Hello from "+author.name;
         com.body = "Hello, my name is "+author.name;
         tx.operations.push_back( com );

         if( author.max_accepted_payout.valid() )
         {
            comment_options_operation copt;
            copt.author = com.author;
            copt.permlink = com.permlink;
            copt.max_accepted_payout = *(author.max_accepted_payout);
            tx.operations.push_back( copt );
         }

         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( author.private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      }

      generate_blocks(1);

      // voters all vote in the same block with the same stake
      for( const auto& voter : voters )
      {
         signed_transaction tx;
         vote_operation vote;
         vote.voter = voter.name;
         vote.author = voter.favorite_author;
         vote.permlink = "mypost";
         vote.weight = ZATTERA_100_PERCENT;
         tx.operations.push_back( vote );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( voter.private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      }

      //auto reward_ztr = db->get_dynamic_global_properties().total_reward_fund_ztr;

      // generate a few blocks to seed the reward fund
      generate_blocks(10);
      //const auto& rf = db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME );
      //idump( (rf) );

      generate_blocks( db->get_comment( "alice", string( "mypost" ) ).cashout_time, true );
      /*
      for( const auto& author : authors )
      {
         const account_object& a = db->get_account(author.name);
         ilog( "${n} : ${ztr} ${zbd}", ("n", author.name)("ztr", a.reward_ztr_balance)("zbd", a.reward_zbd_balance) );
      }
      for( const auto& voter : voters )
      {
         const account_object& a = db->get_account(voter.name);
         ilog( "${n} : ${ztr} ${zbd}", ("n", voter.name)("ztr", a.reward_ztr_balance)("zbd", a.reward_zbd_balance) );
      }
      */

      const account_object& alice_account = db->get_account("alice");
      const account_object& bob_account   = db->get_account("bob");
      const account_object& dave_account  = db->get_account("dave");

      BOOST_CHECK( alice_account.reward_zbd_balance == ASSET( "375000.000 TBD" ) );
      BOOST_CHECK( bob_account.reward_zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_CHECK( dave_account.reward_zbd_balance == alice_account.reward_zbd_balance );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( handle_comment_payout_dust )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: comment_payout_dust" );

      ACTORS( (alice)(bob) )
      generate_block();

      vest( "alice", ASSET( "10.000 TTR" ) );
      vest( "bob", ASSET( "10.000 TTR" ) );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      generate_block();
      validate_database();

      comment_operation comment;
      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "test";
      comment.body = "test";

      signed_transaction tx;
      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      comment.author = "bob";

      tx.clear();
      tx.operations.push_back( comment );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      generate_blocks( db->head_block_time() + ZATTERA_REVERSE_AUCTION_WINDOW_SECONDS );

      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = 81 * ZATTERA_1_PERCENT;

      tx.clear();
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      vote.voter = "bob";
      vote.author = "bob";
      vote.weight = 59 * ZATTERA_1_PERCENT;

      tx.clear();
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time );

      // If comments are paid out independent of order, then the last satoshi of ZTR cannot be divided among them
      const auto rf = db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME );
      BOOST_REQUIRE( rf.reward_balance == ASSET( "0.001 TTR" ) );

      validate_database();

      BOOST_TEST_MESSAGE( "Done" );
   }
   FC_LOG_AND_RETHROW()
}

/*BOOST_AUTO_TEST_CASE( process_comment_payout )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 7500 );
      vest( "bob", 7500 );
      fund( "sam", 8000 );
      vest( "sam", 8000 );
      fund( "dave", 5000 );
      vest( "dave", 5000 );

      price exchange_rate( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;

      BOOST_TEST_MESSAGE( "Creating comments." );

      comment_operation com;
      com.author = "alice";
      com.permlink = "test";
      com.parent_author = "";
      com.parent_permlink = "test";
      com.title = "foo";
      com.body = "bar";
      tx.operations.push_back( com );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      com.author = "bob";
      tx.operations.push_back( com );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Voting for comments." );

      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "sam";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "bob";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "dave";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generate blocks up until first payout" );

      //generate_blocks( db->get_comment( "bob", string( "test" ) ).cashout_time - ZATTERA_BLOCK_INTERVAL, true );

      auto reward_ztr = db->get_dynamic_global_properties().total_reward_fund_ztr + ASSET( "1.667 TTR" );
      auto total_rshares2 = db->get_dynamic_global_properties().total_reward_shares2;
      auto bob_comment_rshares = db->get_comment( "bob", string( "test" ) ).net_rshares;
      auto bob_vest_shares = db->get_account( "bob" ).vesting_shares;
      auto bob_zbd_balance = db->get_account( "bob" ).zbd_balance;

      auto bob_comment_payout = asset( ( ( uint128_t( bob_comment_rshares.value ) * bob_comment_rshares.value * reward_ztr.amount.value ) / total_rshares2 ).to_uint64(), ZTR_SYMBOL );
      auto bob_comment_discussion_rewards = asset( bob_comment_payout.amount / 4, ZTR_SYMBOL );
      bob_comment_payout -= bob_comment_discussion_rewards;
      auto bob_comment_zbd_reward = db->to_zbd( asset( bob_comment_payout.amount / 2, ZTR_SYMBOL ) );
      auto bob_comment_vesting_reward = ( bob_comment_payout - asset( bob_comment_payout.amount / 2, ZTR_SYMBOL) ) * db->get_dynamic_global_properties().get_vesting_share_price();

      BOOST_TEST_MESSAGE( "Cause first payout" );

      generate_block();

      BOOST_REQUIRE( db->get_dynamic_global_properties().total_reward_fund_ztr == reward_ztr - bob_comment_payout );
      BOOST_REQUIRE( db->get_comment( "bob", string( "test" ) ).total_payout_value == bob_comment_vesting_reward * db->get_dynamic_global_properties().get_vesting_share_price() + bob_comment_zbd_reward * exchange_rate );
      BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares == bob_vest_shares + bob_comment_vesting_reward );
      BOOST_REQUIRE( db->get_account( "bob" ).zbd_balance == bob_zbd_balance + bob_comment_zbd_reward );

      BOOST_TEST_MESSAGE( "Testing no payout when less than $0.02" );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      vote.author = "bob";
      vote.weight = ZATTERA_1_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "bob", string( "test" ) ).cashout_time - ZATTERA_BLOCK_INTERVAL, true );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      vote.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      bob_vest_shares = db->get_account( "bob" ).vesting_shares;
      bob_zbd_balance = db->get_account( "bob" ).zbd_balance;

      validate_database();

      generate_block();

      BOOST_REQUIRE( bob_vest_shares.amount.value == db->get_account( "bob" ).vesting_shares.amount.value );
      BOOST_REQUIRE( bob_zbd_balance.amount.value == db->get_account( "bob" ).zbd_balance.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}*/

/*
BOOST_AUTO_TEST_CASE( process_comment_payout )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 7500 );
      vest( "bob", 7500 );
      fund( "sam", 8000 );
      vest( "sam", 8000 );
      fund( "dave", 5000 );
      vest( "dave", 5000 );

      price exchange_rate( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      auto gpo = db->get_dynamic_global_properties();

      signed_transaction tx;

      BOOST_TEST_MESSAGE( "Creating comments. " );

      comment_operation com;
      com.author = "alice";
      com.permlink = "test";
      com.parent_permlink = "test";
      com.title = "foo";
      com.body = "bar";
      tx.operations.push_back( com );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      com.author = "bob";
      tx.operations.push_back( com );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "First round of votes." );

      tx.operations.clear();
      tx.signatures.clear();
      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating blocks..." );

      generate_blocks( fc::time_point_sec( db->head_block_time().sec_since_epoch() + ZATTERA_CASHOUT_WINDOW_SECONDS / 2 ), true );

      BOOST_TEST_MESSAGE( "Second round of votes." );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "bob";
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating more blocks..." );

      generate_blocks( db->get_comment( "bob", string( "test" ) ).cashout_time - ( ZATTERA_BLOCK_INTERVAL / 2 ), true );

      BOOST_TEST_MESSAGE( "Check comments have not been paid out." );

      const auto& vote_idx = db->get_index< comment_vote_index >().indices().get< by_comment_voter >();

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "bob" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "sam" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "dave" ) ).id  ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "bob" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "sam" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value > 0 );
      BOOST_REQUIRE( db->get_comment( "bob", string( "test" ) ).net_rshares.value > 0 );
      validate_database();

      auto reward_ztr = db->get_dynamic_global_properties().total_reward_fund_ztr + ASSET( "2.000 TTR" );
      auto total_rshares2 = db->get_dynamic_global_properties().total_reward_shares2;
      auto bob_comment_vote_total = db->get_comment( "bob", string( "test" ) ).total_vote_weight;
      auto bob_comment_rshares = db->get_comment( "bob", string( "test" ) ).net_rshares;
      auto bob_zbd_balance = db->get_account( "bob" ).zbd_balance;
      auto alice_vest_shares = db->get_account( "alice" ).vesting_shares;
      auto bob_vest_shares = db->get_account( "bob" ).vesting_shares;
      auto sam_vest_shares = db->get_account( "sam" ).vesting_shares;
      auto dave_vest_shares = db->get_account( "dave" ).vesting_shares;

      auto bob_comment_payout = asset( ( ( uint128_t( bob_comment_rshares.value ) * bob_comment_rshares.value * reward_ztr.amount.value ) / total_rshares2 ).to_uint64(), ZTR_SYMBOL );
      auto bob_comment_vote_rewards = asset( bob_comment_payout.amount / 2, ZTR_SYMBOL );
      bob_comment_payout -= bob_comment_vote_rewards;
      auto bob_comment_zbd_reward = asset( bob_comment_payout.amount / 2, ZTR_SYMBOL ) * exchange_rate;
      auto bob_comment_vesting_reward = ( bob_comment_payout - asset( bob_comment_payout.amount / 2, ZTR_SYMBOL ) ) * db->get_dynamic_global_properties().get_vesting_share_price();
      auto unclaimed_payments = bob_comment_vote_rewards;
      auto alice_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "alice" ) ).id ) )->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ), ZTR_SYMBOL );
      auto alice_vote_vesting = alice_vote_reward * db->get_dynamic_global_properties().get_vesting_share_price();
      auto bob_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "bob" ) ).id ) )->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ), ZTR_SYMBOL );
      auto bob_vote_vesting = bob_vote_reward * db->get_dynamic_global_properties().get_vesting_share_price();
      auto sam_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "sam" ) ).id ) )->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ), ZTR_SYMBOL );
      auto sam_vote_vesting = sam_vote_reward * db->get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments -= ( alice_vote_reward + bob_vote_reward + sam_vote_reward );

      BOOST_TEST_MESSAGE( "Generate one block to cause a payout" );

      generate_block();

      auto bob_comment_reward = get_last_operations( 1 )[0].get< comment_reward_operation >();

      BOOST_REQUIRE( db->get_dynamic_global_properties().total_reward_fund_ztr.amount.value == reward_ztr.amount.value - ( bob_comment_payout + bob_comment_vote_rewards - unclaimed_payments ).amount.value );
      BOOST_REQUIRE( db->get_comment( "bob", string( "test" ) ).total_payout_value.amount.value == ( ( bob_comment_vesting_reward * db->get_dynamic_global_properties().get_vesting_share_price() ) + ( bob_comment_zbd_reward * exchange_rate ) ).amount.value );
      BOOST_REQUIRE( db->get_account( "bob" ).zbd_balance.amount.value == ( bob_zbd_balance + bob_comment_zbd_reward ).amount.value );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value > 0 );
      BOOST_REQUIRE( db->get_comment( "bob", string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == ( alice_vest_shares + alice_vote_vesting ).amount.value );
      BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == ( bob_vest_shares + bob_vote_vesting + bob_comment_vesting_reward ).amount.value );
      BOOST_REQUIRE( db->get_account( "sam" ).vesting_shares.amount.value == ( sam_vest_shares + sam_vote_vesting ).amount.value );
      BOOST_REQUIRE( db->get_account( "dave" ).vesting_shares.amount.value == dave_vest_shares.amount.value );
      BOOST_REQUIRE( bob_comment_reward.author == "bob" );
      BOOST_REQUIRE( bob_comment_reward.permlink == "test" );
      BOOST_REQUIRE( bob_comment_reward.payout.amount.value == bob_comment_zbd_reward.amount.value );
      BOOST_REQUIRE( bob_comment_reward.vesting_payout.amount.value == bob_comment_vesting_reward.amount.value );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "bob" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "sam" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "dave" ) ).id  ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "bob" ) ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "sam" ) ).id   ) ) == vote_idx.end() );
      validate_database();

      BOOST_TEST_MESSAGE( "Generating blocks up to next comment payout" );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time - ( ZATTERA_BLOCK_INTERVAL / 2 ), true );

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "bob" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "sam" ) ).id   ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "dave" ) ).id  ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "bob" ) ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "sam" ) ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value > 0 );
      BOOST_REQUIRE( db->get_comment( "bob", string( "test" ) ).net_rshares.value == 0 );
      validate_database();

      BOOST_TEST_MESSAGE( "Generate block to cause payout" );

      reward_ztr = db->get_dynamic_global_properties().total_reward_fund_ztr + ASSET( "2.000 TTR" );
      total_rshares2 = db->get_dynamic_global_properties().total_reward_shares2;
      auto alice_comment_vote_total = db->get_comment( "alice", string( "test" ) ).total_vote_weight;
      auto alice_comment_rshares = db->get_comment( "alice", string( "test" ) ).net_rshares;
      auto alice_zbd_balance = db->get_account( "alice" ).zbd_balance;
      alice_vest_shares = db->get_account( "alice" ).vesting_shares;
      bob_vest_shares = db->get_account( "bob" ).vesting_shares;
      sam_vest_shares = db->get_account( "sam" ).vesting_shares;
      dave_vest_shares = db->get_account( "dave" ).vesting_shares;

      u256 rs( alice_comment_rshares.value );
      u256 rf( reward_ztr.amount.value );
      u256 trs2 = total_rshares2.hi;
      trs2 = ( trs2 << 64 ) + total_rshares2.lo;
      auto rs2 = rs*rs;

      auto alice_comment_payout = asset( static_cast< uint64_t >( ( rf * rs2 ) / trs2 ), ZTR_SYMBOL );
      auto alice_comment_vote_rewards = asset( alice_comment_payout.amount / 2, ZTR_SYMBOL );
      alice_comment_payout -= alice_comment_vote_rewards;
      auto alice_comment_zbd_reward = asset( alice_comment_payout.amount / 2, ZTR_SYMBOL ) * exchange_rate;
      auto alice_comment_vesting_reward = ( alice_comment_payout - asset( alice_comment_payout.amount / 2, ZTR_SYMBOL ) ) * db->get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments = alice_comment_vote_rewards;
      alice_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "alice" ) ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), ZTR_SYMBOL );
      alice_vote_vesting = alice_vote_reward * db->get_dynamic_global_properties().get_vesting_share_price();
      bob_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "bob" ) ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), ZTR_SYMBOL );
      bob_vote_vesting = bob_vote_reward * db->get_dynamic_global_properties().get_vesting_share_price();
      sam_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "sam" ) ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), ZTR_SYMBOL );
      sam_vote_vesting = sam_vote_reward * db->get_dynamic_global_properties().get_vesting_share_price();
      auto dave_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "dave" ) ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), ZTR_SYMBOL );
      auto dave_vote_vesting = dave_vote_reward * db->get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments -= ( alice_vote_reward + bob_vote_reward + sam_vote_reward + dave_vote_reward );

      generate_block();
      auto alice_comment_reward = get_last_operations( 1 )[0].get< comment_reward_operation >();

      BOOST_REQUIRE( ( db->get_dynamic_global_properties().total_reward_fund_ztr + alice_comment_payout + alice_comment_vote_rewards - unclaimed_payments ).amount.value == reward_ztr.amount.value );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).total_payout_value.amount.value == ( ( alice_comment_vesting_reward * db->get_dynamic_global_properties().get_vesting_share_price() ) + ( alice_comment_zbd_reward * exchange_rate ) ).amount.value );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance.amount.value == ( alice_zbd_balance + alice_comment_zbd_reward ).amount.value );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == ( alice_vest_shares + alice_vote_vesting + alice_comment_vesting_reward ).amount.value );
      BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == ( bob_vest_shares + bob_vote_vesting ).amount.value );
      BOOST_REQUIRE( db->get_account( "sam" ).vesting_shares.amount.value == ( sam_vest_shares + sam_vote_vesting ).amount.value );
      BOOST_REQUIRE( db->get_account( "dave" ).vesting_shares.amount.value == ( dave_vest_shares + dave_vote_vesting ).amount.value );
      BOOST_REQUIRE( alice_comment_reward.author == "alice" );
      BOOST_REQUIRE( alice_comment_reward.permlink == "test" );
      BOOST_REQUIRE( alice_comment_reward.payout.amount.value == alice_comment_zbd_reward.amount.value );
      BOOST_REQUIRE( alice_comment_reward.vesting_payout.amount.value == alice_comment_vesting_reward.amount.value );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "bob" ) ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "sam" ) ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "dave" ) ).id  ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "bob" ) ).id   ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id,   db->get_account( "sam" ) ).id   ) ) == vote_idx.end() );
      validate_database();

      BOOST_TEST_MESSAGE( "Testing no payout when less than $0.02" );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      vote.author = "bob";
      vote.weight = ZATTERA_1_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "bob", string( "test" ) ).cashout_time - ZATTERA_BLOCK_INTERVAL, true );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      vote.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      bob_vest_shares = db->get_account( "bob" ).vesting_shares;
      auto bob_zbd = db->get_account( "bob" ).zbd_balance;

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "dave" ) ).id ) ) != vote_idx.end() );
      validate_database();

      generate_block();

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "dave" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( bob_vest_shares.amount.value == db->get_account( "bob" ).vesting_shares.amount.value );
      BOOST_REQUIRE( bob_zbd.amount.value == db->get_account( "bob" ).zbd_balance.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( handle_nested_comments )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 10000 );
      vest( "bob", 10000 );
      fund( "sam", 10000 );
      vest( "sam", 10000 );
      fund( "dave", 10000 );
      vest( "dave", 10000 );

      price exchange_rate( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;
      comment_operation comment_op;
      comment_op.author = "alice";
      comment_op.permlink = "test";
      comment_op.parent_permlink = "test";
      comment_op.title = "foo";
      comment_op.body = "bar";
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( comment_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      comment_op.author = "bob";
      comment_op.parent_author = "alice";
      comment_op.parent_permlink = "test";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      comment_op.author = "sam";
      comment_op.parent_author = "bob";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      comment_op.author = "dave";
      comment_op.parent_author = "sam";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote_operation vote_op;
      vote_op.voter = "alice";
      vote_op.author = "alice";
      vote_op.permlink = "test";
      vote_op.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( vote_op );
      vote_op.author = "bob";
      tx.operations.push_back( vote_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote_op.voter = "bob";
      vote_op.author = "alice";
      tx.operations.push_back( vote_op );
      vote_op.author = "bob";
      tx.operations.push_back( vote_op );
      vote_op.author = "dave";
      vote_op.weight = ZATTERA_1_PERCENT * 20;
      tx.operations.push_back( vote_op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote_op.voter = "sam";
      vote_op.author = "bob";
      vote_op.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( vote_op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time - fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );

      auto gpo = db->get_dynamic_global_properties();
      uint128_t reward_ztr = gpo.total_reward_fund_ztr.amount.value + ASSET( "2.000 TTR" ).amount.value;
      uint128_t total_rshares2 = gpo.total_reward_shares2;

      auto alice_comment = db->get_comment( "alice", string( "test" ) );
      auto bob_comment = db->get_comment( "bob", string( "test" ) );
      auto sam_comment = db->get_comment( "sam", string( "test" ) );
      auto dave_comment = db->get_comment( "dave", string( "test" ) );

      const auto& vote_idx = db->get_index< comment_vote_index >().indices().get< by_comment_voter >();

      // Calculate total comment rewards and voting rewards.
      auto alice_comment_reward = ( ( reward_ztr * alice_comment.net_rshares.value * alice_comment.net_rshares.value ) / total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( alice_comment.net_rshares.value ) * ( alice_comment.net_rshares.value );
      reward_ztr -= alice_comment_reward;
      auto alice_comment_vote_rewards = alice_comment_reward / 2;
      alice_comment_reward -= alice_comment_vote_rewards;

      auto alice_vote_alice_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "alice" ) ).id ) )->weight ) * alice_comment_vote_rewards ) / alice_comment.total_vote_weight ), ZTR_SYMBOL );
      auto bob_vote_alice_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "alice", string( "test" ).id, db->get_account( "bob" ) ).id ) )->weight ) * alice_comment_vote_rewards ) / alice_comment.total_vote_weight ), ZTR_SYMBOL );
      reward_ztr += alice_comment_vote_rewards - ( alice_vote_alice_reward + bob_vote_alice_reward ).amount.value;

      auto bob_comment_reward = ( ( reward_ztr * bob_comment.net_rshares.value * bob_comment.net_rshares.value ) / total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( bob_comment.net_rshares.value ) * bob_comment.net_rshares.value;
      reward_ztr -= bob_comment_reward;
      auto bob_comment_vote_rewards = bob_comment_reward / 2;
      bob_comment_reward -= bob_comment_vote_rewards;

      auto alice_vote_bob_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "alice" ) ).id ) )->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), ZTR_SYMBOL );
      auto bob_vote_bob_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "bob" ) ).id ) )->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), ZTR_SYMBOL );
      auto sam_vote_bob_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "bob", string( "test" ).id, db->get_account( "sam" ) ).id ) )->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), ZTR_SYMBOL );
      reward_ztr += bob_comment_vote_rewards - ( alice_vote_bob_reward + bob_vote_bob_reward + sam_vote_bob_reward ).amount.value;

      auto dave_comment_reward = ( ( reward_ztr * dave_comment.net_rshares.value * dave_comment.net_rshares.value ) / total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( dave_comment.net_rshares.value ) * dave_comment.net_rshares.value;
      reward_ztr -= dave_comment_reward;
      auto dave_comment_vote_rewards = dave_comment_reward / 2;
      dave_comment_reward -= dave_comment_vote_rewards;

      auto bob_vote_dave_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db->get_comment( "dave", string( "test" ).id, db->get_account( "bob" ) ).id ) )->weight ) * dave_comment_vote_rewards ) / dave_comment.total_vote_weight ), ZTR_SYMBOL );
      reward_ztr += dave_comment_vote_rewards - bob_vote_dave_reward.amount.value;

      // Calculate rewards paid to parent posts
      auto alice_pays_alice_zbd = alice_comment_reward / 2;
      auto alice_pays_alice_vest = alice_comment_reward - alice_pays_alice_zbd;
      auto bob_pays_bob_zbd = bob_comment_reward / 2;
      auto bob_pays_bob_vest = bob_comment_reward - bob_pays_bob_zbd;
      auto dave_pays_dave_zbd = dave_comment_reward / 2;
      auto dave_pays_dave_vest = dave_comment_reward - dave_pays_dave_zbd;

      auto bob_pays_alice_zbd = bob_pays_bob_zbd / 2;
      auto bob_pays_alice_vest = bob_pays_bob_vest / 2;
      bob_pays_bob_zbd -= bob_pays_alice_zbd;
      bob_pays_bob_vest -= bob_pays_alice_vest;

      auto dave_pays_sam_zbd = dave_pays_dave_zbd / 2;
      auto dave_pays_sam_vest = dave_pays_dave_vest / 2;
      dave_pays_dave_zbd -= dave_pays_sam_zbd;
      dave_pays_dave_vest -= dave_pays_sam_vest;
      auto dave_pays_bob_zbd = dave_pays_sam_zbd / 2;
      auto dave_pays_bob_vest = dave_pays_sam_vest / 2;
      dave_pays_sam_zbd -= dave_pays_bob_zbd;
      dave_pays_sam_vest -= dave_pays_bob_vest;
      auto dave_pays_alice_zbd = dave_pays_bob_zbd / 2;
      auto dave_pays_alice_vest = dave_pays_bob_vest / 2;
      dave_pays_bob_zbd -= dave_pays_alice_zbd;
      dave_pays_bob_vest -= dave_pays_alice_vest;

      // Calculate total comment payouts
      auto alice_comment_total_payout = db->to_zbd( asset( alice_pays_alice_zbd + alice_pays_alice_vest, ZTR_SYMBOL ) );
      alice_comment_total_payout += db->to_zbd( asset( bob_pays_alice_zbd + bob_pays_alice_vest, ZTR_SYMBOL ) );
      alice_comment_total_payout += db->to_zbd( asset( dave_pays_alice_zbd + dave_pays_alice_vest, ZTR_SYMBOL ) );
      auto bob_comment_total_payout = db->to_zbd( asset( bob_pays_bob_zbd + bob_pays_bob_vest, ZTR_SYMBOL ) );
      bob_comment_total_payout += db->to_zbd( asset( dave_pays_bob_zbd + dave_pays_bob_vest, ZTR_SYMBOL ) );
      auto sam_comment_total_payout = db->to_zbd( asset( dave_pays_sam_zbd + dave_pays_sam_vest, ZTR_SYMBOL ) );
      auto dave_comment_total_payout = db->to_zbd( asset( dave_pays_dave_zbd + dave_pays_dave_vest, ZTR_SYMBOL ) );

      auto alice_starting_vesting = db->get_account( "alice" ).vesting_shares;
      auto alice_starting_zbd = db->get_account( "alice" ).zbd_balance;
      auto bob_starting_vesting = db->get_account( "bob" ).vesting_shares;
      auto bob_starting_zbd = db->get_account( "bob" ).zbd_balance;
      auto sam_starting_vesting = db->get_account( "sam" ).vesting_shares;
      auto sam_starting_zbd = db->get_account( "sam" ).zbd_balance;
      auto dave_starting_vesting = db->get_account( "dave" ).vesting_shares;
      auto dave_starting_zbd = db->get_account( "dave" ).zbd_balance;

      generate_block();

      gpo = db->get_dynamic_global_properties();

      // Calculate vesting share rewards from voting.
      auto alice_vote_alice_vesting = alice_vote_alice_reward * gpo.get_vesting_share_price();
      auto bob_vote_alice_vesting = bob_vote_alice_reward * gpo.get_vesting_share_price();
      auto alice_vote_bob_vesting = alice_vote_bob_reward * gpo.get_vesting_share_price();
      auto bob_vote_bob_vesting = bob_vote_bob_reward * gpo.get_vesting_share_price();
      auto sam_vote_bob_vesting = sam_vote_bob_reward * gpo.get_vesting_share_price();
      auto bob_vote_dave_vesting = bob_vote_dave_reward * gpo.get_vesting_share_price();

      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).total_payout_value.amount.value == alice_comment_total_payout.amount.value );
      BOOST_REQUIRE( db->get_comment( "bob", string( "test" ) ).total_payout_value.amount.value == bob_comment_total_payout.amount.value );
      BOOST_REQUIRE( db->get_comment( "sam", string( "test" ) ).total_payout_value.amount.value == sam_comment_total_payout.amount.value );
      BOOST_REQUIRE( db->get_comment( "dave", string( "test" ) ).total_payout_value.amount.value == dave_comment_total_payout.amount.value );

      // ops 0-3, 5-6, and 10 are comment reward ops
      auto ops = get_last_operations( 13 );

      BOOST_TEST_MESSAGE( "Checking Virtual Operation Correctness" );

      curate_reward_operation cur_vop;
      comment_reward_operation com_vop = ops[0].get< comment_reward_operation >();

      BOOST_REQUIRE( com_vop.author == "alice" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_alice_zbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_alice_vest );

      com_vop = ops[1].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "bob" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_bob_zbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_bob_vest );

      com_vop = ops[2].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "sam" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_sam_zbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_sam_vest );

      com_vop = ops[3].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "dave" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_dave_zbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_dave_vest );

      cur_vop = ops[4].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "bob" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == bob_vote_dave_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "dave" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      com_vop = ops[5].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "alice" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "bob" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == bob_pays_alice_zbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == bob_pays_alice_vest );

      com_vop = ops[6].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "bob" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "bob" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == bob_pays_bob_zbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == bob_pays_bob_vest );

      cur_vop = ops[7].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "sam" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == sam_vote_bob_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "bob" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      cur_vop = ops[8].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "bob" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == bob_vote_bob_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "bob" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      cur_vop = ops[9].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "alice" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == alice_vote_bob_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "bob" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      com_vop = ops[10].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "alice" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "alice" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == alice_pays_alice_zbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == alice_pays_alice_vest );

      cur_vop = ops[11].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "bob" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == bob_vote_alice_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "alice" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      cur_vop = ops[12].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "alice" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == alice_vote_alice_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "alice" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      BOOST_TEST_MESSAGE( "Checking account balances" );

      auto alice_total_zbd = alice_starting_zbd + asset( alice_pays_alice_zbd + bob_pays_alice_zbd + dave_pays_alice_zbd, ZTR_SYMBOL ) * exchange_rate;
      auto alice_total_vesting = alice_starting_vesting + asset( alice_pays_alice_vest + bob_pays_alice_vest + dave_pays_alice_vest + alice_vote_alice_reward.amount + alice_vote_bob_reward.amount, ZTR_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance.amount.value == alice_total_zbd.amount.value );
      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares.amount.value == alice_total_vesting.amount.value );

      auto bob_total_zbd = bob_starting_zbd + asset( bob_pays_bob_zbd + dave_pays_bob_zbd, ZTR_SYMBOL ) * exchange_rate;
      auto bob_total_vesting = bob_starting_vesting + asset( bob_pays_bob_vest + dave_pays_bob_vest + bob_vote_alice_reward.amount + bob_vote_bob_reward.amount + bob_vote_dave_reward.amount, ZTR_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE( db->get_account( "bob" ).zbd_balance.amount.value == bob_total_zbd.amount.value );
      BOOST_REQUIRE( db->get_account( "bob" ).vesting_shares.amount.value == bob_total_vesting.amount.value );

      auto sam_total_zbd = sam_starting_zbd + asset( dave_pays_sam_zbd, ZTR_SYMBOL ) * exchange_rate;
      auto sam_total_vesting = bob_starting_vesting + asset( dave_pays_sam_vest + sam_vote_bob_reward.amount, ZTR_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE( db->get_account( "sam" ).zbd_balance.amount.value == sam_total_zbd.amount.value );
      BOOST_REQUIRE( db->get_account( "sam" ).vesting_shares.amount.value == sam_total_vesting.amount.value );

      auto dave_total_zbd = dave_starting_zbd + asset( dave_pays_dave_zbd, ZTR_SYMBOL ) * exchange_rate;
      auto dave_total_vesting = dave_starting_vesting + asset( dave_pays_dave_vest, ZTR_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE( db->get_account( "dave" ).zbd_balance.amount.value == dave_total_zbd.amount.value );
      BOOST_REQUIRE( db->get_account( "dave" ).vesting_shares.amount.value == dave_total_vesting.amount.value );
   }
   FC_LOG_AND_RETHROW()
}
*/

BOOST_AUTO_TEST_CASE( freeze_comment )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      fund( "bob", 10000 );
      fund( "sam", 10000 );
      fund( "dave", 10000 );

      vest( "alice", 10000 );
      vest( "bob", 10000 );
      vest( "sam", 10000 );
      vest( "dave", 10000 );

      auto exchange_rate = price( ASSET( "1.000 TBD" ), ASSET( "1.250 TTR" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;

      comment_operation comment;
      comment.author = "alice";
      comment.parent_author = "";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.body = "test";

      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      comment.body = "test2";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      vote_operation vote;
      vote.weight = ZATTERA_100_PERCENT;
      vote.voter = "bob";
      vote.author = "alice";
      vote.permlink = "test";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).last_payout == fc::time_point_sec::min() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).cashout_time != fc::time_point_sec::min() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).cashout_time != fc::time_point_sec::maximum() );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time, true );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).last_payout == db->head_block_time() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).cashout_time == fc::time_point_sec::maximum() );

      vote.voter = "sam";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).cashout_time == fc::time_point_sec::maximum() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).abs_rshares.value == 0 );

      vote.voter = "bob";
      vote.weight = ZATTERA_100_PERCENT * -1;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).cashout_time == fc::time_point_sec::maximum() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).abs_rshares.value == 0 );

      vote.voter = "dave";
      vote.weight = 0;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( dave_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).cashout_time == fc::time_point_sec::maximum() );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db->get_comment( "alice", string( "test" ) ).abs_rshares.value == 0 );

      comment.body = "test4";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( comment );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 ); // Works now in #1714
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_comment_beneficiaries )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test Comment Beneficiaries Validate" );
      comment_options_operation op;

      op.author = "alice";
      op.permlink = "test";

      BOOST_TEST_MESSAGE( "--- Testing more than 100% weight on a single route" );
      comment_payout_beneficiaries b;
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), ZATTERA_100_PERCENT + 1 ) );
      op.extensions.insert( b );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing more than 100% total weight" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), ZATTERA_1_PERCENT * 75 ) );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "sam" ), ZATTERA_1_PERCENT * 75 ) );
      op.extensions.clear();
      op.extensions.insert( b );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing maximum number of routes" );
      b.beneficiaries.clear();
      for( size_t i = 0; i < 127; i++ )
      {
         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "foo" + fc::to_string( i ) ), 1 ) );
      }

      op.extensions.clear();
      std::sort( b.beneficiaries.begin(), b.beneficiaries.end() );
      op.extensions.insert( b );
      op.validate();

      BOOST_TEST_MESSAGE( "--- Testing one too many routes" );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bar" ), 1 ) );
      std::sort( b.beneficiaries.begin(), b.beneficiaries.end() );
      op.extensions.clear();
      op.extensions.insert( b );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Testing duplicate accounts" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( "bob", ZATTERA_1_PERCENT * 2 ) );
      b.beneficiaries.push_back( beneficiary_route_type( "bob", ZATTERA_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing incorrect account sort order" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( "bob", ZATTERA_1_PERCENT ) );
      b.beneficiaries.push_back( beneficiary_route_type( "alice", ZATTERA_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing correct account sort order" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( "alice", ZATTERA_1_PERCENT ) );
      b.beneficiaries.push_back( beneficiary_route_type( "bob", ZATTERA_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_comment_beneficiaries )
{
   try
   {
      BOOST_TEST_MESSAGE( "Test Comment Beneficiaries" );
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "bob", 10000 );
      vest( "bob", 10000 );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      comment_operation comment;
      vote_operation vote;
      comment_options_operation op;
      comment_payout_beneficiaries b;
      signed_transaction tx;

      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "test";
      comment.body = "foobar";

      tx.operations.push_back( comment );
      tx.set_expiration( db->head_block_time() + ZATTERA_MIN_TRANSACTION_EXPIRATION_LIMIT );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx );

      BOOST_TEST_MESSAGE( "--- Test failure on more than 8 benefactors" );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), ZATTERA_1_PERCENT ) );

      for( size_t i = 0; i < 8; i++ )
      {
         b.beneficiaries.push_back( beneficiary_route_type( account_name_type( ZATTERA_GENESIS_WITNESS_NAME + fc::to_string( i ) ), ZATTERA_1_PERCENT ) );
      }

      op.author = "alice";
      op.permlink = "test";
      op.allow_curation_rewards = false;
      op.extensions.insert( b );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), chain::plugin_exception );


      BOOST_TEST_MESSAGE( "--- Test specifying a non-existent benefactor" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "doug" ), ZATTERA_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test setting when comment has been voted on" );
      vote.author = "alice";
      vote.permlink = "test";
      vote.voter = "bob";
      vote.weight = ZATTERA_100_PERCENT;

      const auto& bob_account = db->get_account( "bob" );
      idump( (bob_account.vesting_shares) );
      idump( (bob_account.reward_vesting_ztr) );
      idump( (bob_account.reward_vesting_balance) );

      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "bob" ), 25 * ZATTERA_1_PERCENT ) );
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "sam" ), 50 * ZATTERA_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );

      tx.clear();
      tx.operations.push_back( vote );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test success" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx );


      BOOST_TEST_MESSAGE( "--- Test setting when there are already beneficiaries" );
      b.beneficiaries.clear();
      b.beneficiaries.push_back( beneficiary_route_type( account_name_type( "dave" ), 25 * ZATTERA_1_PERCENT ) );
      op.extensions.clear();
      op.extensions.insert( b );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Payout and verify rewards were split properly" );
      tx.clear();
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time - ZATTERA_BLOCK_INTERVAL );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_dynamic_global_properties(), [=]( dynamic_global_property_object& gpo )
         {
            gpo.current_supply -= gpo.total_reward_fund_ztr;
            gpo.total_reward_fund_ztr = ASSET( "100.000 TTR" );
            gpo.current_supply += gpo.total_reward_fund_ztr;
         });
      });

      generate_block();

      BOOST_REQUIRE( db->get_account( "bob" ).reward_ztr_balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).reward_zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).reward_vesting_ztr.amount + db->get_account( "sam" ).reward_vesting_ztr.amount == db->get_comment( "alice", string( "test" ) ).beneficiary_payout_value.amount );
      BOOST_REQUIRE( ( db->get_account( "alice" ).reward_zbd_balance.amount + db->get_account( "alice" ).reward_vesting_ztr.amount ) == db->get_account( "bob" ).reward_vesting_ztr.amount + 1 );
      BOOST_REQUIRE( ( db->get_account( "alice" ).reward_zbd_balance.amount + db->get_account( "alice" ).reward_vesting_ztr.amount ) * 2 == db->get_account( "sam" ).reward_vesting_ztr.amount + 1 );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

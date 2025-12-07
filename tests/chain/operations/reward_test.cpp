#ifdef IS_TEST_MODE
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

BOOST_AUTO_TEST_CASE( validate_reward_balance_claim )
{
   try
   {
      claim_reward_balance_operation op;
      op.account = "alice";
      op.reward_ztr = ASSET( "0.000 TTR" );
      op.reward_zbd = ASSET( "0.000 TBD" );
      op.reward_vests = ASSET( "0.000000 VESTS" );


      BOOST_TEST_MESSAGE( "Testing all 0 amounts" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );


      BOOST_TEST_MESSAGE( "Testing single reward claims" );
      op.reward_ztr.amount = 1000;
      op.validate();

      op.reward_ztr.amount = 0;
      op.reward_zbd.amount = 1000;
      op.validate();

      op.reward_zbd.amount = 0;
      op.reward_vests.amount = 1000;
      op.validate();

      op.reward_vests.amount = 0;


      BOOST_TEST_MESSAGE( "Testing wrong ZTR symbol" );
      op.reward_ztr = ASSET( "1.000 TBD" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );


      BOOST_TEST_MESSAGE( "Testing wrong ZBD symbol" );
      op.reward_ztr = ASSET( "1.000 TTR" );
      op.reward_zbd = ASSET( "1.000 TTR" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );


      BOOST_TEST_MESSAGE( "Testing wrong VESTS symbol" );
      op.reward_zbd = ASSET( "1.000 TBD" );
      op.reward_vests = ASSET( "1.000 TTR" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );


      BOOST_TEST_MESSAGE( "Testing a single negative amount" );
      op.reward_ztr.amount = 1000;
      op.reward_zbd.amount = -1000;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_reward_balance_claim_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: decline_voting_rights_authorities" );

      claim_reward_balance_operation op;
      op.account = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_reward_balance_claim )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: claim_reward_balance_apply" );
      BOOST_TEST_MESSAGE( "--- Setting up test state" );

      ACTORS( (alice) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      db_plugin->debug_update( []( database& db )
      {
         db.modify( db.get_account( "alice" ), []( account_object& a )
         {
            a.reward_ztr_balance = ASSET( "10.000 TTR" );
            a.reward_zbd_balance = ASSET( "10.000 TBD" );
            a.reward_vesting_balance = ASSET( "10.000000 VESTS" );
            a.reward_vesting_ztr = ASSET( "10.000 TTR" );
         });

         db.modify( db.get_dynamic_global_properties(), []( dynamic_global_property_object& gpo )
         {
            gpo.current_supply += ASSET( "20.000 TTR" );
            gpo.current_zbd_supply += ASSET( "10.000 TBD" );
            gpo.virtual_supply += ASSET( "20.000 TTR" );
            gpo.pending_rewarded_vesting_shares += ASSET( "10.000000 VESTS" );
            gpo.pending_rewarded_vesting_ztr += ASSET( "10.000 TTR" );
         });
      });

      generate_block();
      validate_database();

      auto alice_ztr = db->get_account( "alice" ).balance;
      auto alice_zbd = db->get_account( "alice" ).zbd_balance;
      auto alice_vests = db->get_account( "alice" ).vesting_shares;


      BOOST_TEST_MESSAGE( "--- Attempting to claim more ZTR than exists in the reward balance." );

      claim_reward_balance_operation op;
      signed_transaction tx;

      op.account = "alice";
      op.reward_ztr = ASSET( "20.000 TTR" );
      op.reward_zbd = ASSET( "0.000 TBD" );
      op.reward_vests = ASSET( "0.000000 VESTS" );

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Claiming a partial reward balance" );

      op.reward_ztr = ASSET( "0.000 TTR" );
      op.reward_vests = ASSET( "5.000000 VESTS" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == alice_ztr + op.reward_ztr );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_ztr_balance == ASSET( "10.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == alice_zbd + op.reward_zbd );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_zbd_balance == ASSET( "10.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares == alice_vests + op.reward_vests );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_balance == ASSET( "5.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_ztr == ASSET( "5.000 TTR" ) );
      validate_database();

      alice_vests += op.reward_vests;


      BOOST_TEST_MESSAGE( "--- Claiming the full reward balance" );

      op.reward_ztr = ASSET( "10.000 TTR" );
      op.reward_zbd = ASSET( "10.000 TBD" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == alice_ztr + op.reward_ztr );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_ztr_balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == alice_zbd + op.reward_zbd );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).vesting_shares == alice_vests + op.reward_vests );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_balance == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).reward_vesting_ztr == ASSET( "0.000 TTR" ) );
            validate_database();
   }
   FC_LOG_AND_RETHROW()
}

/*
BOOST_AUTO_TEST_CASE( distribute_reward_funds )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: reward_funds" );

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) ) );
      generate_block();

      comment_operation comment;
      vote_operation vote;
      signed_transaction tx;

      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "foo";
      comment.body = "bar";
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( 5 );

      comment.author = "bob";
      comment.parent_author = "alice";
      vote.voter = "bob";
      vote.author = "bob";
      tx.clear();
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time );

      {
         const auto& post_rf = db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME );
         const auto& comment_rf = db->get< reward_fund_object, by_name >( ZATTERA_COMMENT_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.reward_balance.amount == 0 );
         BOOST_REQUIRE( comment_rf.reward_balance.amount > 0 );
         BOOST_REQUIRE( db->get_account( "alice" ).reward_zbd_balance.amount > 0 );
         BOOST_REQUIRE( db->get_account( "bob" ).reward_zbd_balance.amount == 0 );
         validate_database();
      }

      generate_blocks( db->get_comment( "bob", string( "test" ) ).cashout_time );

      {
         const auto& post_rf = db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME );
         const auto& comment_rf = db->get< reward_fund_object, by_name >( ZATTERA_COMMENT_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.reward_balance.amount > 0 );
         BOOST_REQUIRE( comment_rf.reward_balance.amount == 0 );
         BOOST_REQUIRE( db->get_account( "alice" ).reward_zbd_balance.amount > 0 );
         BOOST_REQUIRE( db->get_account( "bob" ).reward_zbd_balance.amount > 0 );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW()
}
*/

BOOST_AUTO_TEST_CASE( process_recent_claims_decay )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: recent_rshares_2decay" );
      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 10000 );
      vest( "bob", 10000 );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );
      generate_block();

      comment_operation comment;
      vote_operation vote;
      signed_transaction tx;

      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "foo";
      comment.body = "bar";
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = ZATTERA_100_PERCENT;
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto alice_vshares = util::evaluate_reward_curve( db->get_comment( "alice", string( "test" ) ).net_rshares.value,
         db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME ).author_reward_curve,
         db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME ).content_constant );

      generate_blocks( 5 );

      comment.author = "bob";
      vote.voter = "bob";
      vote.author = "bob";
      tx.clear();
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time );

      {
         const auto& post_rf = db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.recent_claims == alice_vshares );
         validate_database();
      }

      auto bob_cashout_time = db->get_comment( "bob", string( "test" ) ).cashout_time;
      auto bob_vshares = util::evaluate_reward_curve( db->get_comment( "bob", string( "test" ) ).net_rshares.value,
         db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME ).author_reward_curve,
         db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME ).content_constant );

      generate_block();

      while( db->head_block_time() < bob_cashout_time )
      {
         alice_vshares -= ( alice_vshares * ZATTERA_BLOCK_INTERVAL ) / ZATTERA_RECENT_RSHARES_DECAY_TIME.to_seconds();
         const auto& post_rf = db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.recent_claims == alice_vshares );

         generate_block();

      }

      {
         alice_vshares -= ( alice_vshares * ZATTERA_BLOCK_INTERVAL ) / ZATTERA_RECENT_RSHARES_DECAY_TIME.to_seconds();
         const auto& post_rf = db->get< reward_fund_object, by_name >( ZATTERA_POST_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.recent_claims == alice_vshares + bob_vshares );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

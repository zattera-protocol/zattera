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

BOOST_AUTO_TEST_CASE( validate_transfer_to_vesting )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_transfer_to_vesting_authorities )
{
   try
   {
      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_authorities" );

      transfer_to_vesting_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "2.500 TTR" );

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
      tx.sign( alice_post_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with from signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_transfer_to_vesting )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_vesting_apply" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      const auto& gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( alice.liquid_balance == ASSET( "10.000 TTR" ) );

      auto shares = asset( gpo.total_vesting_shares.amount, VESTS_SYMBOL );
      auto vests = asset( gpo.total_vesting_fund_liquid.amount, LIQUID_SYMBOL );
      auto alice_shares = alice.vesting_share_balance;
      auto bob_shares = bob.vesting_share_balance;

      transfer_to_vesting_operation op;
      op.from = "alice";
      op.to = "";
      op.amount = ASSET( "7.500 TTR" );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto new_vest = op.amount * ( shares / vests );
      shares += new_vest;
      vests += op.amount;
      alice_shares += new_vest;

      BOOST_REQUIRE( alice.liquid_balance.amount.value == ASSET( "2.500 TTR" ).amount.value );
      BOOST_REQUIRE( alice.vesting_share_balance.amount.value == alice_shares.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_fund_liquid.amount.value == vests.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_shares.amount.value == shares.amount.value );
      validate_database();

      op.to = "bob";
      op.amount = asset( 2000, LIQUID_SYMBOL );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      new_vest = asset( ( op.amount * ( shares / vests ) ).amount, VESTS_SYMBOL );
      shares += new_vest;
      vests += op.amount;
      bob_shares += new_vest;

      BOOST_REQUIRE( alice.liquid_balance.amount.value == ASSET( "0.500 TTR" ).amount.value );
      BOOST_REQUIRE( alice.vesting_share_balance.amount.value == alice_shares.amount.value );
      BOOST_REQUIRE( bob.liquid_balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.vesting_share_balance.amount.value == bob_shares.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_fund_liquid.amount.value == vests.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_shares.amount.value == shares.amount.value );
      validate_database();

      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( alice.liquid_balance.amount.value == ASSET( "0.500 TTR" ).amount.value );
      BOOST_REQUIRE( alice.vesting_share_balance.amount.value == alice_shares.amount.value );
      BOOST_REQUIRE( bob.liquid_balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.vesting_share_balance.amount.value == bob_shares.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_fund_liquid.amount.value == vests.amount.value );
      BOOST_REQUIRE( gpo.total_vesting_shares.amount.value == shares.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_vesting_withdrawal )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_vesting_withdrawal_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );

      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = ASSET( "0.001000 VESTS" );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with account signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_vesting_withdrawal )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withdraw_vesting_apply" );

      ACTORS( (alice)(bob) )
      generate_block();
      vest( "alice", ASSET( "10.000 TTR" ) );

      BOOST_TEST_MESSAGE( "--- Test failure withdrawing negative VESTS" );

      {
      const auto& alice = db->get_account( "alice" );

      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( -1, VESTS_SYMBOL );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test withdraw of existing VESTS" );
      op.vesting_shares = asset( alice.vesting_share_balance.amount / 2, VESTS_SYMBOL );

      auto old_vesting_shares = alice.vesting_share_balance;

      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_share_balance.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( ZATTERA_VESTING_WITHDRAW_INTERVALS * 2 ) ).value );
      BOOST_REQUIRE( alice.to_withdraw.value == op.vesting_shares.amount.value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test changing vesting withdrawal" );
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( alice.vesting_share_balance.amount / 3, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_share_balance.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( ZATTERA_VESTING_WITHDRAW_INTERVALS * 3 ) ).value );
      BOOST_REQUIRE( alice.to_withdraw.value == op.vesting_shares.amount.value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing more vests than available" );
      //auto old_withdraw_amount = alice.to_withdraw;
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( alice.vesting_share_balance.amount * 2, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( alice.vesting_share_balance.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == ( old_vesting_shares.amount / ( ZATTERA_VESTING_WITHDRAW_INTERVALS * 3 ) ).value );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing 0 to reset vesting withdraw" );
      tx.operations.clear();
      tx.signatures.clear();

      op.vesting_shares = asset( 0, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.vesting_share_balance.amount.value == old_vesting_shares.amount.value );
      BOOST_REQUIRE( alice.vesting_withdraw_rate.amount.value == 0 );
      BOOST_REQUIRE( alice.to_withdraw.value == 0 );
      BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );


      BOOST_TEST_MESSAGE( "--- Test cancelling a withdraw when below the account creation fee" );
      op.vesting_shares = alice.vesting_share_balance;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();
      }

      db_plugin->debug_update( [=]( database& db )
      {
         auto& wso = db.get_witness_schedule_object();

         db.modify( wso, [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "10.000 TTR" );
         });

         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_liquid_supply += wso.median_props.account_creation_fee - ASSET( "0.001 TTR" ) - gpo.total_vesting_fund_liquid;
            gpo.total_vesting_fund_liquid = wso.median_props.account_creation_fee - ASSET( "0.001 TTR" );
         });

         db.update_virtual_supply();
      }, database::skip_witness_signature );

      withdraw_vesting_operation op;
      signed_transaction tx;
      op.account = "alice";
      op.vesting_shares = ASSET( "0.000000 VESTS" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_withdraw_rate == ASSET( "0.000000 VESTS" ) );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test withdrawing minimal VESTS" );
      op.account = "bob";
      op.vesting_shares = db->get_account( "bob" ).vesting_share_balance;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 ); // We do not need to test the result of this, simply that it works.
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( process_vesting_withdrawals )
{
   try
   {
      ACTORS( (alice) )
      fund( "alice", 100000 );
      vest( "alice", 100000 );

      const auto& new_alice = db->get_account( "alice" );

      BOOST_TEST_MESSAGE( "Setting up withdrawal" );

      signed_transaction tx;
      withdraw_vesting_operation op;
      op.account = "alice";
      op.vesting_shares = asset( new_alice.vesting_share_balance.amount / 2, VESTS_SYMBOL );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto next_withdrawal = db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS;
      asset vesting_shares = new_alice.vesting_share_balance;
      asset to_withdraw = op.vesting_shares;
      asset original_vesting_shares = vesting_shares;
      asset withdraw_rate = new_alice.vesting_withdraw_rate;

      BOOST_TEST_MESSAGE( "Generating block up to first withdrawal" );
      generate_blocks( next_withdrawal - ( ZATTERA_BLOCK_INTERVAL / 2 ), true);

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_share_balance.amount.value == vesting_shares.amount.value );

      BOOST_TEST_MESSAGE( "Generating block to cause withdrawal" );
      generate_block();

      auto fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
      auto gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_share_balance.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
      BOOST_REQUIRE( ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - db->get_account( "alice" ).liquid_balance.amount.value <= 1 ); // Check a range due to differences in the share price
      BOOST_REQUIRE( fill_op.from_account == "alice" );
      BOOST_REQUIRE( fill_op.to_account == "alice" );
      BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
      BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      validate_database();

      BOOST_TEST_MESSAGE( "Generating the rest of the blocks in the withdrawal" );

      vesting_shares = db->get_account( "alice" ).vesting_share_balance;
      auto liquid_balance = db->get_account( "alice" ).liquid_balance;
      auto old_next_vesting_withdrawal = db->get_account( "alice" ).next_vesting_withdrawal;

      for( int i = 1; i < ZATTERA_VESTING_WITHDRAW_INTERVALS - 1; i++ )
      {
         generate_blocks( db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS );

         const auto& alice = db->get_account( "alice" );

         gpo = db->get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE( alice.vesting_share_balance.amount.value == ( vesting_shares - withdraw_rate ).amount.value );
         BOOST_REQUIRE( liquid_balance.amount.value + ( withdraw_rate * gpo.get_vesting_share_price() ).amount.value - alice.liquid_balance.amount.value <= 1 );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         if ( i == ZATTERA_VESTING_WITHDRAW_INTERVALS - 1 )
            BOOST_REQUIRE( alice.next_vesting_withdrawal == fc::time_point_sec::maximum() );
         else
            BOOST_REQUIRE( alice.next_vesting_withdrawal.sec_since_epoch() == ( old_next_vesting_withdrawal + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );

         validate_database();

         vesting_shares = alice.vesting_share_balance;
         liquid_balance = alice.liquid_balance;
         old_next_vesting_withdrawal = alice.next_vesting_withdrawal;
      }

      if (  to_withdraw.amount.value % withdraw_rate.amount.value != 0 )
      {
         BOOST_TEST_MESSAGE( "Generating one more block to take care of remainder" );
         generate_blocks( db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         gpo = db->get_dynamic_global_properties();

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == ( old_next_vesting_withdrawal + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS ).sec_since_epoch() );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         generate_blocks( db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS, true );
         gpo = db->get_dynamic_global_properties();
         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == fc::time_point_sec::maximum().sec_since_epoch() );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == to_withdraw.amount.value % withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );

         validate_database();
      }
      else
      {
         generate_blocks( db->head_block_time() + ZATTERA_VESTING_WITHDRAW_INTERVAL_SECONDS, true );

         BOOST_REQUIRE( db->get_account( "alice" ).next_vesting_withdrawal.sec_since_epoch() == fc::time_point_sec::maximum().sec_since_epoch() );

         fill_op = get_last_operations( 1 )[0].get< fill_vesting_withdraw_operation >();
         BOOST_REQUIRE( fill_op.from_account == "alice" );
         BOOST_REQUIRE( fill_op.to_account == "alice" );
         BOOST_REQUIRE( fill_op.withdrawn.amount.value == withdraw_rate.amount.value );
         BOOST_REQUIRE( std::abs( ( fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price() ).amount.value ) <= 1 );
      }

      BOOST_REQUIRE( db->get_account( "alice" ).vesting_share_balance.amount.value == ( original_vesting_shares - op.vesting_shares ).amount.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( set_vesting_withdraw_route )
{
   try
   {
      ACTORS( (alice)(bob)(sam) )

      auto original_vesting_shares = alice.vesting_share_balance;

      fund( "alice", 1040000 );
      vest( "alice", 1040000 );

      auto withdraw_amount = alice.vesting_share_balance - original_vesting_shares;

      BOOST_TEST_MESSAGE( "Setup vesting withdraw" );
      withdraw_vesting_operation wv;
      wv.account = "alice";
      wv.vesting_shares = withdraw_amount;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( wv );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Setting up bob destination" );
      set_withdraw_vesting_route_operation op;
      op.from_account = "alice";
      op.to_account = "bob";
      op.percent = ZATTERA_1_PERCENT * 50;
      op.auto_vest = true;
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "Setting up sam destination" );
      op.to_account = "sam";
      op.percent = ZATTERA_1_PERCENT * 30;
      op.auto_vest = false;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Setting up first withdraw" );

      auto vesting_withdraw_rate = alice.vesting_withdraw_rate;
      auto old_alice_liquid_balance = alice.liquid_balance;
      auto old_alice_vests_balance = alice.vesting_share_balance;
      auto old_bob_liquid_balance = bob.liquid_balance;
      auto old_bob_vests_balance = bob.vesting_share_balance;
      auto old_sam_liquid_balance = sam.liquid_balance;
      auto old_sam_vests_balance = sam.vesting_share_balance;
      generate_blocks( alice.next_vesting_withdrawal, true );

      {
         const auto& alice = db->get_account( "alice" );
         const auto& bob = db->get_account( "bob" );
         const auto& sam = db->get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_share_balance == old_alice_vests_balance - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.liquid_balance == old_alice_liquid_balance + asset( ( vesting_withdraw_rate.amount * ZATTERA_1_PERCENT * 20 ) / ZATTERA_100_PERCENT, VESTS_SYMBOL ) * db->get_dynamic_global_properties().get_vesting_share_price() );
         BOOST_REQUIRE( bob.vesting_share_balance == old_bob_vests_balance + asset( ( vesting_withdraw_rate.amount * ZATTERA_1_PERCENT * 50 ) / ZATTERA_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.liquid_balance == old_bob_liquid_balance );
         BOOST_REQUIRE( sam.vesting_share_balance == old_sam_vests_balance );
         BOOST_REQUIRE( sam.liquid_balance ==  old_sam_liquid_balance + asset( ( vesting_withdraw_rate.amount * ZATTERA_1_PERCENT * 30 ) / ZATTERA_100_PERCENT, VESTS_SYMBOL ) * db->get_dynamic_global_properties().get_vesting_share_price() );

         old_alice_liquid_balance = alice.liquid_balance;
         old_alice_vests_balance = alice.vesting_share_balance;
         old_bob_liquid_balance = bob.liquid_balance;
         old_bob_vests_balance = bob.vesting_share_balance;
         old_sam_liquid_balance = sam.liquid_balance;
         old_sam_vests_balance = sam.vesting_share_balance;
      }

      BOOST_TEST_MESSAGE( "Test failure with greater than 100% destination assignment" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = ZATTERA_1_PERCENT * 50 + 1;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "Test from_account receiving no withdraw" );

      tx.operations.clear();
      tx.signatures.clear();

      op.to_account = "sam";
      op.percent = ZATTERA_1_PERCENT * 50;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_account( "alice" ).next_vesting_withdrawal, true );
      {
         const auto& alice = db->get_account( "alice" );
         const auto& bob = db->get_account( "bob" );
         const auto& sam = db->get_account( "sam" );

         BOOST_REQUIRE( alice.vesting_share_balance == old_alice_vests_balance - vesting_withdraw_rate );
         BOOST_REQUIRE( alice.liquid_balance == old_alice_liquid_balance );
         BOOST_REQUIRE( bob.vesting_share_balance == old_bob_vests_balance + asset( ( vesting_withdraw_rate.amount * ZATTERA_1_PERCENT * 50 ) / ZATTERA_100_PERCENT, VESTS_SYMBOL ) );
         BOOST_REQUIRE( bob.liquid_balance == old_bob_liquid_balance );
         BOOST_REQUIRE( sam.vesting_share_balance == old_sam_vests_balance );
         BOOST_REQUIRE( sam.liquid_balance ==  old_sam_liquid_balance + asset( ( vesting_withdraw_rate.amount * ZATTERA_1_PERCENT * 50 ) / ZATTERA_100_PERCENT, VESTS_SYMBOL ) * db->get_dynamic_global_properties().get_vesting_share_price() );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_vesting_shares_delegation )
{
   try
   {
      delegate_vesting_shares_operation op;

      op.delegator = "alice";
      op.delegatee = "bob";
      op.vesting_shares = asset( -1, VESTS_SYMBOL );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_vesting_shares_delegation_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: delegate_vesting_shares_authorities" );
      signed_transaction tx;
      ACTORS( (alice)(bob) )
      fund( "alice", 500000 );
      vest( "alice", 500000 );

      delegate_vesting_shares_operation op;
      op.vesting_shares = ASSET( "300.000000 VESTS");
      op.delegator = "alice";
      op.delegatee = "bob";

      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.operations.clear();
      tx.signatures.clear();
      op.delegatee = "sam";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( init_account_priv_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( init_account_priv_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_vesting_shares_delegation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: delegate_vesting_shares_apply" );
      signed_transaction tx;
      ACTORS( (alice)(bob) )
      generate_block();

      fund( "alice", ASSET( "40000000.000 TTR" ) );
      vest( "alice", ASSET( "40000000.000 TTR" ) );

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "1.000 TTR" );
         });
      });

      generate_block();

      delegate_vesting_shares_operation op;
      op.vesting_shares = ASSET( "10000000.000000 VESTS");
      op.delegator = "alice";
      op.delegatee = "bob";

      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_blocks( 1 );

      BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_share_balance == ASSET( "10000000.000000 VESTS"));
      BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_share_balance == ASSET( "10000000.000000 VESTS"));

      BOOST_TEST_MESSAGE( "--- Test that the delegation object is correct. " );
      auto delegation = db->find< vesting_delegation_object, by_delegation >( boost::make_tuple( op.delegator, op.delegatee ) );

      BOOST_REQUIRE( delegation != nullptr );
      BOOST_REQUIRE( delegation->delegator == op.delegator);
      BOOST_REQUIRE( delegation->vesting_shares == ASSET( "10000000.000000 VESTS"));

      validate_database();
      tx.clear();
      op.vesting_shares = ASSET( "20000000.000000 VESTS");
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_blocks(1);

      BOOST_REQUIRE( delegation != nullptr );
      BOOST_REQUIRE( delegation->delegator == op.delegator);
      BOOST_REQUIRE( delegation->vesting_shares == ASSET( "20000000.000000 VESTS"));
      BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_share_balance == ASSET( "20000000.000000 VESTS"));
      BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_share_balance == ASSET( "20000000.000000 VESTS"));

      BOOST_TEST_MESSAGE( "--- Test that effective vesting shares is accurate and being applied." );
      tx.operations.clear();
      tx.signatures.clear();

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
      tx.signatures.clear();
      tx.operations.clear();
      vote_operation vote_op;
      vote_op.voter = "bob";
      vote_op.author = "alice";
      vote_op.permlink = "foo";
      vote_op.weight = ZATTERA_100_PERCENT;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( vote_op );
      tx.sign( bob_private_key, db->get_chain_id() );
      auto old_voting_power = db->get_account( "bob" ).voting_power;

      db->push_transaction( tx, 0 );
      generate_blocks(1);

      const auto& vote_idx = db->get_index< comment_vote_index >().indices().get< by_comment_voter >();

      auto& alice_comment = db->get_comment( "alice", string( "foo" ) );
      const auto& bob_after_vote = db->get_account( "bob" );
      auto itr = vote_idx.find( std::make_tuple( alice_comment.id, bob_after_vote.id ) );
      BOOST_REQUIRE( alice_comment.net_rshares.value == db->get_effective_vesting_shares(bob_after_vote, VESTS_SYMBOL).amount.value * ( old_voting_power - bob_after_vote.voting_power ) / ZATTERA_100_PERCENT - ZATTERA_VOTE_DUST_THRESHOLD);
      BOOST_REQUIRE( itr->rshares == db->get_effective_vesting_shares(bob_after_vote, VESTS_SYMBOL).amount.value * ( old_voting_power - bob_after_vote.voting_power ) / ZATTERA_100_PERCENT - ZATTERA_VOTE_DUST_THRESHOLD );


      generate_block();
      ACTORS( (sam)(dave) )
      generate_block();

      vest( "sam", ASSET( "1000.000 TTR" ) );

      generate_block();

      auto sam_vests_balance = db->get_account( "sam" ).vesting_share_balance;

      BOOST_TEST_MESSAGE( "--- Test failure when delegating 0 VESTS" );
      tx.clear();
      op.delegator = "sam";
      op.delegatee = "dave";
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Testing failure delegating more vesting shares than account has." );
      tx.clear();
      op.vesting_shares = asset( sam_vests_balance.amount + 1, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Test failure delegating vesting shares that are part of a power down" );
      tx.clear();
      sam_vests_balance = asset( sam_vests_balance.amount / 2, VESTS_SYMBOL );
      withdraw_vesting_operation withdraw;
      withdraw.account = "sam";
      withdraw.vesting_shares = sam_vests_balance;
      tx.operations.push_back( withdraw );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.clear();
      op.vesting_shares = asset( sam_vests_balance.amount + 2, VESTS_SYMBOL );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );

      tx.clear();
      withdraw.vesting_shares = ASSET( "0.000000 VESTS" );
      tx.operations.push_back( withdraw );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- Test failure powering down vesting shares that are delegated" );
      sam_vests_balance.amount += 1000;
      op.vesting_shares = sam_vests_balance;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.clear();
      withdraw.vesting_shares = asset( sam_vests_balance.amount, VESTS_SYMBOL );
      tx.operations.push_back( withdraw );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx ), fc::assert_exception );


      BOOST_TEST_MESSAGE( "--- Remove a delegation and ensure it is returned after 1 week" );
      tx.clear();
      op.vesting_shares = ASSET( "0.000000 VESTS" );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto exp_obj = db->get_index< vesting_delegation_expiration_index, by_id >().begin();
      auto end = db->get_index< vesting_delegation_expiration_index, by_id >().end();
      auto gpo = db->get_dynamic_global_properties();

      BOOST_REQUIRE( gpo.delegation_return_period == ZATTERA_DELEGATION_RETURN_PERIOD );

      BOOST_REQUIRE( exp_obj != end );
      BOOST_REQUIRE( exp_obj->delegator == "sam" );
      BOOST_REQUIRE( exp_obj->vesting_shares == sam_vests_balance );
      BOOST_REQUIRE( exp_obj->expiration == db->head_block_time() + gpo.delegation_return_period );
      BOOST_REQUIRE( db->get_account( "sam" ).delegated_vesting_share_balance == sam_vests_balance );
      BOOST_REQUIRE( db->get_account( "dave" ).received_vesting_share_balance == ASSET( "0.000000 VESTS" ) );
      delegation = db->find< vesting_delegation_object, by_delegation >( boost::make_tuple( op.delegator, op.delegatee ) );
      BOOST_REQUIRE( delegation == nullptr );

      generate_blocks( exp_obj->expiration + ZATTERA_BLOCK_INTERVAL );

      exp_obj = db->get_index< vesting_delegation_expiration_index, by_id >().begin();
      end = db->get_index< vesting_delegation_expiration_index, by_id >().end();

      BOOST_REQUIRE( exp_obj == end );
      BOOST_REQUIRE( db->get_account( "sam" ).delegated_vesting_share_balance == ASSET( "0.000000 VESTS" ) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( fix_issue_971_vesting_removal )
{
   // This is a regression test specifically for issue #971
   try
   {
      BOOST_TEST_MESSAGE( "Test Issue 971 Vesting Removal" );
      ACTORS( (alice)(bob) )
      generate_block();

      fund( "alice", ASSET( "20000000.000 TTR" ) );
      vest( "alice", ASSET( "20000000.000 TTR" ) );

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "1.000 TTR" );
         });
      });

      generate_block();

      signed_transaction tx;
      delegate_vesting_shares_operation op;
      op.vesting_shares = ASSET( "10000000.000000 VESTS");
      op.delegator = "alice";
      op.delegatee = "bob";

      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();

      BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_share_balance == ASSET( "10000000.000000 VESTS"));
      BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_share_balance == ASSET( "10000000.000000 VESTS"));

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "100.000 TTR" );
         });
      });

      generate_block();

      op.vesting_shares = ASSET( "0.000000 VESTS" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();

      BOOST_REQUIRE( db->get_account( "alice" ).delegated_vesting_share_balance == ASSET( "10000000.000000 VESTS"));
      BOOST_REQUIRE( db->get_account( "bob" ).received_vesting_share_balance == ASSET( "0.000000 VESTS"));
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

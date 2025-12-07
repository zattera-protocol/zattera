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

BOOST_AUTO_TEST_CASE( validate_transfer )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_transfer_authorities )
{
   try
   {
      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      BOOST_TEST_MESSAGE( "Testing: transfer_authorities" );

      transfer_operation op;
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

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( strip_signatures )
{
   try
   {
      // Alice, Bob and Sam all have 2-of-3 multisig on corp.
      // Legitimate tx signed by (Alice, Bob) goes through.
      // Sam shouldn't be able to add or remove signatures to get the transaction to process multiple times.

      ACTORS( (alice)(bob)(sam)(corp) )
      fund( "corp", 10000 );

      account_update_operation update_op;
      update_op.account = "corp";
      update_op.active = authority( 2, "alice", 1, "bob", 1, "sam", 1 );

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( update_op );

      tx.sign( corp_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      transfer_operation transfer_op;
      transfer_op.from = "corp";
      transfer_op.to = "sam";
      transfer_op.amount = ASSET( "1.000 TTR" );

      tx.operations.push_back( transfer_op );

      tx.sign( alice_private_key, db->get_chain_id() );
      signature_type alice_sig = tx.signatures.back();
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );
      tx.sign( bob_private_key, db->get_chain_id() );
      signature_type bob_sig = tx.signatures.back();
      tx.sign( sam_private_key, db->get_chain_id() );
      signature_type sam_sig = tx.signatures.back();
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      tx.signatures.clear();
      tx.signatures.push_back( alice_sig );
      tx.signatures.push_back( bob_sig );
      db->push_transaction( tx, 0 );

      tx.signatures.clear();
      tx.signatures.push_back( alice_sig );
      tx.signatures.push_back( sam_sig );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_transfer )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_apply" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "10.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET(" 0.000 TTR" ).amount.value );

      signed_transaction tx;
      transfer_operation op;

      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "5.000 TTR" );

      BOOST_TEST_MESSAGE( "--- Test normal transaction" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "5.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "5.000 TTR" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Generating a block" );
      generate_block();

      const auto& new_alice = db->get_account( "alice" );
      const auto& new_bob = db->get_account( "bob" );

      BOOST_REQUIRE( new_alice.balance.amount.value == ASSET( "5.000 TTR" ).amount.value );
      BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "5.000 TTR" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test emptying an account" );
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_REQUIRE( new_alice.balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "10.000 TTR" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test transferring non-existent funds" );
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( new_alice.balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "10.000 TTR" ).amount.value );
      validate_database();

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_escrow_transfer )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_transfer_validate" );

      escrow_transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.zbd_amount = ASSET( "1.000 TBD" );
      op.ztr_amount = ASSET( "1.000 TTR" );
      op.escrow_id = 0;
      op.agent = "sam";
      op.fee = ASSET( "0.100 TTR" );
      op.json_meta = "";
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;

      BOOST_TEST_MESSAGE( "--- failure when zbd symbol != ZBD" );
      op.zbd_amount.symbol = ZTR_SYMBOL;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ztr symbol != ZTR" );
      op.zbd_amount.symbol = ZBD_SYMBOL;
      op.ztr_amount.symbol = ZBD_SYMBOL;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when fee symbol != ZBD and fee symbol != ZTR" );
      op.ztr_amount.symbol = ZTR_SYMBOL;
      op.fee.symbol = VESTS_SYMBOL;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when zbd == 0 and ztr == 0" );
      op.fee.symbol = ZTR_SYMBOL;
      op.zbd_amount.amount = 0;
      op.ztr_amount.amount = 0;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when zbd < 0" );
      op.zbd_amount.amount = -100;
      op.ztr_amount.amount = 1000;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ztr < 0" );
      op.zbd_amount.amount = 1000;
      op.ztr_amount.amount = -100;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when fee < 0" );
      op.ztr_amount.amount = 1000;
      op.fee.amount = -100;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ratification deadline == escrow expiration" );
      op.fee.amount = 100;
      op.ratification_deadline = op.escrow_expiration;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ratification deadline > escrow expiration" );
      op.ratification_deadline = op.escrow_expiration + 100;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- success" );
      op.ratification_deadline = op.escrow_expiration - 100;
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_escrow_transfer_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_transfer_authorities" );

      escrow_transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.zbd_amount = ASSET( "1.000 TBD" );
      op.ztr_amount = ASSET( "1.000 TTR" );
      op.escrow_id = 0;
      op.agent = "sam";
      op.fee = ASSET( "0.100 TTR" );
      op.json_meta = "";
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "alice" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_escrow_transfer )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_transfer_apply" );

      ACTORS( (alice)(bob)(sam) )

      fund( "alice", 10000 );

      escrow_transfer_operation op;
      op.from = "alice";
      op.to = "bob";
      op.zbd_amount = ASSET( "1.000 TBD" );
      op.ztr_amount = ASSET( "1.000 TTR" );
      op.escrow_id = 0;
      op.agent = "sam";
      op.fee = ASSET( "0.100 TTR" );
      op.json_meta = "";
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;

      BOOST_TEST_MESSAGE( "--- failure when from cannot cover zbd amount" );
      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- falure when from cannot cover amount + fee" );
      op.zbd_amount.amount = 0;
      op.ztr_amount.amount = 10000;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when ratification deadline is in the past" );
      op.ztr_amount.amount = 1000;
      op.ratification_deadline = db->head_block_time() - 200;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- failure when expiration is in the past" );
      op.escrow_expiration = db->head_block_time() - 100;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- success" );
      op.ratification_deadline = db->head_block_time() + 100;
      op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      auto alice_ztr_balance = alice.balance - op.ztr_amount - op.fee;
      auto alice_zbd_balance = alice.zbd_balance - op.zbd_amount;
      auto bob_ztr_balance = bob.balance;
      auto bob_zbd_balance = bob.zbd_balance;
      auto sam_ztr_balance = sam.balance;
      auto sam_zbd_balance = sam.zbd_balance;

      db->push_transaction( tx, 0 );

      const auto& escrow = db->get_escrow( op.from, op.escrow_id );

      BOOST_REQUIRE( escrow.escrow_id == op.escrow_id );
      BOOST_REQUIRE( escrow.from == op.from );
      BOOST_REQUIRE( escrow.to == op.to );
      BOOST_REQUIRE( escrow.agent == op.agent );
      BOOST_REQUIRE( escrow.ratification_deadline == op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == op.escrow_expiration );
      BOOST_REQUIRE( escrow.zbd_balance == op.zbd_amount );
      BOOST_REQUIRE( escrow.ztr_balance == op.ztr_amount );
      BOOST_REQUIRE( escrow.pending_fee == op.fee );
      BOOST_REQUIRE( !escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );
      BOOST_REQUIRE( alice.balance == alice_ztr_balance );
      BOOST_REQUIRE( alice.zbd_balance == alice_zbd_balance );
      BOOST_REQUIRE( bob.balance == bob_ztr_balance );
      BOOST_REQUIRE( bob.zbd_balance == bob_zbd_balance );
      BOOST_REQUIRE( sam.balance == sam_ztr_balance );
      BOOST_REQUIRE( sam.zbd_balance == sam_zbd_balance );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_escrow_approve )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_approve_validate" );

      escrow_approve_operation op;

      op.from = "alice";
      op.to = "bob";
      op.agent = "sam";
      op.who = "bob";
      op.escrow_id = 0;
      op.approve = true;

      BOOST_TEST_MESSAGE( "--- failure when who is not to or agent" );
      op.who = "dave";
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- success when who is to" );
      op.who = op.to;
      op.validate();

      BOOST_TEST_MESSAGE( "--- success when who is agent" );
      op.who = op.agent;
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_escrow_approve_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_approve_authorities" );

      escrow_approve_operation op;

      op.from = "alice";
      op.to = "bob";
      op.agent = "sam";
      op.who = "bob";
      op.escrow_id = 0;
      op.approve = true;

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "bob" );
      BOOST_REQUIRE( auths == expected );

      expected.clear();
      auths.clear();

      op.who = "sam";
      op.get_required_active_authorities( auths );
      expected.insert( "sam" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_escrow_approve )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_approve_apply" );
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );

      escrow_transfer_operation et_op;
      et_op.from = "alice";
      et_op.to = "bob";
      et_op.agent = "sam";
      et_op.ztr_amount = ASSET( "1.000 TTR" );
      et_op.fee = ASSET( "0.100 TTR" );
      et_op.json_meta = "";
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;

      signed_transaction tx;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      tx.operations.clear();
      tx.signatures.clear();


      BOOST_TEST_MESSAGE( "---failure when to does not match escrow" );
      escrow_approve_operation op;
      op.from = "alice";
      op.to = "dave";
      op.agent = "sam";
      op.who = "dave";
      op.approve = true;

      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when agent does not match escrow" );
      op.to = "bob";
      op.agent = "dave";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success approving to" );
      op.agent = "sam";
      op.who = "bob";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto& escrow = db->get_escrow( op.from, op.escrow_id );
      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( escrow.ztr_balance == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.100 TTR" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure on repeat approval" );
      tx.signatures.clear();

      tx.set_expiration( db->head_block_time() + ZATTERA_BLOCK_INTERVAL );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( escrow.ztr_balance == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.100 TTR" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure trying to repeal after approval" );
      tx.signatures.clear();
      tx.operations.clear();

      op.approve = false;

      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( escrow.ztr_balance == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.100 TTR" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- success refunding from because of repeal" );
      tx.signatures.clear();
      tx.operations.clear();

      op.who = op.agent;

      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      ZATTERA_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( alice.balance == ASSET( "10.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- test automatic refund when escrow is not ratified before deadline" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( et_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( et_op.ratification_deadline + ZATTERA_BLOCK_INTERVAL, true );

      ZATTERA_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "10.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- test ratification expiration when escrow is only approved by to" );
      tx.operations.clear();
      tx.signatures.clear();
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.to;
      op.approve = true;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( et_op.ratification_deadline + ZATTERA_BLOCK_INTERVAL, true );

      ZATTERA_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "10.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- test ratification expiration when escrow is only approved by agent" );
      tx.operations.clear();
      tx.signatures.clear();
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( et_op.ratification_deadline + ZATTERA_BLOCK_INTERVAL, true );

      ZATTERA_REQUIRE_THROW( db->get_escrow( op.from, op.escrow_id ), fc::exception );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "10.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success approving escrow" );
      tx.operations.clear();
      tx.signatures.clear();
      et_op.ratification_deadline = db->head_block_time() + 100;
      et_op.escrow_expiration = db->head_block_time() + 200;
      tx.operations.push_back( et_op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.to;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.who = op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      {
         const auto& escrow = db->get_escrow( op.from, op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.zbd_balance == ASSET( "0.000 TBD" ) );
         BOOST_REQUIRE( escrow.ztr_balance == ASSET( "1.000 TTR" ) );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000 TTR" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( !escrow.disputed );
      }

      BOOST_REQUIRE( db->get_account( "sam" ).balance == et_op.fee );
      validate_database();


      BOOST_TEST_MESSAGE( "--- ratification expiration does not remove an approved escrow" );

      generate_blocks( et_op.ratification_deadline + ZATTERA_BLOCK_INTERVAL, true );
      {
         const auto& escrow = db->get_escrow( op.from, op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.zbd_balance == ASSET( "0.000 TBD" ) );
         BOOST_REQUIRE( escrow.ztr_balance == ASSET( "1.000 TTR" ) );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000 TTR" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( !escrow.disputed );
      }

      BOOST_REQUIRE( db->get_account( "sam" ).balance == et_op.fee );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_escrow_dispute )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_dispute_validate" );
      escrow_dispute_operation op;
      op.from = "alice";
      op.to = "bob";
      op.agent = "alice";
      op.who = "alice";

      BOOST_TEST_MESSAGE( "failure when who is not from or to" );
      op.who = "sam";
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "success" );
      op.who = "alice";
      op.validate();

      op.who = "bob";
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_escrow_dispute_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_dispute_authorities" );
      escrow_dispute_operation op;
      op.from = "alice";
      op.to = "bob";
      op.who = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "alice" );
      BOOST_REQUIRE( auths == expected );

      auths.clear();
      expected.clear();
      op.who = "bob";
      op.get_required_active_authorities( auths );
      expected.insert( "bob" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_escrow_dispute )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_dispute_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );

      escrow_transfer_operation et_op;
      et_op.from = "alice";
      et_op.to = "bob";
      et_op.agent = "sam";
      et_op.ztr_amount = ASSET( "1.000 TTR" );
      et_op.fee = ASSET( "0.100 TTR" );
      et_op.ratification_deadline = db->head_block_time() + ZATTERA_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * ZATTERA_BLOCK_INTERVAL;

      escrow_approve_operation ea_b_op;
      ea_b_op.from = "alice";
      ea_b_op.to = "bob";
      ea_b_op.agent = "sam";
      ea_b_op.who = "bob";
      ea_b_op.approve = true;

      signed_transaction tx;
      tx.operations.push_back( et_op );
      tx.operations.push_back( ea_b_op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- failure when escrow has not been approved" );
      escrow_dispute_operation op;
      op.from = "alice";
      op.to = "bob";
      op.agent = "sam";
      op.who = "bob";

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.zbd_balance == et_op.zbd_amount );
      BOOST_REQUIRE( escrow.ztr_balance == et_op.ztr_amount );
      BOOST_REQUIRE( escrow.pending_fee == et_op.fee );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( !escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure when to does not match escrow" );
      escrow_approve_operation ea_s_op;
      ea_s_op.from = "alice";
      ea_s_op.to = "bob";
      ea_s_op.agent = "sam";
      ea_s_op.who = "sam";
      ea_s_op.approve = true;

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( ea_s_op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.to = "dave";
      op.who = "alice";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.zbd_balance == et_op.zbd_amount );
      BOOST_REQUIRE( escrow.ztr_balance == et_op.ztr_amount );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure when agent does not match escrow" );
      op.to = "bob";
      op.who = "alice";
      op.agent = "dave";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( escrow.to == "bob" );
      BOOST_REQUIRE( escrow.agent == "sam" );
      BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
      BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
      BOOST_REQUIRE( escrow.zbd_balance == et_op.zbd_amount );
      BOOST_REQUIRE( escrow.ztr_balance == et_op.ztr_amount );
      BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( escrow.to_approved );
      BOOST_REQUIRE( escrow.agent_approved );
      BOOST_REQUIRE( !escrow.disputed );


      BOOST_TEST_MESSAGE( "--- failure when escrow is expired" );
      generate_blocks( 2 );

      tx.operations.clear();
      tx.signatures.clear();
      op.agent = "sam";
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      {
         const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.zbd_balance == et_op.zbd_amount );
         BOOST_REQUIRE( escrow.ztr_balance == et_op.ztr_amount );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000 TTR" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( !escrow.disputed );
      }


      BOOST_TEST_MESSAGE( "--- success disputing escrow" );
      et_op.escrow_id = 1;
      et_op.ratification_deadline = db->head_block_time() + ZATTERA_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * ZATTERA_BLOCK_INTERVAL;
      ea_b_op.escrow_id = et_op.escrow_id;
      ea_s_op.escrow_id = et_op.escrow_id;

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( et_op );
      tx.operations.push_back( ea_b_op );
      tx.operations.push_back( ea_s_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.escrow_id = et_op.escrow_id;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      {
         const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.zbd_balance == et_op.zbd_amount );
         BOOST_REQUIRE( escrow.ztr_balance == et_op.ztr_amount );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000 TTR" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( escrow.disputed );
      }


      BOOST_TEST_MESSAGE( "--- failure when escrow is already under dispute" );
      tx.operations.clear();
      tx.signatures.clear();
      op.who = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      {
         const auto& escrow = db->get_escrow( et_op.from, et_op.escrow_id );
         BOOST_REQUIRE( escrow.to == "bob" );
         BOOST_REQUIRE( escrow.agent == "sam" );
         BOOST_REQUIRE( escrow.ratification_deadline == et_op.ratification_deadline );
         BOOST_REQUIRE( escrow.escrow_expiration == et_op.escrow_expiration );
         BOOST_REQUIRE( escrow.zbd_balance == et_op.zbd_amount );
         BOOST_REQUIRE( escrow.ztr_balance == et_op.ztr_amount );
         BOOST_REQUIRE( escrow.pending_fee == ASSET( "0.000 TTR" ) );
         BOOST_REQUIRE( escrow.to_approved );
         BOOST_REQUIRE( escrow.agent_approved );
         BOOST_REQUIRE( escrow.disputed );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_escrow_release )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow release validate" );
      escrow_release_operation op;
      op.from = "alice";
      op.to = "bob";
      op.who = "alice";
      op.agent = "sam";
      op.receiver = "bob";


      BOOST_TEST_MESSAGE( "--- failure when ztr < 0" );
      op.ztr_amount.amount = -1;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when zbd < 0" );
      op.ztr_amount.amount = 0;
      op.zbd_amount.amount = -1;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when ztr == 0 and zbd == 0" );
      op.zbd_amount.amount = 0;
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when zbd is not zbd symbol" );
      op.zbd_amount = ASSET( "1.000 TTR" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when ztr is not ztr symbol" );
      op.zbd_amount.symbol = ZBD_SYMBOL;
      op.ztr_amount = ASSET( "1.000 TBD" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "--- success" );
      op.ztr_amount.symbol = ZTR_SYMBOL;
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_escrow_release_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_release_authorities" );
      escrow_release_operation op;
      op.from = "alice";
      op.to = "bob";
      op.who = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.who = "bob";
      auths.clear();
      expected.clear();
      expected.insert( "bob" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.who = "sam";
      auths.clear();
      expected.clear();
      expected.insert( "sam" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_escrow_release )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: escrow_release_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );

      escrow_transfer_operation et_op;
      et_op.from = "alice";
      et_op.to = "bob";
      et_op.agent = "sam";
      et_op.ztr_amount = ASSET( "1.000 TTR" );
      et_op.fee = ASSET( "0.100 TTR" );
      et_op.ratification_deadline = db->head_block_time() + ZATTERA_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * ZATTERA_BLOCK_INTERVAL;

      signed_transaction tx;
      tx.operations.push_back( et_op );

      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- failure releasing funds prior to approval" );
      escrow_release_operation op;
      op.from = et_op.from;
      op.to = et_op.to;
      op.agent = et_op.agent;
      op.who = et_op.from;
      op.receiver = et_op.to;
      op.ztr_amount = ASSET( "0.100 TTR" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      escrow_approve_operation ea_b_op;
      ea_b_op.from = "alice";
      ea_b_op.to = "bob";
      ea_b_op.agent = "sam";
      ea_b_op.who = "bob";

      escrow_approve_operation ea_s_op;
      ea_s_op.from = "alice";
      ea_s_op.to = "bob";
      ea_s_op.agent = "sam";
      ea_s_op.who = "sam";

      tx.clear();
      tx.operations.push_back( ea_b_op );
      tx.operations.push_back( ea_s_op );
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempts to release non-disputed escrow to 'to'" );
      op.who = et_op.agent;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when 'agent' attempts to release non-disputed escrow to 'from' " );
      op.receiver = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempt to release non-disputed escrow to not 'to' or 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when other attempts to release non-disputed escrow to 'to'" );
      op.receiver = et_op.to;
      op.who = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when other attempts to release non-disputed escrow to 'from' " );
      op.receiver = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when other attempt to release non-disputed escrow to not 'to' or 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attemtps to release non-disputed escrow to 'to'" );
      op.receiver = et_op.to;
      op.who = et_op.to;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when 'to' attempts to release non-dispured escrow to 'agent' " );
      op.receiver = et_op.agent;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release non-disputed escrow to not 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed escrow to 'to' from 'from'" );
      op.receiver = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_escrow( op.from, op.escrow_id ).ztr_balance == ASSET( "0.900 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.000 TTR" ) );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed escrow to 'from'" );
      op.receiver = et_op.from;
      op.who = et_op.from;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release non-disputed escrow to 'agent'" );
      op.receiver = et_op.agent;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed escrow to not 'from'" );
      op.receiver = "dave";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed escrow to 'from' from 'to'" );
      op.receiver = et_op.to;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_escrow( op.from, op.escrow_id ).ztr_balance == ASSET( "0.800 TTR" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.100 TTR" ) );


      BOOST_TEST_MESSAGE( "--- failure when releasing more zbd than available" );
      op.ztr_amount = ASSET( "1.000 TTR" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when releasing less ztr than available" );
      op.ztr_amount = ASSET( "0.000 TTR" );
      op.zbd_amount = ASSET( "1.000 TBD" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release disputed escrow" );
      escrow_dispute_operation ed_op;
      ed_op.from = "alice";
      ed_op.to = "bob";
      ed_op.agent = "sam";
      ed_op.who = "alice";

      tx.clear();
      tx.operations.push_back( ed_op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.clear();
      op.from = et_op.from;
      op.receiver = et_op.from;
      op.who = et_op.to;
      op.ztr_amount = ASSET( "0.100 TTR" );
      op.zbd_amount = ASSET( "0.000 TBD" );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release disputed escrow" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.from;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when releasing disputed escrow to an account not 'to' or 'from'" );
      tx.clear();
      op.who = et_op.agent;
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when agent does not match escrow" );
      tx.clear();
      op.who = "dave";
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success releasing disputed escrow with agent to 'to'" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.200 TTR" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).ztr_balance == ASSET( "0.700 TTR" ) );


      BOOST_TEST_MESSAGE( "--- success releasing disputed escrow with agent to 'from'" );
      tx.clear();
      op.receiver = et_op.from;
      op.who = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.100 TTR" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).ztr_balance == ASSET( "0.600 TTR" ) );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release disputed expired escrow" );
      generate_blocks( 2 );

      tx.clear();
      op.receiver = et_op.from;
      op.who = et_op.to;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release disputed expired escrow" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.from;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success releasing disputed expired escrow with agent" );
      tx.clear();
      op.receiver = et_op.from;
      op.who = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.200 TTR" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).ztr_balance == ASSET( "0.500 TTR" ) );


      BOOST_TEST_MESSAGE( "--- success deleting escrow when balances are both zero" );
      tx.clear();
      op.ztr_amount = ASSET( "0.500 TTR" );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.700 TTR" ) );
      ZATTERA_REQUIRE_THROW( db->get_escrow( et_op.from, et_op.escrow_id ), fc::exception );


      tx.clear();
      et_op.ratification_deadline = db->head_block_time() + ZATTERA_BLOCK_INTERVAL;
      et_op.escrow_expiration = db->head_block_time() + 2 * ZATTERA_BLOCK_INTERVAL;
      tx.operations.push_back( et_op );
      tx.operations.push_back( ea_b_op );
      tx.operations.push_back( ea_s_op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_blocks( 2 );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempts to release non-disputed expired escrow to 'to'" );
      tx.clear();
      op.receiver = et_op.to;
      op.who = et_op.agent;
      op.ztr_amount = ASSET( "0.100 TTR" );
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempts to release non-disputed expired escrow to 'from'" );
      tx.clear();
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'agent' attempt to release non-disputed expired escrow to not 'to' or 'from'" );
      tx.clear();
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release non-dispured expired escrow to 'agent'" );
      tx.clear();
      op.who = et_op.to;
      op.receiver = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'to' attempts to release non-disputed expired escrow to not 'from' or 'to'" );
      tx.clear();
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'to' from 'to'" );
      tx.clear();
      op.receiver = et_op.to;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.300 TTR" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).ztr_balance == ASSET( "0.900 TTR" ) );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'from' from 'to'" );
      tx.clear();
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "8.700 TTR" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).ztr_balance == ASSET( "0.800 TTR" ) );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed expired escrow to 'agent'" );
      tx.clear();
      op.who = et_op.from;
      op.receiver = et_op.agent;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure when 'from' attempts to release non-disputed expired escrow to not 'from' or 'to'" );
      tx.clear();
      op.receiver = "dave";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'to' from 'from'" );
      tx.clear();
      op.receiver = et_op.to;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.400 TTR" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).ztr_balance == ASSET( "0.700 TTR" ) );


      BOOST_TEST_MESSAGE( "--- success release non-disputed expired escrow to 'from' from 'from'" );
      tx.clear();
      op.receiver = et_op.from;
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "8.800 TTR" ) );
      BOOST_REQUIRE( db->get_escrow( et_op.from, et_op.escrow_id ).ztr_balance == ASSET( "0.600 TTR" ) );


      BOOST_TEST_MESSAGE( "--- success deleting escrow when balances are zero on non-disputed escrow" );
      tx.clear();
      op.ztr_amount = ASSET( "0.600 TTR" );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.400 TTR" ) );
      ZATTERA_REQUIRE_THROW( db->get_escrow( et_op.from, et_op.escrow_id ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_transfer_to_savings )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_savings_validate" );

      transfer_to_savings_operation op;
      op.from = "alice";
      op.to = "alice";
      op.amount = ASSET( "1.000 TTR" );


      BOOST_TEST_MESSAGE( "failure when 'from' is empty" );
      op.from = "";
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "failure when 'to' is empty" );
      op.from = "alice";
      op.to = "";
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "sucess when 'to' is not empty" );
      op.to = "bob";
      op.validate();


      BOOST_TEST_MESSAGE( "failure when amount is VESTS" );
      op.to = "alice";
      op.amount = ASSET( "1.000000 VESTS" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "success when amount is ZBD" );
      op.amount = ASSET( "1.000 TBD" );
      op.validate();


      BOOST_TEST_MESSAGE( "success when amount is ZTR" );
      op.amount = ASSET( "1.000 TTR" );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_transfer_to_savings_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_savings_authorities" );

      transfer_to_savings_operation op;
      op.from = "alice";
      op.to = "alice";
      op.amount = ASSET( "1.000 TTR" );

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "alice" );
      BOOST_REQUIRE( auths == expected );

      auths.clear();
      expected.clear();
      op.from = "bob";
      op.get_required_active_authorities( auths );
      expected.insert( "bob" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_transfer_to_savings )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_to_savings_apply" );

      ACTORS( (alice)(bob) );
      generate_block();

      fund( "alice", ASSET( "10.000 TTR" ) );
      fund( "alice", ASSET( "10.000 TBD" ) );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "10.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "10.000 TBD" ) );

      transfer_to_savings_operation op;
      signed_transaction tx;

      BOOST_TEST_MESSAGE( "--- failure with insufficient funds" );
      op.from = "alice";
      op.to = "alice";
      op.amount = ASSET( "20.000 TTR" );

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- failure when transferring to non-existent account" );
      op.to = "sam";
      op.amount = ASSET( "1.000 TTR" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success transferring ZTR to self" );
      op.to = "alice";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "9.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_balance == ASSET( "1.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success transferring ZBD to self" );
      op.amount = ASSET( "1.000 TBD" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "9.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_zbd_balance == ASSET( "1.000 TBD" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success transferring ZTR to other" );
      op.to = "bob";
      op.amount = ASSET( "1.000 TTR" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "8.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).savings_balance == ASSET( "1.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success transferring ZBD to other" );
      op.amount = ASSET( "1.000 TBD" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "8.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).savings_zbd_balance == ASSET( "1.000 TBD" ) );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_transfer_from_savings )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_from_savings_validate" );

      transfer_from_savings_operation op;
      op.from = "alice";
      op.request_id = 0;
      op.to = "alice";
      op.amount = ASSET( "1.000 TTR" );


      BOOST_TEST_MESSAGE( "failure when 'from' is empty" );
      op.from = "";
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "failure when 'to' is empty" );
      op.from = "alice";
      op.to = "";
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "sucess when 'to' is not empty" );
      op.to = "bob";
      op.validate();


      BOOST_TEST_MESSAGE( "failure when amount is VESTS" );
      op.to = "alice";
      op.amount = ASSET( "1.000000 VESTS" );
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "success when amount is ZBD" );
      op.amount = ASSET( "1.000 TBD" );
      op.validate();


      BOOST_TEST_MESSAGE( "success when amount is ZTR" );
      op.amount = ASSET( "1.000 TTR" );
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_transfer_from_savings_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_from_savings_authorities" );

      transfer_from_savings_operation op;
      op.from = "alice";
      op.to = "alice";
      op.amount = ASSET( "1.000 TTR" );

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "alice" );
      BOOST_REQUIRE( auths == expected );

      auths.clear();
      expected.clear();
      op.from = "bob";
      op.get_required_active_authorities( auths );
      expected.insert( "bob" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_transfer_from_savings )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: transfer_from_savings_apply" );

      ACTORS( (alice)(bob) );
      generate_block();

      fund( "alice", ASSET( "10.000 TTR" ) );
      fund( "alice", ASSET( "10.000 TBD" ) );

      transfer_to_savings_operation save;
      save.from = "alice";
      save.to = "alice";
      save.amount = ASSET( "10.000 TTR" );

      signed_transaction tx;
      tx.operations.push_back( save );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      save.amount = ASSET( "10.000 TBD" );
      tx.clear();
      tx.operations.push_back( save );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "--- failure when account has insufficient funds" );
      transfer_from_savings_operation op;
      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "20.000 TTR" );
      op.request_id = 0;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- failure withdrawing to non-existant account" );
      op.to = "sam";
      op.amount = ASSET( "1.000 TTR" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success withdrawing ZTR to self" );
      op.to = "alice";

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_balance == ASSET( "9.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 1 );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).from == op.from );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).to == op.to );
      BOOST_REQUIRE( to_string( db->get_savings_withdraw( "alice", op.request_id ).memo ) == op.memo );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).request_id == op.request_id );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).amount == op.amount );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).complete == db->head_block_time() + ZATTERA_SAVINGS_WITHDRAW_TIME );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success withdrawing ZBD to self" );
      op.amount = ASSET( "1.000 TBD" );
      op.request_id = 1;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_zbd_balance == ASSET( "9.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 2 );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).from == op.from );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).to == op.to );
      BOOST_REQUIRE( to_string( db->get_savings_withdraw( "alice", op.request_id ).memo ) == op.memo );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).request_id == op.request_id );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).amount == op.amount );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).complete == db->head_block_time() + ZATTERA_SAVINGS_WITHDRAW_TIME );
      validate_database();


      BOOST_TEST_MESSAGE( "--- failure withdrawing with repeat request id" );
      op.amount = ASSET( "2.000 TTR" );

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- success withdrawing ZTR to other" );
      op.to = "bob";
      op.amount = ASSET( "1.000 TTR" );
      op.request_id = 3;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_balance == ASSET( "8.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 3 );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).from == op.from );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).to == op.to );
      BOOST_REQUIRE( to_string( db->get_savings_withdraw( "alice", op.request_id ).memo ) == op.memo );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).request_id == op.request_id );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).amount == op.amount );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).complete == db->head_block_time() + ZATTERA_SAVINGS_WITHDRAW_TIME );
      validate_database();


      BOOST_TEST_MESSAGE( "--- success withdrawing ZBD to other" );
      op.amount = ASSET( "1.000 TBD" );
      op.request_id = 4;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_zbd_balance == ASSET( "8.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 4 );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).from == op.from );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).to == op.to );
      BOOST_REQUIRE( to_string( db->get_savings_withdraw( "alice", op.request_id ).memo ) == op.memo );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).request_id == op.request_id );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).amount == op.amount );
      BOOST_REQUIRE( db->get_savings_withdraw( "alice", op.request_id ).complete == db->head_block_time() + ZATTERA_SAVINGS_WITHDRAW_TIME );
      validate_database();


      BOOST_TEST_MESSAGE( "--- withdraw on timeout" );
      generate_blocks( db->head_block_time() + ZATTERA_SAVINGS_WITHDRAW_TIME - fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 4 );
      validate_database();

      generate_block();

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "1.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).zbd_balance == ASSET( "1.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 0 );
      validate_database();


      BOOST_TEST_MESSAGE( "--- savings withdraw request limit" );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      op.to = "alice";
      op.amount = ASSET( "0.001 TTR" );

      for( int i = 0; i < ZATTERA_SAVINGS_WITHDRAW_REQUEST_LIMIT; i++ )
      {
         op.request_id = i;
         tx.clear();
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
         BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == i + 1 );
      }

      op.request_id = ZATTERA_SAVINGS_WITHDRAW_REQUEST_LIMIT;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == ZATTERA_SAVINGS_WITHDRAW_REQUEST_LIMIT );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_cancel_transfer_from_savings )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: cancel_transfer_from_savings_validate" );

      cancel_transfer_from_savings_operation op;
      op.from = "alice";
      op.request_id = 0;


      BOOST_TEST_MESSAGE( "--- failure when 'from' is empty" );
      op.from = "";
      ZATTERA_REQUIRE_THROW( op.validate(), fc::exception );


      BOOST_TEST_MESSAGE( "--- sucess when 'from' is not empty" );
      op.from = "alice";
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_cancel_transfer_from_savings_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: cancel_transfer_from_savings_authorities" );

      cancel_transfer_from_savings_operation op;
      op.from = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      expected.insert( "alice" );
      BOOST_REQUIRE( auths == expected );

      auths.clear();
      expected.clear();
      op.from = "bob";
      op.get_required_active_authorities( auths );
      expected.insert( "bob" );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_cancel_transfer_from_savings )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: cancel_transfer_from_savings_apply" );

      ACTORS( (alice)(bob) )
      generate_block();

      fund( "alice", ASSET( "10.000 TTR" ) );

      transfer_to_savings_operation save;
      save.from = "alice";
      save.to = "alice";
      save.amount = ASSET( "10.000 TTR" );

      transfer_from_savings_operation withdraw;
      withdraw.from = "alice";
      withdraw.to = "bob";
      withdraw.request_id = 1;
      withdraw.amount = ASSET( "3.000 TTR" );

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( save );
      tx.operations.push_back( withdraw );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 1 );
      BOOST_REQUIRE( db->get_account( "bob" ).savings_withdraw_requests == 0 );


      BOOST_TEST_MESSAGE( "--- Failure when there is no pending request" );
      cancel_transfer_from_savings_operation op;
      op.from = "alice";
      op.request_id = 0;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 1 );
      BOOST_REQUIRE( db->get_account( "bob" ).savings_withdraw_requests == 0 );


      BOOST_TEST_MESSAGE( "--- Success" );
      op.request_id = 1;

      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_balance == ASSET( "10.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).savings_withdraw_requests == 0 );
      BOOST_REQUIRE( db->get_account( "bob" ).balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).savings_balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "bob" ).savings_withdraw_requests == 0 );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_decline_voting_rights_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: decline_voting_rights_authorities" );

      decline_voting_rights_operation op;
      op.account = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_decline_voting_rights )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: decline_voting_rights_apply" );

      ACTORS( (alice)(bob) );
      generate_block();
      vest( "alice", ASSET( "10.000 TTR" ) );
      vest( "bob", ASSET( "10.000 TTR" ) );
      generate_block();

      account_witness_proxy_operation proxy;
      proxy.account = "bob";
      proxy.proxy = "alice";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( proxy );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      decline_voting_rights_operation op;
      op.account = "alice";


      BOOST_TEST_MESSAGE( "--- success" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& request_idx = db->get_index< decline_voting_rights_request_index >().indices().get< by_account >();
      auto itr = request_idx.find( db->get_account( "alice" ).name );
      BOOST_REQUIRE( itr != request_idx.end() );
      BOOST_REQUIRE( itr->effective_date == db->head_block_time() + ZATTERA_OWNER_AUTH_RECOVERY_PERIOD );


      BOOST_TEST_MESSAGE( "--- failure revoking voting rights with existing request" );
      generate_block();
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- successs cancelling a request" );
      op.decline = false;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      itr = request_idx.find( db->get_account( "alice" ).name );
      BOOST_REQUIRE( itr == request_idx.end() );


      BOOST_TEST_MESSAGE( "--- failure cancelling a request that doesn't exist" );
      generate_block();
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );


      BOOST_TEST_MESSAGE( "--- check account can vote during waiting period" );
      op.decline = true;
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ZATTERA_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );
      BOOST_REQUIRE( db->get_account( "alice" ).can_vote );
      witness_create( "alice", alice_private_key, "foo.bar", alice_private_key.get_public_key(), 0 );

      account_witness_vote_operation witness_vote;
      witness_vote.account = "alice";
      witness_vote.witness = "alice";
      tx.clear();
      tx.operations.push_back( witness_vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      comment_operation comment;
      comment.author = "alice";
      comment.permlink = "test";
      comment.parent_permlink = "test";
      comment.title = "test";
      comment.body = "test";
      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = ZATTERA_100_PERCENT;
      tx.clear();
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();


      BOOST_TEST_MESSAGE( "--- check account cannot vote after request is processed" );
      generate_block();
      BOOST_REQUIRE( !db->get_account( "alice" ).can_vote );
      validate_database();

      itr = request_idx.find( db->get_account( "alice" ).name );
      BOOST_REQUIRE( itr == request_idx.end() );

      const auto& witness_idx = db->get_index< witness_vote_index >().indices().get< by_account_witness >();
      auto witness_itr = witness_idx.find( boost::make_tuple( db->get_account( "alice" ).name, db->get_witness( "alice" ).owner ) );
      BOOST_REQUIRE( witness_itr == witness_idx.end() );

      tx.clear();
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( witness_vote );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      db->get< comment_vote_object, by_comment_voter >( boost::make_tuple( db->get_comment( "alice", string( "test" ) ).id, db->get_account( "alice" ).id ) );

      vote.weight = 0;
      tx.clear();
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      vote.weight = ZATTERA_1_PERCENT * 50;
      tx.clear();
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      proxy.account = "alice";
      proxy.proxy = "bob";
      tx.clear();
      tx.operations.push_back( proxy );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

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

BOOST_AUTO_TEST_CASE( validate_account_creation )
{
   try
   {

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_account_creation_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_authorities" );

      account_create_operation op;
      op.creator = "alice";
      op.new_account_name = "bob";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      BOOST_TEST_MESSAGE( "--- Testing owner authority" );
      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      BOOST_TEST_MESSAGE( "--- Testing active authority" );
      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      BOOST_TEST_MESSAGE( "--- Testing posting authority" );
      expected.clear();
      auths.clear();
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_account_creation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_apply" );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      signed_transaction tx;
      private_key_type priv_key = generate_private_key( "alice" );

      const account_object& init = db->get_account( ZATTERA_GENESIS_WITNESS_NAME );
      asset init_starting_balance = init.balance;

      const auto& gpo = db->get_dynamic_global_properties();

      account_create_operation op;

      op.fee = asset( 100, ZTR_SYMBOL );
      op.new_account_name = "alice";
      op.creator = ZATTERA_GENESIS_WITNESS_NAME;
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 2, priv_key.get_public_key(), 2 );
      op.memo_key = priv_key.get_public_key();
      op.json_metadata = "{\"foo\":\"bar\"}";

      BOOST_TEST_MESSAGE( "--- Test normal account creation" );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      tx.validate();
      db->push_transaction( tx, 0 );

      const account_object& acct = db->get_account( "alice" );
      const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );

      auto vest_shares = gpo.total_vesting_shares;
      auto vests = gpo.total_vesting_fund_ztr;

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
      BOOST_REQUIRE( acct.proxy == "" );
      BOOST_REQUIRE( acct.created == db->head_block_time() );
      BOOST_REQUIRE( acct.balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( acct.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      BOOST_REQUIRE( acct.id._id == acct_auth.id._id );

      /// because init_witness has created vesting shares and blocks have been produced, 100 ZTR is worth less than 100 vesting shares due to rounding
      BOOST_REQUIRE( acct.vesting_shares.amount.value == ( op.fee * ( vest_shares / vests ) ).amount.value );
      BOOST_REQUIRE( acct.vesting_withdraw_rate.amount.value == ASSET( "0.000000 VESTS" ).amount.value );
      BOOST_REQUIRE( acct.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100 TTR" ) ).amount.value == init.balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure of duplicate account creation" );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == priv_key.get_public_key() );
      BOOST_REQUIRE( acct.proxy == "" );
      BOOST_REQUIRE( acct.created == db->head_block_time() );
      BOOST_REQUIRE( acct.balance.amount.value == ASSET( "0.000 TTR " ).amount.value );
      BOOST_REQUIRE( acct.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      BOOST_REQUIRE( acct.vesting_shares.amount.value == ( op.fee * ( vest_shares / vests ) ).amount.value );
      BOOST_REQUIRE( acct.vesting_withdraw_rate.amount.value == ASSET( "0.000000 VESTS" ).amount.value );
      BOOST_REQUIRE( acct.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( ( init_starting_balance - ASSET( "0.100 TTR" ) ).amount.value == init.balance.amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when creator cannot cover fee" );
      tx.signatures.clear();
      tx.operations.clear();
      op.fee = asset( db->get_account( ZATTERA_GENESIS_WITNESS_NAME ).balance.amount + 1, ZTR_SYMBOL );
      op.new_account_name = "bob";
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure covering witness fee" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "10.000 TTR" );
         });
      });
      generate_block();

      tx.clear();
      op.fee = ASSET( "1.000 TTR" );
      tx.operations.push_back( op );
      tx.sign( init_account_priv_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test account creation with temp account does not set recovery account" );
      fund( ZATTERA_TEMP_ACCOUNT, ASSET( "310.000 TTR" ) );
      vest( ZATTERA_TEMP_ACCOUNT, ASSET( "10.000 TTR" ) );
      op.creator = ZATTERA_TEMP_ACCOUNT;
      op.fee = ASSET( "300.000 TTR" );
      op.new_account_name = "bob";
      tx.clear();
      tx.operations.push_back( op );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "bob" ).recovery_account == account_name_type() );
      validate_database();

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_account_update )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_validate" );

      ACTORS( (alice) )

      account_update_operation op;
      op.account = "alice";
      op.posting = authority();
      op.posting->weight_threshold = 1;
      op.posting->add_authorities( "abcdefghijklmnopq", 1 );

      try
      {
         op.validate();

         signed_transaction tx;
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.operations.push_back( op );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );

         BOOST_FAIL( "An exception was not thrown for an invalid account name" );
      }
      catch( fc::exception& ) {}

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_account_update_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_authorities" );

      ACTORS( (alice)(bob) )
      private_key_type active_key = generate_private_key( "new_key" );

      db->modify( db->get< account_authority_object, by_account >( "alice" ), [&]( account_authority_object& a )
      {
         a.active = authority( 1, active_key.get_public_key(), 1 );
      });

      account_update_operation op;
      op.account = "alice";
      op.json_metadata = "{\"success\":true}";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "  Tests when owner authority is not updated ---" );
      BOOST_TEST_MESSAGE( "--- Test failure when no signature" );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when wrong signature" );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when containing additional incorrect signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when containing duplicate signatures" );
      tx.signatures.clear();
      tx.sign( active_key, db->get_chain_id() );
      tx.sign( active_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test success on active key" );
      tx.signatures.clear();
      tx.sign( active_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test success on owner key alone" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      BOOST_TEST_MESSAGE( "  Tests when owner authority is updated ---" );
      BOOST_TEST_MESSAGE( "--- Test failure when updating the owner authority with an active key" );
      tx.signatures.clear();
      tx.operations.clear();
      op.owner = authority( 1, active_key.get_public_key(), 1 );
      tx.operations.push_back( op );
      tx.sign( active_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_owner_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when owner key and active key are present" );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0), tx_missing_owner_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate owner keys are present" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test success when updating the owner authority with an owner key" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_account_update )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_update_apply" );

      ACTORS( (alice) )
      private_key_type new_private_key = generate_private_key( "new_key" );

      BOOST_TEST_MESSAGE( "--- Test normal update" );

      account_update_operation op;
      op.account = "alice";
      op.owner = authority( 1, new_private_key.get_public_key(), 1 );
      op.active = authority( 2, new_private_key.get_public_key(), 2 );
      op.memo_key = new_private_key.get_public_key();
      op.json_metadata = "{\"bar\":\"foo\"}";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const account_object& acct = db->get_account( "alice" );
      const account_authority_object& acct_auth = db->get< account_authority_object, by_account >( "alice" );

      BOOST_REQUIRE( acct.name == "alice" );
      BOOST_REQUIRE( acct_auth.owner == authority( 1, new_private_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( acct_auth.active == authority( 2, new_private_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( acct.memo_key == new_private_key.get_public_key() );

      /* This is being moved out of consensus
      #ifndef IS_LOW_MEM
         BOOST_REQUIRE( acct.json_metadata == "{\"bar\":\"foo\"}" );
      #else
         BOOST_REQUIRE( acct.json_metadata == "" );
      #endif
      */

      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when updating a non-existent account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( new_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception )
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test failure when account authority does not exist" );
      tx.clear();
      op = account_update_operation();
      op.account = "alice";
      op.posting = authority();
      op.posting->weight_threshold = 1;
      op.posting->add_authorities( "dave", 1 );
      tx.operations.push_back( op );
      tx.sign( new_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_account_witness_vote )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_account_witness_vote_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_authorities" );

      ACTORS( (alice)(bob)(sam) )

      fund( "alice", 1000 );
      private_key_type alice_witness_key = generate_private_key( "alice_witness" );
      witness_create( "alice", alice_private_key, "foo.bar", alice_witness_key.get_public_key(), 1000 );

      account_witness_vote_operation op;
      op.account = "bob";
      op.witness = "alice";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
      tx.sign( bob_post_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
      proxy( "bob", "sam" );
      tx.signatures.clear();
      tx.sign( sam_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_account_witness_vote )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_vote_apply" );

      ACTORS( (alice)(bob)(sam) )
      fund( "alice" , 5000 );
      vest( "alice", 5000 );
      fund( "sam", 1000 );

      private_key_type sam_witness_key = generate_private_key( "sam_key" );
      witness_create( "sam", sam_private_key, "foo.bar", sam_witness_key.get_public_key(), 1000 );
      const witness_object& sam_witness = db->get_witness( "sam" );

      const auto& witness_vote_idx = db->get_index< witness_vote_index >().indices().get< by_witness_account >();

      BOOST_TEST_MESSAGE( "--- Test normal vote" );
      account_witness_vote_operation op;
      op.account = "alice";
      op.witness = "sam";
      op.approve = true;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes == alice.vesting_shares.amount );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) != witness_vote_idx.end() );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test revoke vote" );
      op.approve = false;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test failure when attempting to revoke a non-existent vote" );

      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );
      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test proxied vote" );
      proxy( "alice", "bob" );
      tx.operations.clear();
      tx.signatures.clear();
      op.approve = true;
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes == ( bob.proxied_vsf_votes_total() + bob.vesting_shares.amount ) );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) != witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test vote from a proxied account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( sam_witness.votes == ( bob.proxied_vsf_votes_total() + bob.vesting_shares.amount ) );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) != witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test revoke proxied vote" );
      tx.operations.clear();
      tx.signatures.clear();
      op.account = "bob";
      op.approve = false;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( sam_witness.votes.value == 0 );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, bob.name ) ) == witness_vote_idx.end() );
      BOOST_REQUIRE( witness_vote_idx.find( std::make_tuple( sam_witness.owner, alice.name ) ) == witness_vote_idx.end() );

      BOOST_TEST_MESSAGE( "--- Test failure when voting for a non-existent account" );
      tx.operations.clear();
      tx.signatures.clear();
      op.witness = "dave";
      op.approve = true;
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when voting for an account that is not a witness" );
      tx.operations.clear();
      tx.signatures.clear();
      op.witness = "alice";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_account_witness_proxy )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_account_witness_proxy_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_authorities" );

      ACTORS( (alice)(bob) )

      account_witness_proxy_operation op;
      op.account = "bob";
      op.proxy = "alice";

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );

      BOOST_TEST_MESSAGE( "--- Test failure when no signatures" );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by a signature not in the account's authority" );
      tx.sign( bob_post_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure when duplicate signatures" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure when signed by an additional signature not in the creator's authority" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness signature" );
      tx.signatures.clear();
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test failure with proxy signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_account_witness_proxy )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_witness_proxy_apply" );

      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 1000 );
      vest( "alice", 1000 );
      fund( "bob", 3000 );
      vest( "bob", 3000 );
      fund( "sam", 5000 );
      vest( "sam", 5000 );
      fund( "dave", 7000 );
      vest( "dave", 7000 );

      BOOST_TEST_MESSAGE( "--- Test setting proxy to another account from self." );
      // bob -> alice

      account_witness_proxy_operation op;
      op.account = "bob";
      op.proxy = "alice";

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob.proxy == "alice" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( alice.proxy == ZATTERA_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total() == bob.vesting_shares.amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test changing proxy" );
      // bob->sam

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "sam";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == ZATTERA_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total().value == bob.vesting_shares.amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when changing proxy to existing proxy" );

      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), fc::exception );

      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == ZATTERA_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == bob.vesting_shares.amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test adding a grandparent proxy" );
      // bob->sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "dave";
      op.account = "sam";
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == "dave" );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == bob.vesting_shares.amount );
      BOOST_REQUIRE( dave.proxy == ZATTERA_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( dave.proxied_vsf_votes_total() == ( sam.vesting_shares + bob.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test adding a grandchild proxy" );
      //       alice
      //         |
      // bob->  sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = "sam";
      op.account = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.proxy == "sam" );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( bob.proxy == "sam" );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == "dave" );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == ( bob.vesting_shares + alice.vesting_shares ).amount );
      BOOST_REQUIRE( dave.proxy == ZATTERA_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( dave.proxied_vsf_votes_total() == ( sam.vesting_shares + bob.vesting_shares + alice.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test removing a grandchild proxy" );
      // alice->sam->dave

      tx.operations.clear();
      tx.signatures.clear();
      op.proxy = ZATTERA_PROXY_TO_SELF_ACCOUNT;
      op.account = "bob";
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice.proxy == "sam" );
      BOOST_REQUIRE( alice.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( bob.proxy == ZATTERA_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( bob.proxied_vsf_votes_total().value == 0 );
      BOOST_REQUIRE( sam.proxy == "dave" );
      BOOST_REQUIRE( sam.proxied_vsf_votes_total() == alice.vesting_shares.amount );
      BOOST_REQUIRE( dave.proxy == ZATTERA_PROXY_TO_SELF_ACCOUNT );
      BOOST_REQUIRE( dave.proxied_vsf_votes_total() == ( sam.vesting_shares + alice.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test votes are transferred when a proxy is added" );
      account_witness_vote_operation vote;
      vote.account= "bob";
      vote.witness = ZATTERA_GENESIS_WITNESS_NAME;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      op.account = "alice";
      op.proxy = "bob";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_witness( ZATTERA_GENESIS_WITNESS_NAME ).votes == ( alice.vesting_shares + bob.vesting_shares ).amount );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test votes are removed when a proxy is removed" );
      op.proxy = ZATTERA_PROXY_TO_SELF_ACCOUNT;
      tx.signatures.clear();
      tx.operations.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_witness( ZATTERA_GENESIS_WITNESS_NAME ).votes == bob.vesting_shares.amount );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_account_creation_with_delegation_authorities )
{
   try
   {
     BOOST_TEST_MESSAGE( "Testing: account_create_with_delegation_authorities" );

      account_create_with_delegation_operation op;
      op.creator = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.clear();
      auths.clear();
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()

}

BOOST_AUTO_TEST_CASE( apply_account_creation_with_delegation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_create_with_delegation_apply" );
      signed_transaction tx;
      ACTORS( (alice) );
      // 150 * fee = ( 5 * ZTR ) + ZP
      //auto gpo = db->get_dynamic_global_properties();
      generate_blocks(1);
      fund( "alice", ASSET("1510.000 TTR") );
      vest( "alice", ASSET("1000.000 TTR") );

      private_key_type priv_key = generate_private_key( "temp_key" );

      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& w )
         {
            w.median_props.account_creation_fee = ASSET( "1.000 TTR" );
         });
      });

      generate_block();

      // This test passed pre HF20
      BOOST_TEST_MESSAGE( "--- Test deprecation. " );
      account_create_with_delegation_operation op;
      op.fee = ASSET( "10.000 TTR" );
      op.delegation = ASSET( "100000000.000000 VESTS" );
      op.creator = "alice";
      op.new_account_name = "bob";
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 2, priv_key.get_public_key(), 2 );
      op.memo_key = priv_key.get_public_key();
      op.json_metadata = "{\"foo\":\"bar\"}";
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( enforce_post_rate_limit )
{
   try
   {
      ACTORS( (alice) )

      fund( "alice", 10000 );
      vest( "alice", 10000 );

      comment_operation op;
      op.author = "alice";
      op.permlink = "test1";
      op.parent_author = "";
      op.parent_permlink = "test";
      op.body = "test";

      signed_transaction tx;

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test1" ) ).reward_weight == ZATTERA_100_PERCENT );

      tx.operations.clear();
      tx.signatures.clear();

      generate_blocks( db->head_block_time() + ZATTERA_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );

      op.permlink = "test2";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test2" ) ).reward_weight == ZATTERA_100_PERCENT );

      generate_blocks( db->head_block_time() + ZATTERA_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test3";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test3" ) ).reward_weight == ZATTERA_100_PERCENT );

      generate_blocks( db->head_block_time() + ZATTERA_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test4";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test4" ) ).reward_weight == ZATTERA_100_PERCENT );

      generate_blocks( db->head_block_time() + ZATTERA_MIN_ROOT_COMMENT_INTERVAL + fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );

      tx.operations.clear();
      tx.signatures.clear();

      op.permlink = "test5";

      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_comment( "alice", string( "test5" ) ).reward_weight == ZATTERA_100_PERCENT );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( clear_null_account )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing clearing the null account's balances on block" );

      ACTORS( (alice) );
      generate_block();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      fund( "alice", ASSET( "10.000 TTR" ) );
      fund( "alice", ASSET( "10.000 TBD" ) );

      transfer_operation transfer1;
      transfer1.from = "alice";
      transfer1.to = ZATTERA_NULL_ACCOUNT;
      transfer1.amount = ASSET( "1.000 TTR" );

      transfer_operation transfer2;
      transfer2.from = "alice";
      transfer2.to = ZATTERA_NULL_ACCOUNT;
      transfer2.amount = ASSET( "2.000 TBD" );

      transfer_to_vesting_operation vest;
      vest.from = "alice";
      vest.to = ZATTERA_NULL_ACCOUNT;
      vest.amount = ASSET( "3.000 TTR" );

      transfer_to_savings_operation save1;
      save1.from = "alice";
      save1.to = ZATTERA_NULL_ACCOUNT;
      save1.amount = ASSET( "4.000 TTR" );

      transfer_to_savings_operation save2;
      save2.from = "alice";
      save2.to = ZATTERA_NULL_ACCOUNT;
      save2.amount = ASSET( "5.000 TBD" );

      BOOST_TEST_MESSAGE( "--- Transferring to NULL Account" );

      signed_transaction tx;
      tx.operations.push_back( transfer1 );
      tx.operations.push_back( transfer2 );
      tx.operations.push_back( vest );
      tx.operations.push_back( save1);
      tx.operations.push_back( save2 );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( ZATTERA_NULL_ACCOUNT ), [&]( account_object& a )
         {
            a.reward_ztr_balance = ASSET( "1.000 TTR" );
            a.reward_zbd_balance = ASSET( "1.000 TBD" );
            a.reward_vesting_balance = ASSET( "1.000000 VESTS" );
            a.reward_vesting_ztr = ASSET( "1.000 TTR" );
         });

         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_supply += ASSET( "2.000 TTR" );
            gpo.virtual_supply += ASSET( "3.000 TTR" );
            gpo.current_zbd_supply += ASSET( "1.000 TBD" );
            gpo.pending_rewarded_vesting_shares += ASSET( "1.000000 VESTS" );
            gpo.pending_rewarded_vesting_ztr += ASSET( "1.000 TTR" );
         });
      });

      validate_database();

      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).balance == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).zbd_balance == ASSET( "2.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).vesting_shares > ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).savings_balance == ASSET( "4.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).savings_zbd_balance == ASSET( "5.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_zbd_balance == ASSET( "1.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_ztr_balance == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_vesting_balance == ASSET( "1.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_vesting_ztr == ASSET( "1.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "2.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "3.000 TBD" ) );

      BOOST_TEST_MESSAGE( "--- Generating block to clear balances" );
      generate_block();
      validate_database();

      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).vesting_shares == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).savings_balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).savings_zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_zbd_balance == ASSET( "0.000 TBD" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_ztr_balance == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_vesting_balance == ASSET( "0.000000 VESTS" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).reward_vesting_ztr == ASSET( "0.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "2.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance == ASSET( "3.000 TBD" ) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( recover_account )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account recovery" );

      ACTORS( (alice) );
      fund( "alice", 1000000 );

      BOOST_TEST_MESSAGE( "Creating account bob with alice" );

      account_create_operation acc_create;
      acc_create.fee = ASSET( "10.000 TTR" );
      acc_create.creator = "alice";
      acc_create.new_account_name = "bob";
      acc_create.owner = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
      acc_create.active = authority( 1, generate_private_key( "bob_active" ).get_public_key(), 1 );
      acc_create.posting = authority( 1, generate_private_key( "bob_posting" ).get_public_key(), 1 );
      acc_create.memo_key = generate_private_key( "bob_memo" ).get_public_key();
      acc_create.json_metadata = "";


      signed_transaction tx;
      tx.operations.push_back( acc_create );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& bob_auth = db->get< account_authority_object, by_account >( "bob" );
      BOOST_REQUIRE( bob_auth.owner == acc_create.owner );


      BOOST_TEST_MESSAGE( "Changing bob's owner authority" );

      account_update_operation acc_update;
      acc_update.account = "bob";
      acc_update.owner = authority( 1, generate_private_key( "bad_key" ).get_public_key(), 1 );
      acc_update.memo_key = acc_create.memo_key;
      acc_update.json_metadata = "";

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( acc_update );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );


      BOOST_TEST_MESSAGE( "Creating recover request for bob with alice" );

      request_account_recovery_operation request;
      request.recovery_account = "alice";
      request.account_to_recover = "bob";
      request.new_owner_authority = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( bob_auth.owner == *acc_update.owner );


      BOOST_TEST_MESSAGE( "Recovering bob's account with original owner auth and new secret" );

      generate_blocks( db->head_block_time() + ZATTERA_OWNER_UPDATE_LIMIT );

      recover_account_operation recover;
      recover.account_to_recover = "bob";
      recover.new_owner_authority = request.new_owner_authority;
      recover.recent_owner_authority = acc_create.owner;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "new_key" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );
      const auto& owner1 = db->get< account_authority_object, by_account >("bob").owner;

      BOOST_REQUIRE( owner1 == recover.new_owner_authority );


      BOOST_TEST_MESSAGE( "Creating new recover request for a bogus key" );

      request.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );


      BOOST_TEST_MESSAGE( "Testing failure when bob does not have new authority" );

      generate_blocks( db->head_block_time() + ZATTERA_OWNER_UPDATE_LIMIT + fc::seconds( ZATTERA_BLOCK_INTERVAL ) );

      recover.new_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "idontknow" ), db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner2 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner2 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );


      BOOST_TEST_MESSAGE( "Testing failure when bob does not have old authority" );

      recover.recent_owner_authority = authority( 1, generate_private_key( "idontknow" ).get_public_key(), 1 );
      recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      tx.sign( generate_private_key( "idontknow" ), db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner3 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner3 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );


      BOOST_TEST_MESSAGE( "Testing using the same old owner auth again for recovery" );

      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );
      recover.new_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& owner4 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner4 == recover.new_owner_authority );

      BOOST_TEST_MESSAGE( "Creating a recovery request that will expire" );

      request.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& request_idx = db->get_index< account_recovery_request_index >().indices();
      auto req_itr = request_idx.begin();

      BOOST_REQUIRE( req_itr->account_to_recover == "bob" );
      BOOST_REQUIRE( req_itr->new_owner_authority == authority( 1, generate_private_key( "expire" ).get_public_key(), 1 ) );
      BOOST_REQUIRE( req_itr->expires == db->head_block_time() + ZATTERA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD );
      auto expires = req_itr->expires;
      ++req_itr;
      BOOST_REQUIRE( req_itr == request_idx.end() );

      generate_blocks( time_point_sec( expires - ZATTERA_BLOCK_INTERVAL ), true );

      const auto& new_request_idx = db->get_index< account_recovery_request_index >().indices();
      BOOST_REQUIRE( new_request_idx.begin() != new_request_idx.end() );

      generate_block();

      BOOST_REQUIRE( new_request_idx.begin() == new_request_idx.end() );

      recover.new_owner_authority = authority( 1, generate_private_key( "expire" ).get_public_key(), 1 );
      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() );
      tx.sign( generate_private_key( "expire" ), db->get_chain_id() );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner5 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner5 == authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 ) );

      BOOST_TEST_MESSAGE( "Expiring owner authority history" );

      acc_update.owner = authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( acc_update );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ( ZATTERA_OWNER_AUTH_RECOVERY_PERIOD - ZATTERA_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD ) );
      generate_block();

      request.new_owner_authority = authority( 1, generate_private_key( "last key" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( request );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      recover.new_owner_authority = request.new_owner_authority;
      recover.recent_owner_authority = authority( 1, generate_private_key( "bob_owner" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "bob_owner" ), db->get_chain_id() );
      tx.sign( generate_private_key( "last key" ), db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      const auto& owner6 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner6 == authority( 1, generate_private_key( "new_key" ).get_public_key(), 1 ) );

      recover.recent_owner_authority = authority( 1, generate_private_key( "foo bar" ).get_public_key(), 1 );

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( recover );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( generate_private_key( "foo bar" ), db->get_chain_id() );
      tx.sign( generate_private_key( "last key" ), db->get_chain_id() );
      db->push_transaction( tx, 0 );
      const auto& owner7 = db->get< account_authority_object, by_account >("bob").owner;
      BOOST_REQUIRE( owner7 == authority( 1, generate_private_key( "last key" ).get_public_key(), 1 ) );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( change_account_recovery )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing change_recovery_account_operation" );

      ACTORS( (alice)(bob)(sam)(tyler) )

      auto change_recovery_account = [&]( const std::string& account_to_recover, const std::string& new_recovery_account )
      {
         change_recovery_account_operation op;
         op.account_to_recover = account_to_recover;
         op.new_recovery_account = new_recovery_account;

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( alice_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      auto recover_account = [&]( const std::string& account_to_recover, const fc::ecc::private_key& new_owner_key, const fc::ecc::private_key& recent_owner_key )
      {
         recover_account_operation op;
         op.account_to_recover = account_to_recover;
         op.new_owner_authority = authority( 1, public_key_type( new_owner_key.get_public_key() ), 1 );
         op.recent_owner_authority = authority( 1, public_key_type( recent_owner_key.get_public_key() ), 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( recent_owner_key, db->get_chain_id() );
         // only Alice -> throw
         ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         tx.signatures.clear();
         tx.sign( new_owner_key, db->get_chain_id() );
         // only Sam -> throw
         ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
         tx.sign( recent_owner_key, db->get_chain_id() );
         // Alice+Sam -> OK
         db->push_transaction( tx, 0 );
      };

      auto request_account_recovery = [&]( const std::string& recovery_account, const fc::ecc::private_key& recovery_account_key, const std::string& account_to_recover, const public_key_type& new_owner_key )
      {
         request_account_recovery_operation op;
         op.recovery_account    = recovery_account;
         op.account_to_recover  = account_to_recover;
         op.new_owner_authority = authority( 1, new_owner_key, 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( recovery_account_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      auto change_owner = [&]( const std::string& account, const fc::ecc::private_key& old_private_key, const public_key_type& new_public_key )
      {
         account_update_operation op;
         op.account = account;
         op.owner = authority( 1, new_public_key, 1 );

         signed_transaction tx;
         tx.operations.push_back( op );
         tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         tx.sign( old_private_key, db->get_chain_id() );
         db->push_transaction( tx, 0 );
      };

      // if either/both users do not exist, we shouldn't allow it
      ZATTERA_REQUIRE_THROW( change_recovery_account("alice", "nobody"), fc::exception );
      ZATTERA_REQUIRE_THROW( change_recovery_account("haxer", "sam"   ), fc::exception );
      ZATTERA_REQUIRE_THROW( change_recovery_account("haxer", "nobody"), fc::exception );
      change_recovery_account("alice", "sam");

      fc::ecc::private_key alice_priv1 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k1" ) );
      fc::ecc::private_key alice_priv2 = fc::ecc::private_key::regenerate( fc::sha256::hash( "alice_k2" ) );
      public_key_type alice_pub1 = public_key_type( alice_priv1.get_public_key() );

      generate_blocks( db->head_block_time() + ZATTERA_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds( ZATTERA_BLOCK_INTERVAL ), true );
      // cannot request account recovery until recovery account is approved
      ZATTERA_REQUIRE_THROW( request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 ), fc::exception );
      generate_blocks(1);
      // cannot finish account recovery until requested
      ZATTERA_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
      // do the request
      request_account_recovery( "sam", sam_private_key, "alice", alice_pub1 );
      // can't recover with the current owner key
      ZATTERA_REQUIRE_THROW( recover_account( "alice", alice_priv1, alice_private_key ), fc::exception );
      // unless we change it!
      change_owner( "alice", alice_private_key, public_key_type( alice_priv2.get_public_key() ) );
      recover_account( "alice", alice_priv1, alice_private_key );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( track_account_bandwidth )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: account_bandwidth" );
      ACTORS( (alice)(bob) )
      generate_block();
      vest( "alice", ASSET( "10.000 TTR" ) );
      fund( "alice", ASSET( "10.000 TTR" ) );
      vest( "bob", ASSET( "10.000 TTR" ) );

      generate_block();
      db->skip_transaction_delta_check = false;

      BOOST_TEST_MESSAGE( "--- Test first tx in block" );

      signed_transaction tx;
      transfer_operation op;

      op.from = "alice";
      op.to = "bob";
      op.amount = ASSET( "1.000 TTR" );

      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      auto last_bandwidth_update = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).last_bandwidth_update;
      auto average_bandwidth = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).average_bandwidth;
      BOOST_REQUIRE( last_bandwidth_update == db->head_block_time() );
      BOOST_REQUIRE( average_bandwidth == fc::raw::pack_size( tx ) * 10 * ZATTERA_BANDWIDTH_PRECISION );
      auto total_bandwidth = average_bandwidth;

      BOOST_TEST_MESSAGE( "--- Test second tx in block" );

      op.amount = ASSET( "0.100 TTR" );
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      last_bandwidth_update = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).last_bandwidth_update;
      average_bandwidth = db->get< plugins::witness::account_bandwidth_object, plugins::witness::by_account_bandwidth_type >( boost::make_tuple( "alice", plugins::witness::bandwidth_type::market ) ).average_bandwidth;
      BOOST_REQUIRE( last_bandwidth_update == db->head_block_time() );
      BOOST_REQUIRE( average_bandwidth == total_bandwidth + fc::raw::pack_size( tx ) * 10 * ZATTERA_BANDWIDTH_PRECISION );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_account_claim )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: claim_account_validate" );

      claim_account_operation op;
      op.creator = "alice";
      op.fee = ASSET( "1.000 TTR" );

      BOOST_TEST_MESSAGE( "--- Test failure with invalid account name" );
      op.creator = "aA0";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with invalid fee symbol" );
      op.creator = "alice";
      op.fee = ASSET( "1.000 TBD" );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with negative fee" );
      op.fee = ASSET( "-1.000 TTR" );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with non-zero extensions" );
      op.fee = ASSET( "1.000 TTR" );
      op.extensions.insert( future_extensions( void_t() ) );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test success" );
      op.extensions.clear();
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_account_claim_authorities )
{
   try
   {
     BOOST_TEST_MESSAGE( "Testing: claim_account_authorities" );

      claim_account_operation op;
      op.creator = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.clear();
      auths.clear();
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_account_claim )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: claim_account_apply" );

      ACTORS( (alice) )
      generate_block();

      fund( "alice", ASSET( "15.000 TTR" ) );
      generate_block();

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&](witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "20.000 TTR" );
         });
      });
      generate_block();

      signed_transaction tx;
      claim_account_operation op;

      BOOST_TEST_MESSAGE( "--- Test failure when creator cannot cover fee" );
      op.creator = "alice";
      op.fee = ASSET( "20.000 TTR" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();


      // This test will be removed when soft forking for discount creation is implemented
      BOOST_TEST_MESSAGE( "--- Test failure covering witness fee" );

      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_witness_schedule_object(), [&]( witness_schedule_object& wso )
         {
            wso.median_props.account_creation_fee = ASSET( "5.000 TTR" );
         });
      });
      generate_block();

      op.fee = ASSET( "1.000 TTR" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test success claiming an account" );
      op.fee = ASSET( "5.000 TTR" );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( db->get_account( "alice" ).pending_claimed_accounts == 1 );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "10.000 TTR" ) );
      BOOST_REQUIRE( db->get_account( ZATTERA_NULL_ACCOUNT ).balance == ASSET( "5.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test claiming from a non-existent account" );
      op.creator = "bob";
      tx.clear();
      tx.operations.push_back( op );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test success claiming a second account" );
      generate_block();
      op.creator = "alice";
      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( db->get_account( "alice" ).pending_claimed_accounts == 2 );
      BOOST_REQUIRE( db->get_account( "alice" ).balance == ASSET( "5.000 TTR" ) );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test failure on claim overflow" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( "alice" ), [&]( account_object& a )
         {
            a.pending_claimed_accounts = std::numeric_limits< int64_t >::max();
         });
      });
      generate_block();

      tx.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_claimed_account_creation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create_claimed_account_validate" );

      private_key_type priv_key = generate_private_key( "alice" );

      create_claimed_account_operation op;
      op.creator = "alice";
      op.new_account_name = "bob";
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 1, priv_key.get_public_key(), 1 );
      op.posting = authority( 1, priv_key.get_public_key(), 1 );
      op.memo_key = priv_key.get_public_key();

      BOOST_TEST_MESSAGE( "--- Test invalid creator name" );
      op.creator = "aA0";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid new account name" );
      op.creator = "alice";
      op.new_account_name = "aA0";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid account name in owner authority" );
      op.new_account_name = "bob";
      op.owner = authority( 1, "aA0", 1 );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid account name in active authority" );
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 1, "aA0", 1 );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid account name in posting authority" );
      op.active = authority( 1, priv_key.get_public_key(), 1 );
      op.posting = authority( 1, "aA0", 1 );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test invalid JSON metadata" );
      op.posting = authority( 1, priv_key.get_public_key(), 1 );
      op.json_metadata = "{\"foo\",\"bar\"}";
      BOOST_REQUIRE_THROW( op.validate(), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test non UTF-8 JSON metadata" );
      op.json_metadata = "{\"foo\":\"\xc0\xc1\"}";
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test failure with non-zero extensions" );
      op.json_metadata = "";
      op.extensions.insert( future_extensions( void_t() ) );
      BOOST_REQUIRE_THROW( op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Test success" );
      op.extensions.clear();
      op.validate();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_claimed_account_creation_authorities )
{
   try
   {
     BOOST_TEST_MESSAGE( "Testing: create_claimed_account_authorities" );

      create_claimed_account_operation op;
      op.creator = "alice";

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.insert( "alice" );
      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected.clear();
      auths.clear();
      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_claimed_account_creation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: create_claimed_account_apply" );

      ACTORS( (alice) )
      vest( ZATTERA_TEMP_ACCOUNT, ASSET( "10.000 TTR" ) );
      generate_block();

      signed_transaction tx;
      create_claimed_account_operation op;
      private_key_type priv_key = generate_private_key( "bob" );

      BOOST_TEST_MESSAGE( "--- Test failure when creator has not claimed an account" );
      op.creator = "alice";
      op.new_account_name = "bob";
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      op.active = authority( 2, priv_key.get_public_key(), 2 );
      op.posting = authority( 3, priv_key.get_public_key(), 3 );
      op.memo_key = priv_key.get_public_key();
      op.json_metadata = "{\"foo\":\"bar\"}";
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure creating account with non-existent account auth" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( "alice" ), [&]( account_object& a )
         {
            a.pending_claimed_accounts = 2;
         });
      });
      generate_block();
      op.owner = authority( 1, "bob", 1 );
      tx.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test success creating claimed account" );
      op.owner = authority( 1, priv_key.get_public_key(), 1 );
      tx.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      const auto& bob = db->get_account( "bob" );
      const auto& bob_auth = db->get< account_authority_object, by_account >( "bob" );

      BOOST_REQUIRE( bob.name == "bob" );
      BOOST_REQUIRE( bob_auth.owner == authority( 1, priv_key.get_public_key(), 1 ) );
      BOOST_REQUIRE( bob_auth.active == authority( 2, priv_key.get_public_key(), 2 ) );
      BOOST_REQUIRE( bob_auth.posting == authority( 3, priv_key.get_public_key(), 3 ) );
      BOOST_REQUIRE( bob.memo_key == priv_key.get_public_key() );
#ifndef IS_LOW_MEM // json_metadata is not stored on low memory nodes
      BOOST_REQUIRE( bob.json_metadata == "{\"foo\":\"bar\"}" );
#endif
      BOOST_REQUIRE( bob.proxy == "" );
      BOOST_REQUIRE( bob.recovery_account == "alice" );
      BOOST_REQUIRE( bob.created == db->head_block_time() );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      BOOST_REQUIRE( bob.vesting_shares.amount.value == ASSET( "0.000000 VESTS" ).amount.value );
      BOOST_REQUIRE( bob.id._id == bob_auth.id._id );

      BOOST_REQUIRE( db->get_account( "alice" ).pending_claimed_accounts == 1 );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test failure creating duplicate account name" );
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      BOOST_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();


      BOOST_TEST_MESSAGE( "--- Test account creation with temp account does not set recovery account" );
      generate_block();
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( ZATTERA_TEMP_ACCOUNT ), [&]( account_object& a )
         {
            a.pending_claimed_accounts = 1;
         });
      });
      generate_block();
      op.creator = ZATTERA_TEMP_ACCOUNT;
      op.new_account_name = "charlie";
      tx.clear();
      tx.operations.push_back( op );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "charlie" ).recovery_account == account_name_type() );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

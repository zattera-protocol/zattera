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

BOOST_AUTO_TEST_CASE( validate_witness_update )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: withness_update_validate" );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_witness_update_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_update_authorities" );

      ACTORS( (alice)(bob) );
      fund( "alice", 10000 );

      private_key_type signing_key = generate_private_key( "new_key" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.fee = ASSET( "1.000 TTR" );
      op.block_signing_key = signing_key.get_public_key();

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

      tx.signatures.clear();
      tx.sign( signing_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_witness_update )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_update_apply" );

      ACTORS( (alice) )
      fund( "alice", 10000 );

      private_key_type signing_key = generate_private_key( "new_key" );

      BOOST_TEST_MESSAGE( "--- Test upgrading an account to a witness" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.fee = ASSET( "1.000 TTR" );
      op.block_signing_key = signing_key.get_public_key();
      op.props.account_creation_fee = legacy_zattera_asset::from_asset( asset(ZATTERA_MIN_ACCOUNT_CREATION_FEE + 10, ZTR_SYMBOL) );
      op.props.maximum_block_size = ZATTERA_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      const witness_object& alice_witness = db->get_witness( "alice" );

      BOOST_REQUIRE( alice_witness.owner == "alice" );
      BOOST_REQUIRE( alice_witness.created == db->head_block_time() );
      BOOST_REQUIRE( to_string( alice_witness.url ) == op.url );
      BOOST_REQUIRE( alice_witness.signing_key == op.block_signing_key );
      BOOST_REQUIRE( alice_witness.props.account_creation_fee == op.props.account_creation_fee.to_asset<true>() );
      BOOST_REQUIRE( alice_witness.props.maximum_block_size == op.props.maximum_block_size );
      BOOST_REQUIRE( alice_witness.total_missed == 0 );
      BOOST_REQUIRE( alice_witness.last_aslot == 0 );
      BOOST_REQUIRE( alice_witness.last_confirmed_block_num == 0 );
      BOOST_REQUIRE( alice_witness.votes.value == 0 );
      BOOST_REQUIRE( alice_witness.virtual_last_update == 0 );
      BOOST_REQUIRE( alice_witness.virtual_position == 0 );
      BOOST_REQUIRE( alice_witness.virtual_scheduled_time == fc::uint128_t::max_value() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "10.000 TTR" ).amount.value ); // No fee
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test updating a witness" );

      tx.signatures.clear();
      tx.operations.clear();
      op.url = "bar.foo";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( alice_witness.owner == "alice" );
      BOOST_REQUIRE( alice_witness.created == db->head_block_time() );
      BOOST_REQUIRE( to_string( alice_witness.url ) == "bar.foo" );
      BOOST_REQUIRE( alice_witness.signing_key == op.block_signing_key );
      BOOST_REQUIRE( alice_witness.props.account_creation_fee == op.props.account_creation_fee.to_asset<true>() );
      BOOST_REQUIRE( alice_witness.props.maximum_block_size == op.props.maximum_block_size );
      BOOST_REQUIRE( alice_witness.total_missed == 0 );
      BOOST_REQUIRE( alice_witness.last_aslot == 0 );
      BOOST_REQUIRE( alice_witness.last_confirmed_block_num == 0 );
      BOOST_REQUIRE( alice_witness.votes.value == 0 );
      BOOST_REQUIRE( alice_witness.virtual_last_update == 0 );
      BOOST_REQUIRE( alice_witness.virtual_position == 0 );
      BOOST_REQUIRE( alice_witness.virtual_scheduled_time == fc::uint128_t::max_value() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "10.000 TTR" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when upgrading a non-existent account" );

      tx.signatures.clear();
      tx.operations.clear();
      op.owner = "bob";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_feed_publish )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: feed_publish_validate" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_feed_publish_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: feed_publish_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );
      witness_create( "alice", alice_private_key, "foo.bar", alice_private_key.get_public_key(), 1000 );

      feed_publish_operation op;
      op.publisher = "alice";
      op.exchange_rate = price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) );

      signed_transaction tx;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );

      BOOST_TEST_MESSAGE( "--- Test failure when no signature." );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure with incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_post_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_missing_active_auth );

      BOOST_TEST_MESSAGE( "--- Test failure with duplicate signature" );
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_duplicate_sig );

      BOOST_TEST_MESSAGE( "--- Test failure with additional incorrect signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, database::skip_transaction_dupe_check ), tx_irrelevant_sig );

      BOOST_TEST_MESSAGE( "--- Test success with witness account signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, database::skip_transaction_dupe_check );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_feed_publish )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: feed_publish_apply" );

      ACTORS( (alice) )
      fund( "alice", 10000 );
      witness_create( "alice", alice_private_key, "foo.bar", alice_private_key.get_public_key(), 1000 );

      BOOST_TEST_MESSAGE( "--- Test publishing price feed" );
      feed_publish_operation op;
      op.publisher = "alice";
      op.exchange_rate = price( ASSET( "1.000 TBD" ), ASSET( "1000.000 TTR" ) ); // 1000 ZTR : 1 ZBD

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      witness_object& alice_witness = const_cast< witness_object& >( db->get_witness( "alice" ) );

      BOOST_REQUIRE( alice_witness.zbd_exchange_rate == op.exchange_rate );
      BOOST_REQUIRE( alice_witness.last_zbd_exchange_update == db->head_block_time() );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure publishing to non-existent witness" );

      tx.operations.clear();
      tx.signatures.clear();
      op.publisher = "bob";
      tx.sign( alice_private_key, db->get_chain_id() );

      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure publishing with ZBD base symbol" );

      tx.operations.clear();
      tx.signatures.clear();
      op.exchange_rate = price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) );
      tx.sign( alice_private_key, db->get_chain_id() );

      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test updating price feed" );

      tx.operations.clear();
      tx.signatures.clear();
      op.exchange_rate = price( ASSET(" 1.000 TBD" ), ASSET( "1500.000 TTR" ) );
      op.publisher = "alice";
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );

      db->push_transaction( tx, 0 );

      alice_witness = const_cast< witness_object& >( db->get_witness( "alice" ) );
      // BOOST_REQUIRE( std::abs( alice_witness.zbd_exchange_rate.to_real() - op.exchange_rate.to_real() ) < 0.0000005 );
      BOOST_REQUIRE( alice_witness.last_zbd_exchange_update == db->head_block_time() );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_witness_set_properties )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_set_properties_validate" );

      ACTORS( (alice) )
      fund( "alice", 10000 );
      private_key_type signing_key = generate_private_key( "old_key" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.fee = ASSET( "1.000 TTR" );
      op.block_signing_key = signing_key.get_public_key();
      op.props.account_creation_fee = legacy_zattera_asset::from_asset( asset(ZATTERA_MIN_ACCOUNT_CREATION_FEE + 10, ZTR_SYMBOL) );
      op.props.maximum_block_size = ZATTERA_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      generate_block();

      BOOST_TEST_MESSAGE( "--- failure when signing key is not present" );
      witness_set_properties_operation prop_op;
      prop_op.owner = "alice";
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting account_creation_fee with incorrect symbol" );
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
      prop_op.props[ "account_creation_fee" ] = fc::raw::pack_to_vector( ASSET( "2.000 TBD" ) );
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting maximum_block_size below ZATTERA_MIN_BLOCK_SIZE_LIMIT" );
      prop_op.props.erase( "account_creation_fee" );
      prop_op.props[ "maximum_block_size" ] = fc::raw::pack_to_vector( ZATTERA_MIN_BLOCK_SIZE_LIMIT - 1 );
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting zbd_interest_rate with negative number" );
      prop_op.props.erase( "maximum_block_size" );
      prop_op.props[ "zbd_interest_rate" ] = fc::raw::pack_to_vector( -700 );
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting zbd_interest_rate to ZATTERA_100_PERCENT + 1" );
      prop_op.props[ "zbd_interest_rate" ].clear();
      prop_op.props[ "zbd_interest_rate" ] = fc::raw::pack_to_vector( ZATTERA_100_PERCENT + 1 );
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting new zbd_exchange_rate with ZBD / ZTR" );
      prop_op.props.erase( "zbd_interest_rate" );
      prop_op.props[ "zbd_exchange_rate" ] = fc::raw::pack_to_vector( price( ASSET( "1.000 TTR" ), ASSET( "10.000 TBD" ) ) );
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting new url with length of zero" );
      prop_op.props.erase( "zbd_exchange_rate" );
      prop_op.props[ "url" ] = fc::raw::pack_to_vector( "" );
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- failure when setting new url with non UTF-8 character" );
      prop_op.props[ "url" ].clear();
      prop_op.props[ "url" ] = fc::raw::pack_to_vector( "\xE0\x80\x80" );
      ZATTERA_REQUIRE_THROW( prop_op.validate(), fc::assert_exception );

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_witness_set_properties_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_set_properties_authorities" );

      witness_set_properties_operation op;
      op.owner = "alice";
      op.props[ "key" ] = fc::raw::pack_to_vector( generate_private_key( "key" ).get_public_key() );

      flat_set< account_name_type > auths;
      flat_set< account_name_type > expected;

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      vector< authority > key_auths;
      vector< authority > expected_keys;
      expected_keys.push_back( authority( 1, generate_private_key( "key" ).get_public_key(), 1 ) );
      op.get_required_authorities( key_auths );
      BOOST_REQUIRE( key_auths == expected_keys );

      op.props.erase( "key" );
      key_auths.clear();
      expected_keys.clear();

      op.get_required_owner_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_active_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      op.get_required_posting_authorities( auths );
      BOOST_REQUIRE( auths == expected );

      expected_keys.push_back( authority( 1, ZATTERA_NULL_ACCOUNT, 1 ) );
      op.get_required_authorities( key_auths );
      BOOST_REQUIRE( key_auths == expected_keys );

   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_witness_set_properties )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: witness_set_properties_apply" );

      ACTORS( (alice) )
      fund( "alice", 10000 );
      private_key_type signing_key = generate_private_key( "old_key" );

      witness_update_operation op;
      op.owner = "alice";
      op.url = "foo.bar";
      op.fee = ASSET( "1.000 TTR" );
      op.block_signing_key = signing_key.get_public_key();
      op.props.account_creation_fee = legacy_zattera_asset::from_asset( asset(ZATTERA_MIN_ACCOUNT_CREATION_FEE + 10, ZTR_SYMBOL) );
      op.props.maximum_block_size = ZATTERA_MIN_BLOCK_SIZE_LIMIT + 100;

      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test setting runtime parameters" );

      // Setting account_creation_fee
      const witness_object& alice_witness = db->get_witness( "alice" );
      witness_set_properties_operation prop_op;
      prop_op.owner = "alice";
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
      prop_op.props[ "account_creation_fee" ] = fc::raw::pack_to_vector( ASSET( "2.000 TTR" ) );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.props.account_creation_fee == ASSET( "2.000 TTR" ) );

      // Setting maximum_block_size
      prop_op.props.erase( "account_creation_fee" );
      prop_op.props[ "maximum_block_size" ] = fc::raw::pack_to_vector( ZATTERA_MIN_BLOCK_SIZE_LIMIT + 1 );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.props.maximum_block_size == ZATTERA_MIN_BLOCK_SIZE_LIMIT + 1 );

      // Setting zbd_interest_rate
      prop_op.props.erase( "maximim_block_size" );
      prop_op.props[ "zbd_interest_rate" ] = fc::raw::pack_to_vector( 700 );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.props.zbd_interest_rate == 700 );

      // Setting new signing_key
      private_key_type old_signing_key = signing_key;
      signing_key = generate_private_key( "new_key" );
      public_key_type alice_pub = signing_key.get_public_key();
      prop_op.props.erase( "zbd_interest_rate" );
      prop_op.props[ "new_signing_key" ] = fc::raw::pack_to_vector( alice_pub );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( old_signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.signing_key == alice_pub );

      // Setting new zbd_exchange_rate
      prop_op.props.erase( "new_signing_key" );
      prop_op.props[ "key" ].clear();
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
      prop_op.props[ "zbd_exchange_rate" ] = fc::raw::pack_to_vector( price( ASSET(" 1.000 TBD" ), ASSET( "100.000 TTR" ) ) );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.zbd_exchange_rate == price( ASSET( "1.000 TBD" ), ASSET( "100.000 TTR" ) ) );
      BOOST_REQUIRE( alice_witness.last_zbd_exchange_update == db->head_block_time() );

      // Setting new url
      prop_op.props.erase( "zbd_exchange_rate" );
      prop_op.props[ "url" ] = fc::raw::pack_to_vector( "foo.bar" );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.url == "foo.bar" );

      // Setting new extranious_property
      prop_op.props.erase( "zbd_exchange_rate" );
      prop_op.props[ "extraneous_property" ] = fc::raw::pack_to_vector( "foo" );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Testing failure when 'key' does not match witness signing key" );
      prop_op.props.erase( "extranious_property" );
      prop_op.props[ "key" ].clear();
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( old_signing_key.get_public_key() );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( old_signing_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::assert_exception );

      BOOST_TEST_MESSAGE( "--- Testing setting account subsidy limit" );
      prop_op.props[ "key" ].clear();
      prop_op.props[ "key" ] = fc::raw::pack_to_vector( signing_key.get_public_key() );
      prop_op.props[ "account_subsidy_limit" ] = fc::raw::pack_to_vector( 1000 );
      tx.clear();
      tx.operations.push_back( prop_op );
      tx.sign( signing_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      BOOST_REQUIRE( alice_witness.props.account_subsidy_limit == 1000 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( calculate_feed_publish_mean )
{
   try
   {
      resize_shared_mem( 1024 * 1024 * 32 );

      ACTORS( (alice0)(alice1)(alice2)(alice3)(alice4)(alice5)(alice6) )

      BOOST_TEST_MESSAGE( "Setup" );

      generate_blocks( 30 / ZATTERA_BLOCK_INTERVAL );

      vector< string > accounts;
      accounts.push_back( "alice0" );
      accounts.push_back( "alice1" );
      accounts.push_back( "alice2" );
      accounts.push_back( "alice3" );
      accounts.push_back( "alice4" );
      accounts.push_back( "alice5" );
      accounts.push_back( "alice6" );

      vector< private_key_type > keys;
      keys.push_back( alice0_private_key );
      keys.push_back( alice1_private_key );
      keys.push_back( alice2_private_key );
      keys.push_back( alice3_private_key );
      keys.push_back( alice4_private_key );
      keys.push_back( alice5_private_key );
      keys.push_back( alice6_private_key );

      vector< feed_publish_operation > ops;
      vector< signed_transaction > txs;

      // Upgrade accounts to witnesses
      for( int i = 0; i < 7; i++ )
      {
         transfer( ZATTERA_GENESIS_WITNESS_NAME, accounts[i], asset( 10000, ZTR_SYMBOL ) );
         witness_create( accounts[i], keys[i], "foo.bar", keys[i].get_public_key(), 1000 );

         ops.push_back( feed_publish_operation() );
         ops[i].publisher = accounts[i];

         txs.push_back( signed_transaction() );
      }

      ops[0].exchange_rate = price( asset( 1000, ZBD_SYMBOL ), asset( 100000, ZTR_SYMBOL ) );
      ops[1].exchange_rate = price( asset( 1000, ZBD_SYMBOL ), asset( 105000, ZTR_SYMBOL ) );
      ops[2].exchange_rate = price( asset( 1000, ZBD_SYMBOL ), asset(  98000, ZTR_SYMBOL ) );
      ops[3].exchange_rate = price( asset( 1000, ZBD_SYMBOL ), asset(  97000, ZTR_SYMBOL ) );
      ops[4].exchange_rate = price( asset( 1000, ZBD_SYMBOL ), asset(  99000, ZTR_SYMBOL ) );
      ops[5].exchange_rate = price( asset( 1000, ZBD_SYMBOL ), asset(  97500, ZTR_SYMBOL ) );
      ops[6].exchange_rate = price( asset( 1000, ZBD_SYMBOL ), asset( 102000, ZTR_SYMBOL ) );

      for( int i = 0; i < 7; i++ )
      {
         txs[i].set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
         txs[i].operations.push_back( ops[i] );
         txs[i].sign( keys[i], db->get_chain_id() );
         db->push_transaction( txs[i], 0 );
      }

      BOOST_TEST_MESSAGE( "Jump forward an hour" );

      generate_blocks( ZATTERA_BLOCKS_PER_HOUR ); // Jump forward 1 hour
      BOOST_TEST_MESSAGE( "Get feed history object" );
      feed_history_object feed_history = db->get_feed_history();
      BOOST_TEST_MESSAGE( "Check state" );
      BOOST_REQUIRE( feed_history.current_median_history == price( asset( 1000, ZBD_SYMBOL ), asset( 99000, ZTR_SYMBOL) ) );
      BOOST_REQUIRE( feed_history.price_history[ 0 ] == price( asset( 1000, ZBD_SYMBOL ), asset( 99000, ZTR_SYMBOL) ) );
      validate_database();

      for ( int i = 0; i < 23; i++ )
      {
         BOOST_TEST_MESSAGE( "Updating ops" );

         for( int j = 0; j < 7; j++ )
         {
            txs[j].operations.clear();
            txs[j].signatures.clear();
            ops[j].exchange_rate = price( ops[j].exchange_rate.base, asset( ops[j].exchange_rate.quote.amount + 10, ZTR_SYMBOL ) );
            txs[j].set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
            txs[j].operations.push_back( ops[j] );
            txs[j].sign( keys[j], db->get_chain_id() );
            db->push_transaction( txs[j], 0 );
         }

         BOOST_TEST_MESSAGE( "Generate Blocks" );

         generate_blocks( ZATTERA_BLOCKS_PER_HOUR  ); // Jump forward 1 hour

         BOOST_TEST_MESSAGE( "Check feed_history" );

         feed_history = db->get(feed_history_id_type());
         BOOST_REQUIRE( feed_history.current_median_history == feed_history.price_history[ ( i + 1 ) / 2 ] );
         BOOST_REQUIRE( feed_history.price_history[ i + 1 ] == ops[4].exchange_rate );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_SUITE_END()
#endif

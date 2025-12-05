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

BOOST_AUTO_TEST_CASE( validate_convert )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: convert_validate" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_convert_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: convert_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      convert( "alice", ASSET( "2.500 TTR" ) );

      validate_database();

      convert_operation op;
      op.owner = "alice";
      op.amount = ASSET( "2.500 TBD" );

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

      BOOST_TEST_MESSAGE( "--- Test success with owner signature" );
      tx.signatures.clear();
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( apply_convert )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: convert_apply" );
      ACTORS( (alice)(bob) );
      fund( "alice", 10000 );
      fund( "bob" , 10000 );

      convert_operation op;
      signed_transaction tx;
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );

      const auto& convert_request_idx = db->get_index< convert_request_index >().indices().get< by_owner >();

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      convert( "alice", ASSET( "2.500 TTR" ) );
      convert( "bob", ASSET( "7.000 TTR" ) );

      const auto& new_alice = db->get_account( "alice" );
      const auto& new_bob = db->get_account( "bob" );

      BOOST_TEST_MESSAGE( "--- Test failure when account does not have the required TTR" );
      op.owner = "bob";
      op.amount = ASSET( "5.000 TTR" );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "3.000 TTR" ).amount.value );
      BOOST_REQUIRE( new_bob.zbd_balance.amount.value == ASSET( "7.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when account does not have the required TBD" );
      op.owner = "alice";
      op.amount = ASSET( "5.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( new_alice.balance.amount.value == ASSET( "7.500 TTR" ).amount.value );
      BOOST_REQUIRE( new_alice.zbd_balance.amount.value == ASSET( "2.500 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when account does not exist" );
      op.owner = "sam";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test success converting ZBD to TTR" );
      op.owner = "bob";
      op.amount = ASSET( "3.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "3.000 TTR" ).amount.value );
      BOOST_REQUIRE( new_bob.zbd_balance.amount.value == ASSET( "4.000 TBD" ).amount.value );

      auto convert_request = convert_request_idx.find( std::make_tuple( op.owner, op.requestid ) );
      BOOST_REQUIRE( convert_request != convert_request_idx.end() );
      BOOST_REQUIRE( convert_request->owner == op.owner );
      BOOST_REQUIRE( convert_request->requestid == op.requestid );
      BOOST_REQUIRE( convert_request->amount.amount.value == op.amount.amount.value );
      //BOOST_REQUIRE( convert_request->premium == 100000 );
      BOOST_REQUIRE( convert_request->conversion_date == db->head_block_time() + ZATTERA_CONVERSION_DELAY );

      BOOST_TEST_MESSAGE( "--- Test failure from repeated id" );
      op.amount = ASSET( "2.000 TTR" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( new_bob.balance.amount.value == ASSET( "3.000 TTR" ).amount.value );
      BOOST_REQUIRE( new_bob.zbd_balance.amount.value == ASSET( "4.000 TBD" ).amount.value );

      convert_request = convert_request_idx.find( std::make_tuple( op.owner, op.requestid ) );
      BOOST_REQUIRE( convert_request != convert_request_idx.end() );
      BOOST_REQUIRE( convert_request->owner == op.owner );
      BOOST_REQUIRE( convert_request->requestid == op.requestid );
      BOOST_REQUIRE( convert_request->amount.amount.value == ASSET( "3.000 TBD" ).amount.value );
      //BOOST_REQUIRE( convert_request->premium == 100000 );
      BOOST_REQUIRE( convert_request->conversion_date == db->head_block_time() + ZATTERA_CONVERSION_DELAY );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_convert_balance_in_fixture )
{
   // This actually tests the convert() method of the database fixture can't result in negative
   //   balances, see issue #1825
   try
   {
      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );
      ACTORS( (dany) )

      fund( "dany", 5000 );
      ZATTERA_REQUIRE_THROW( convert( "dany", ASSET( "5000.000 TTR" ) ), fc::exception );
   }
   FC_LOG_AND_RETHROW()

}

BOOST_AUTO_TEST_CASE( validate_limit_order_creation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_create_validate" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_limit_order_creation_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_create_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      limit_order_create_operation op;
      op.owner = "alice";
      op.amount_to_sell = ASSET( "1.000 TTR" );
      op.min_to_receive = ASSET( "1.000 TBD" );
      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );

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

BOOST_AUTO_TEST_CASE( apply_limit_order_creation )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_create_apply" );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      ACTORS( (alice)(bob) )
      fund( "alice", 1000000 );
      fund( "bob", 1000000 );
      convert( "bob", ASSET("1000.000 TTR" ) );

      const auto& limit_order_idx = db->get_index< limit_order_index >().indices().get< by_account >();

      BOOST_TEST_MESSAGE( "--- Test failure when account does not have required funds" );
      limit_order_create_operation op;
      signed_transaction tx;

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "10.000 TTR" );
      op.min_to_receive = ASSET( "10.000 TBD" );
      op.fill_or_kill = false;
      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "1000.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when amount to receive is 0" );

      op.owner = "alice";
      op.min_to_receive = ASSET( "0.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "1000.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when amount to sell is 0" );

      op.amount_to_sell = ASSET( "0.000 TTR" );
      op.min_to_receive = ASSET( "10.000 TBD" ) ;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "1000.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when expiration is too long" );
      op.amount_to_sell = ASSET( "10.000 TTR" );
      op.min_to_receive = ASSET( "15.000 TBD" );
      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION + 1 );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test success creating limit order that will not be filled" );

      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == op.amount_to_sell.amount );
      BOOST_REQUIRE( limit_order->sell_price == price( op.amount_to_sell / op.min_to_receive ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure creating limit order with duplicate id" );

      op.amount_to_sell = ASSET( "20.000 TTR" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 10000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "10.000 TTR" ), op.min_to_receive ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test sucess killing an order that will not be filled" );

      op.orderid = 2;
      op.fill_or_kill = true;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test having a partial match to limit order" );
      // Alice has order for 15 ZBD at a price of 2:3
      // Fill 5 ZTR for 7.5 ZBD

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "7.500 TBD" );
      op.min_to_receive = ASSET( "5.000 TTR" );
      op.fill_or_kill = false;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto recent_ops = get_last_operations( 1 );
      auto fill_order_op = recent_ops[0].get< fill_order_operation >();

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 5000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "10.000 TTR" ), ASSET( "15.000 TBD" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "7.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "5.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "992.500 TBD" ).amount.value );
      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == ASSET( "5.000 TTR").amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == ASSET( "7.500 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order fully, but the new order partially" );

      op.amount_to_sell = ASSET( "15.000 TBD" );
      op.min_to_receive = ASSET( "10.000 TTR" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 1 );
      BOOST_REQUIRE( limit_order->for_sale.value == 7500 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "15.000 TBD" ), ASSET( "10.000 TTR" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "15.000 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "10.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "977.500 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order and new order fully" );

      op.owner = "alice";
      op.orderid = 3;
      op.amount_to_sell = ASSET( "5.000 TTR" );
      op.min_to_receive = ASSET( "7.500 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 3 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "985.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "22.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "15.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "977.500 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is better." );

      op.owner = "alice";
      op.orderid = 4;
      op.amount_to_sell = ASSET( "10.000 TTR" );
      op.min_to_receive = ASSET( "11.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = ASSET( "12.000 TBD" );
      op.min_to_receive = ASSET( "10.000 TTR" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 4 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "alice", 4 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 4 );
      BOOST_REQUIRE( limit_order->for_sale.value == 1000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "12.000 TBD" ), ASSET( "10.000 TTR" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "975.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "33.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "25.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "965.500 TBD" ).amount.value );
      validate_database();

      limit_order_cancel_operation can;
      can.owner = "bob";
      can.orderid = 4;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( can );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is worse." );

      //auto gpo = db->get_dynamic_global_properties();
      //auto start_zbd = gpo.current_zbd_supply;

      op.owner = "alice";
      op.orderid = 5;
      op.amount_to_sell = ASSET( "20.000 TTR" );
      op.min_to_receive = ASSET( "22.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 5;
      op.amount_to_sell = ASSET( "12.000 TBD" );
      op.min_to_receive = ASSET( "10.000 TTR" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 5 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "bob", 5 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == 5 );
      BOOST_REQUIRE( limit_order->for_sale.value == 9091 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "20.000 TTR" ), ASSET( "22.000 TBD" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "955.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "45.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "35.909 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "954.500 TBD" ).amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_limit_order_creation2_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_create2_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      limit_order_create2_operation op;
      op.owner = "alice";
      op.amount_to_sell = ASSET( "1.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) );
      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );

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

BOOST_AUTO_TEST_CASE( apply_limit_order_creation2 )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_create2_apply" );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      ACTORS( (alice)(bob) )
      fund( "alice", 1000000 );
      fund( "bob", 1000000 );
      convert( "bob", ASSET("1000.000 TTR" ) );

      const auto& limit_order_idx = db->get_index< limit_order_index >().indices().get< by_account >();

      BOOST_TEST_MESSAGE( "--- Test failure when account does not have required funds" );
      limit_order_create2_operation op;
      signed_transaction tx;

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "10.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) );
      op.fill_or_kill = false;
      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "0.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "1000.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when price is 0" );

      /// First check validation on price constructor level:
      {
        price broken_price;
        /// Invalid base value
        ZATTERA_REQUIRE_THROW(broken_price=price(ASSET("0.000 TTR"), ASSET("1.000 TBD")),
          fc::exception);
        /// Invalid quote value
        ZATTERA_REQUIRE_THROW(broken_price=price(ASSET("1.000 TTR"), ASSET("0.000 TBD")),
          fc::exception);
        /// Invalid symbol (same in base & quote)
        ZATTERA_REQUIRE_THROW(broken_price=price(ASSET("1.000 TTR"), ASSET("0.000 TTR")),
          fc::exception);
      }

      op.owner = "alice";
      /** Here intentionally price has assigned its members directly, to skip validation
          inside price constructor, and force the one performed at tx push.
      */
      op.exchange_rate = price();
      op.exchange_rate.base = ASSET("0.000 TTR");
      op.exchange_rate.quote = ASSET("1.000 TBD");

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "1000.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when amount to sell is 0" );

      op.amount_to_sell = ASSET( "0.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "1000.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure when expiration is too long" );
      op.amount_to_sell = ASSET( "10.000 TTR" );
      op.exchange_rate = price( ASSET( "2.000 TTR" ), ASSET( "3.000 TBD" ) );
      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION + 1 );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test success creating limit order that will not be filled" );

      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == op.amount_to_sell.amount );
      BOOST_REQUIRE( limit_order->sell_price == op.exchange_rate );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test failure creating limit order with duplicate id" );

      op.amount_to_sell = ASSET( "20.000 TTR" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      limit_order = limit_order_idx.find( std::make_tuple( "alice", op.orderid ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == op.owner );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 10000 );
      BOOST_REQUIRE( limit_order->sell_price == op.exchange_rate );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test sucess killing an order that will not be filled" );

      op.orderid = 2;
      op.fill_or_kill = true;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test having a partial match to limit order" );
      // Alice has order for 15 ZBD at a price of 2:3
      // Fill 5 ZTR for 7.5 ZBD

      op.owner = "bob";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "7.500 TBD" );
      op.exchange_rate = price( ASSET( "3.000 TBD" ), ASSET( "2.000 TTR" ) );
      op.fill_or_kill = false;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto recent_ops = get_last_operations( 1 );
      auto fill_order_op = recent_ops[0].get< fill_order_operation >();

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == op.orderid );
      BOOST_REQUIRE( limit_order->for_sale == 5000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "2.000 TTR" ), ASSET( "3.000 TBD" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", op.orderid ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "7.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "5.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "992.500 TBD" ).amount.value );
      BOOST_REQUIRE( fill_order_op.open_owner == "alice" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.open_pays.amount.value == ASSET( "5.000 TTR").amount.value );
      BOOST_REQUIRE( fill_order_op.current_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.current_pays.amount.value == ASSET( "7.500 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order fully, but the new order partially" );

      op.amount_to_sell = ASSET( "15.000 TBD" );
      op.exchange_rate = price( ASSET( "3.000 TBD" ), ASSET( "2.000 TTR" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 1 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 1 );
      BOOST_REQUIRE( limit_order->for_sale.value == 7500 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "3.000 TBD" ), ASSET( "2.000 TTR" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "990.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "15.000 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "10.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "977.500 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling an existing order and new order fully" );

      op.owner = "alice";
      op.orderid = 3;
      op.amount_to_sell = ASSET( "5.000 TTR" );
      op.exchange_rate = price( ASSET( "2.000 TTR" ), ASSET( "3.000 TBD" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 3 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "bob", 1 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "985.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "22.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "15.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "977.500 TBD" ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is better." );

      op.owner = "alice";
      op.orderid = 4;
      op.amount_to_sell = ASSET( "10.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "1.100 TBD" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 4;
      op.amount_to_sell = ASSET( "12.000 TBD" );
      op.exchange_rate = price( ASSET( "1.200 TBD" ), ASSET( "1.000 TTR" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 4 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "alice", 4 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 4 );
      BOOST_REQUIRE( limit_order->for_sale.value == 1000 );
      BOOST_REQUIRE( limit_order->sell_price == op.exchange_rate );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "975.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "33.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "25.000 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "965.500 TBD" ).amount.value );
      validate_database();

      limit_order_cancel_operation can;
      can.owner = "bob";
      can.orderid = 4;
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( can );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "--- Test filling limit order with better order when partial order is worse." );

      //auto gpo = db->get_dynamic_global_properties();
      //auto start_zbd = gpo.current_zbd_supply;


      op.owner = "alice";
      op.orderid = 5;
      op.amount_to_sell = ASSET( "20.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "1.100 TBD" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "bob";
      op.orderid = 5;
      op.amount_to_sell = ASSET( "12.000 TBD" );
      op.exchange_rate = price( ASSET( "1.200 TBD" ), ASSET( "1.000 TTR" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 5 ) );
      BOOST_REQUIRE( limit_order != limit_order_idx.end() );
      BOOST_REQUIRE( limit_order_idx.find(std::make_tuple( "bob", 5 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == 5 );
      BOOST_REQUIRE( limit_order->for_sale.value == 9091 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "1.000 TTR" ), ASSET( "1.100 TBD" ) ) );
      BOOST_REQUIRE( limit_order->get_market() == std::make_pair( ZBD_SYMBOL, ZTR_SYMBOL ) );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "955.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "45.500 TBD" ).amount.value );
      BOOST_REQUIRE( bob.balance.amount.value == ASSET( "35.909 TTR" ).amount.value );
      BOOST_REQUIRE( bob.zbd_balance.amount.value == ASSET( "954.500 TBD" ).amount.value );

      BOOST_TEST_MESSAGE( "--- Test filling best order with multiple matches." );
      ACTORS( (sam)(dave) )
      fund( "sam", 1000000 );
      fund( "dave", 1000000 );
      convert( "dave", ASSET("1000.000 TTR" ) );

      op.owner = "bob";
      op.orderid = 6;
      op.amount_to_sell = ASSET( "20.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "sam";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "20.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "0.500 TBD" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "alice";
      op.orderid = 6;
      op.amount_to_sell = ASSET( "20.000 TTR" );
      op.exchange_rate = price( ASSET( "1.000 TTR" ), ASSET( "2.000 TBD" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      op.owner = "dave";
      op.orderid = 1;
      op.amount_to_sell = ASSET( "25.000 TBD" );
      op.exchange_rate = price( ASSET( "1.000 TBD" ), ASSET( "0.010 TTR" ) );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( dave_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      recent_ops = get_last_operations( 3 );
      fill_order_op = recent_ops[2].get< fill_order_operation >();
      BOOST_REQUIRE( fill_order_op.open_owner == "sam" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.open_pays == ASSET( "20.000 TTR") );
      BOOST_REQUIRE( fill_order_op.current_owner == "dave" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.current_pays == ASSET( "10.000 TBD" ) );

      fill_order_op = recent_ops[0].get< fill_order_operation >();
      BOOST_REQUIRE( fill_order_op.open_owner == "bob" );
      BOOST_REQUIRE( fill_order_op.open_orderid == 6 );
      BOOST_REQUIRE( fill_order_op.open_pays == ASSET( "15.000 TTR") );
      BOOST_REQUIRE( fill_order_op.current_owner == "dave" );
      BOOST_REQUIRE( fill_order_op.current_orderid == 1 );
      BOOST_REQUIRE( fill_order_op.current_pays == ASSET( "15.000 TBD" ) );

      limit_order = limit_order_idx.find( std::make_tuple( "bob", 6 ) );
      BOOST_REQUIRE( limit_order->seller == "bob" );
      BOOST_REQUIRE( limit_order->orderid == 6 );
      BOOST_REQUIRE( limit_order->for_sale.value == 5000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "1.000 TTR" ), ASSET( "1.000 TBD" ) ) );

      limit_order = limit_order_idx.find( std::make_tuple( "alice", 6 ) );
      BOOST_REQUIRE( limit_order->seller == "alice" );
      BOOST_REQUIRE( limit_order->orderid == 6 );
      BOOST_REQUIRE( limit_order->for_sale.value == 20000 );
      BOOST_REQUIRE( limit_order->sell_price == price( ASSET( "1.000 TTR" ), ASSET( "2.000 TBD" ) ) );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( validate_limit_order_cancel )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_cancel_validate" );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( check_limit_order_cancel_authorities )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_cancel_authorities" );

      ACTORS( (alice)(bob) )
      fund( "alice", 10000 );

      limit_order_create_operation c;
      c.owner = "alice";
      c.orderid = 1;
      c.amount_to_sell = ASSET( "1.000 TTR" );
      c.min_to_receive = ASSET( "1.000 TBD" );
      c.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );

      signed_transaction tx;
      tx.operations.push_back( c );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      limit_order_cancel_operation op;
      op.owner = "alice";
      op.orderid = 1;

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );

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

BOOST_AUTO_TEST_CASE( apply_limit_order_cancel )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: limit_order_cancel_apply" );

      ACTORS( (alice) )
      fund( "alice", 10000 );

      const auto& limit_order_idx = db->get_index< limit_order_index >().indices().get< by_account >();

      BOOST_TEST_MESSAGE( "--- Test cancel non-existent order" );

      limit_order_cancel_operation op;
      signed_transaction tx;

      op.owner = "alice";
      op.orderid = 5;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      ZATTERA_REQUIRE_THROW( db->push_transaction( tx, 0 ), fc::exception );

      BOOST_TEST_MESSAGE( "--- Test cancel order" );

      limit_order_create_operation create;
      create.owner = "alice";
      create.orderid = 5;
      create.amount_to_sell = ASSET( "5.000 TTR" );
      create.min_to_receive = ASSET( "7.500 TBD" );
      create.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( create );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 5 ) ) != limit_order_idx.end() );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( op );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( limit_order_idx.find( std::make_tuple( "alice", 5 ) ) == limit_order_idx.end() );
      BOOST_REQUIRE( alice.balance.amount.value == ASSET( "10.000 TTR" ).amount.value );
      BOOST_REQUIRE( alice.zbd_balance.amount.value == ASSET( "0.000 TBD" ).amount.value );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( process_convert_delay )
{
   try
   {
      ACTORS( (alice) )
      generate_block();
      vest( "alice", ASSET( "10.000 TTR" ) );
      fund( "alice", ASSET( "25.000 TBD" ) );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.250 TTR" ) ) );

      convert_operation op;
      signed_transaction tx;

      auto start_balance = ASSET( "25.000 TBD" );

      BOOST_TEST_MESSAGE( "Setup conversion to TTR" );
      tx.operations.clear();
      tx.signatures.clear();
      op.owner = "alice";
      op.amount = asset( 2000, ZBD_SYMBOL );
      op.requestid = 2;
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating Blocks up to conversion block" );
      generate_blocks( db->head_block_time() + ZATTERA_CONVERSION_DELAY - fc::seconds( ZATTERA_BLOCK_INTERVAL / 2 ), true );

      BOOST_TEST_MESSAGE( "Verify conversion is not applied" );
      const auto& alice_2 = db->get_account( "alice" );
      const auto& convert_request_idx = db->get_index< convert_request_index >().indices().get< by_owner >();
      auto convert_request = convert_request_idx.find( std::make_tuple( "alice", 2 ) );

      BOOST_REQUIRE( convert_request != convert_request_idx.end() );
      BOOST_REQUIRE( alice_2.balance.amount.value == 0 );
      BOOST_REQUIRE( alice_2.zbd_balance.amount.value == ( start_balance - op.amount ).amount.value );
      validate_database();

      BOOST_TEST_MESSAGE( "Generate one more block" );
      generate_block();

      BOOST_TEST_MESSAGE( "Verify conversion applied" );
      const auto& alice_3 = db->get_account( "alice" );
      auto vop = get_last_operations( 1 )[0].get< fill_convert_request_operation >();

      convert_request = convert_request_idx.find( std::make_tuple( "alice", 2 ) );
      BOOST_REQUIRE( convert_request == convert_request_idx.end() );
      BOOST_REQUIRE( alice_3.balance.amount.value == 2500 );
      BOOST_REQUIRE( alice_3.zbd_balance.amount.value == ( start_balance - op.amount ).amount.value );
      BOOST_REQUIRE( vop.owner == "alice" );
      BOOST_REQUIRE( vop.requestid == 2 );
      BOOST_REQUIRE( vop.amount_in.amount.value == ASSET( "2.000 TBD" ).amount.value );
      BOOST_REQUIRE( vop.amount_out.amount.value == ASSET( "2.500 TTR" ).amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW();
}

BOOST_AUTO_TEST_CASE( calculate_zbd_interest )
{
   try
   {
      ACTORS( (alice)(bob) )
      generate_block();
      vest( "alice", ASSET( "10.000 TTR" ) );
      vest( "bob", ASSET( "10.000 TTR" ) );

      set_price_feed( price( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) ) );

      BOOST_TEST_MESSAGE( "Testing interest over smallest interest period" );

      convert_operation op;
      signed_transaction tx;

      fund( "alice", ASSET( "31.903 TBD" ) );

      auto start_time = db->get_account( "alice" ).zbd_seconds_last_update;
      auto alice_zbd = db->get_account( "alice" ).zbd_balance;

      generate_blocks( db->head_block_time() + fc::seconds( ZATTERA_ZBD_INTEREST_COMPOUND_INTERVAL_SEC ), true );

      transfer_operation transfer;
      transfer.to = "bob";
      transfer.from = "alice";
      transfer.amount = ASSET( "1.000 TBD" );
      tx.operations.clear();
      tx.signatures.clear();
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( transfer );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      auto gpo = db->get_dynamic_global_properties();
      auto interest_op = get_last_operations( 1 )[0].get< interest_operation >();

      BOOST_REQUIRE( gpo.zbd_interest_rate > 0 );
      BOOST_REQUIRE( static_cast<uint64_t>(db->get_account( "alice" ).zbd_balance.amount.value) == alice_zbd.amount.value - ASSET( "1.000 TBD" ).amount.value + ( ( ( ( uint128_t( alice_zbd.amount.value ) * ( db->head_block_time() - start_time ).to_seconds() ) / ZATTERA_SECONDS_PER_YEAR ) * gpo.zbd_interest_rate ) / ZATTERA_100_PERCENT ).to_uint64() );
      BOOST_REQUIRE( interest_op.owner == "alice" );
      BOOST_REQUIRE( interest_op.interest.amount.value == db->get_account( "alice" ).zbd_balance.amount.value - ( alice_zbd.amount.value - ASSET( "1.000 TBD" ).amount.value ) );
      validate_database();

      BOOST_TEST_MESSAGE( "Testing interest under interest period" );

      start_time = db->get_account( "alice" ).zbd_seconds_last_update;
      alice_zbd = db->get_account( "alice" ).zbd_balance;

      generate_blocks( db->head_block_time() + fc::seconds( ZATTERA_ZBD_INTEREST_COMPOUND_INTERVAL_SEC / 2 ), true );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance.amount.value == alice_zbd.amount.value - ASSET( "1.000 TBD" ).amount.value );
      validate_database();

      auto alice_coindays = uint128_t( alice_zbd.amount.value ) * ( db->head_block_time() - start_time ).to_seconds();
      alice_zbd = db->get_account( "alice" ).zbd_balance;
      start_time = db->get_account( "alice" ).zbd_seconds_last_update;

      BOOST_TEST_MESSAGE( "Testing longer interest period" );

      generate_blocks( db->head_block_time() + fc::seconds( ( ZATTERA_ZBD_INTEREST_COMPOUND_INTERVAL_SEC * 7 ) / 3 ), true );

      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( transfer );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_REQUIRE( static_cast<uint64_t>(db->get_account( "alice" ).zbd_balance.amount.value) == alice_zbd.amount.value - ASSET( "1.000 TBD" ).amount.value + ( ( ( ( uint128_t( alice_zbd.amount.value ) * ( db->head_block_time() - start_time ).to_seconds() + alice_coindays ) / ZATTERA_SECONDS_PER_YEAR ) * gpo.zbd_interest_rate ) / ZATTERA_100_PERCENT ).to_uint64() );
      validate_database();
   }
   FC_LOG_AND_RETHROW();
}

#ifndef DEBUG

BOOST_AUTO_TEST_CASE( maintain_zbd_stability )
{
   try
   {
      resize_shared_mem( 1024 * 1024 * 512 ); // Due to number of blocks in the test, it requires a large file. (64 MB)

      auto debug_key = "5JdouSvkK75TKWrJixYufQgePT21V7BAVWbNUWt3ktqhPmy8Z78"; //get_dev_key debug node

      ACTORS( (alice)(bob)(sam)(dave)(greg) );

      fund( "alice", 10000 );
      fund( "bob", 10000 );

      vest( "alice", 10000 );
      vest( "bob", 10000 );

      auto exchange_rate = price( ASSET( "1.000 TBD" ), ASSET( "10.000 TTR" ) );
      set_price_feed( exchange_rate );

      BOOST_REQUIRE( db->get_dynamic_global_properties().zbd_print_rate == ZATTERA_100_PERCENT );

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

      vote_operation vote;
      vote.voter = "bob";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = ZATTERA_100_PERCENT;

      tx.operations.clear();
      tx.signatures.clear();

      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating blocks up to comment payout" );

      db_plugin->debug_generate_blocks_until( debug_key, fc::time_point_sec( db->get_comment( comment.author, comment.permlink ).cashout_time.sec_since_epoch() - 2 * ZATTERA_BLOCK_INTERVAL ), true, database::skip_witness_signature );

      auto& gpo = db->get_dynamic_global_properties();

      BOOST_TEST_MESSAGE( "Changing sam and gpo to set up market cap conditions" );

      asset zbd_balance = asset( ( gpo.virtual_supply.amount * ( ZATTERA_ZBD_STOP_PERCENT + 30 ) ) / ZATTERA_100_PERCENT, ZTR_SYMBOL ) * exchange_rate;
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( "sam" ), [&]( account_object& a )
         {
            a.zbd_balance = zbd_balance;
         });
      }, database::skip_witness_signature );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_zbd_supply = zbd_balance;
            gpo.virtual_supply = gpo.virtual_supply + zbd_balance * exchange_rate;
         });
      }, database::skip_witness_signature );

      validate_database();

      db_plugin->debug_generate_blocks( debug_key, 1, database::skip_witness_signature );

      auto comment_reward = ( gpo.total_reward_fund_ztr.amount + 2000 ) - ( ( gpo.total_reward_fund_ztr.amount + 2000 ) * 25 * ZATTERA_1_PERCENT ) / ZATTERA_100_PERCENT ;
      comment_reward /= 2;
      auto zbd_reward = ( comment_reward * gpo.zbd_print_rate ) / ZATTERA_100_PERCENT;
      auto alice_zbd = db->get_account( "alice" ).zbd_balance + db->get_account( "alice" ).reward_zbd_balance + asset( zbd_reward, ZTR_SYMBOL ) * exchange_rate;
      auto alice_ztr = db->get_account( "alice" ).balance + db->get_account( "alice" ).reward_ztr_balance ;

      BOOST_TEST_MESSAGE( "Checking printing ZBD has slowed" );
      BOOST_REQUIRE( db->get_dynamic_global_properties().zbd_print_rate < ZATTERA_100_PERCENT );

      BOOST_TEST_MESSAGE( "Pay out comment and check rewards are paid as ZTR" );
      db_plugin->debug_generate_blocks( debug_key, 1, database::skip_witness_signature );

      validate_database();

      BOOST_REQUIRE( db->get_account( "alice" ).zbd_balance + db->get_account( "alice" ).reward_zbd_balance == alice_zbd );
      BOOST_REQUIRE( db->get_account( "alice" ).balance + db->get_account( "alice" ).reward_ztr_balance > alice_ztr );

      BOOST_TEST_MESSAGE( "Letting percent market cap fall to 2% to verify printing of ZBD turns back on" );

      // Get close to 1.5% for printing ZBD to start again, but not all the way
      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_account( "sam" ), [&]( account_object& a )
         {
            a.zbd_balance = asset( ( 194 * zbd_balance.amount ) / 500, ZBD_SYMBOL );
         });
      }, database::skip_witness_signature );

      db_plugin->debug_update( [=]( database& db )
      {
         db.modify( db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
         {
            gpo.current_zbd_supply = alice_zbd + asset( ( 194 * zbd_balance.amount ) / 500, ZBD_SYMBOL );
         });
      }, database::skip_witness_signature );

      db_plugin->debug_generate_blocks( debug_key, 1, database::skip_witness_signature );
      validate_database();

      BOOST_REQUIRE( db->get_dynamic_global_properties().zbd_print_rate < ZATTERA_100_PERCENT );

      auto last_print_rate = db->get_dynamic_global_properties().zbd_print_rate;

      // Keep producing blocks until printing ZBD is back
      while( ( db->get_dynamic_global_properties().current_zbd_supply * exchange_rate ).amount >= ( db->get_dynamic_global_properties().virtual_supply.amount * ZATTERA_ZBD_START_PERCENT ) / ZATTERA_100_PERCENT )
      {
         auto& gpo = db->get_dynamic_global_properties();
         BOOST_REQUIRE( gpo.zbd_print_rate >= last_print_rate );
         last_print_rate = gpo.zbd_print_rate;
         db_plugin->debug_generate_blocks( debug_key, 1, database::skip_witness_signature );
         validate_database();
      }

      validate_database();

      BOOST_REQUIRE( db->get_dynamic_global_properties().zbd_print_rate == ZATTERA_100_PERCENT );
   }
   FC_LOG_AND_RETHROW()
}

#endif

BOOST_AUTO_TEST_CASE( enforce_zbd_price_feed_limit )
{
   try
   {
      ACTORS( (alice) );
      generate_block();
      vest( "alice", ASSET( "10.000 TTR" ) );

      price exchange_rate( ASSET( "1.000 TBD" ), ASSET( "1.000 TTR" ) );
      set_price_feed( exchange_rate );

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

      signed_transaction tx;
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->get_comment( "alice", string( "test" ) ).cashout_time, true );

      BOOST_TEST_MESSAGE( "Setting ZBD percent to greater than 10% market cap." );

      db->skip_price_feed_limit_check = false;
      const auto& gpo = db->get_dynamic_global_properties();
      auto new_exchange_rate = price( gpo.current_zbd_supply, asset( ( ZATTERA_100_PERCENT ) * gpo.current_supply.amount, ZTR_SYMBOL ) );
      set_price_feed( new_exchange_rate );
      set_price_feed( new_exchange_rate );

      BOOST_REQUIRE( db->get_feed_history().current_median_history > new_exchange_rate && db->get_feed_history().current_median_history < exchange_rate );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

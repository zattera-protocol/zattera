#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <zattera/chain/account_object.hpp>
#include <zattera/chain/comment_object.hpp>
#include <zattera/protocol/zattera_operations.hpp>

#include <zattera/plugins/market_history/market_history_plugin.hpp>

#include "../../fixtures/database_fixture.hpp"

using namespace zattera::chain;
using namespace zattera::protocol;

BOOST_FIXTURE_TEST_SUITE( market_history, database_fixture )

BOOST_AUTO_TEST_CASE( track_market_history_and_buckets )
{
   using namespace zattera::plugins::market_history;

   try
   {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;
      for( int i=1; i<argc; i++ )
      {
         const std::string arg = argv[i];
         if( arg == "--record-assert-trip" )
            fc::enable_record_assert_trip = true;
         if( arg == "--show-test-names" )
            std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
      }

      appbase::app().register_plugin< market_history_plugin >();
      db_plugin = &appbase::app().register_plugin< zattera::plugins::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      db_plugin->logging = false;
      appbase::app().initialize<
         zattera::plugins::market_history::market_history_plugin,
         zattera::plugins::debug_node::debug_node_plugin
      >( argc, argv );

      db = &appbase::app().get_plugin< zattera::plugins::chain::chain_plugin >().db();
      BOOST_REQUIRE( db );

      open_database();

      generate_block();
      db->set_hardfork( ZATTERA_NUM_HARDFORKS );
      generate_block();

      vest( "genesis", 10000 );

      // Fill up the rest of the required miners
      for( int i = ZATTERA_NUM_GENESIS_WITNESSES; i < ZATTERA_MAX_WITNESSES; i++ )
      {
         account_create( ZATTERA_GENESIS_WITNESS_NAME + fc::to_string( i ), init_account_pub_key );
         fund( ZATTERA_GENESIS_WITNESS_NAME + fc::to_string( i ), 10000 );
         witness_create( ZATTERA_GENESIS_WITNESS_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key, 0 );
      }

      validate_database();

      ACTORS( (alice)(bob)(sam) );
      generate_block();

      fund( "alice", ASSET( "1000.000 TTR" ) );
      fund( "alice", ASSET( "1000.000 TBD" ) );
      fund( "bob", ASSET( "1000.000 TTR" ) );
      fund( "sam", ASSET( "1000.000 TTR" ) );

      const auto& bucket_idx = db->get_index< bucket_index >().indices().get< by_bucket >();
      const auto& order_hist_idx = db->get_index< order_history_index >().indices().get< by_id >();

      BOOST_REQUIRE( bucket_idx.begin() == bucket_idx.end() );
      BOOST_REQUIRE( order_hist_idx.begin() == order_hist_idx.end() );
      validate_database();

      signed_transaction tx;

      auto fill_order_a_time = db->head_block_time();
      auto time_a = fc::time_point_sec( ( fill_order_a_time.sec_since_epoch() / 15 ) * 15 );

      limit_order_create_operation op;
      op.owner = "alice";
      op.amount_to_sell = ASSET( "1.000 TBD" );
      op.min_to_receive = ASSET( "2.000 TTR" );
      op.expiration = db->head_block_time() + fc::seconds( ZATTERA_MAX_LIMIT_ORDER_EXPIRATION );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx,  0 );

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "bob";
      op.amount_to_sell = ASSET( "1.500 TTR" );
      op.min_to_receive = ASSET( "0.750 TBD" );
      tx.operations.push_back( op );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + ( 60 * 90 ) );

      auto fill_order_b_time = db->head_block_time();

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "sam";
      op.amount_to_sell = ASSET( "1.000 TTR" );
      op.min_to_receive = ASSET( "0.500 TBD" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( sam_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      generate_blocks( db->head_block_time() + 60 );

      auto fill_order_c_time = db->head_block_time();

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "alice";
      op.amount_to_sell = ASSET( "0.500 TBD" );
      op.min_to_receive = ASSET( "0.900 TTR" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      op.owner = "bob";
      op.amount_to_sell = ASSET( "0.450 TTR" );
      op.min_to_receive = ASSET( "0.250 TBD" );
      tx.operations.push_back( op );
      tx.set_expiration( db->head_block_time() + ZATTERA_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db->get_chain_id() );
      db->push_transaction( tx, 0 );
      validate_database();

      auto bucket = bucket_idx.begin();

      BOOST_REQUIRE( bucket->seconds == 15 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "1.500 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "1.500 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.750 TBD" ).amount );

      bucket++;

      BOOST_REQUIRE( bucket->seconds == 15 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "0.500 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "0.500 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.250 TBD" ).amount );

      bucket++;

      BOOST_REQUIRE( bucket->seconds == 15 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) + 60 );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "0.450 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "0.450 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "0.950 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.500 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 60 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "1.500 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "1.500 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.750 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 60 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "0.500 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "0.500 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.250 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 60 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) + 60 );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "0.450 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "0.450 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "0.950 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.500 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 300 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "1.500 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "1.500 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.750 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 300 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 90 ) );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "0.450 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "0.450 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "1.450 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.750 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 3600 );
      BOOST_REQUIRE( bucket->open == time_a );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "1.500 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "1.500 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.750 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 3600 );
      BOOST_REQUIRE( bucket->open == time_a + ( 60 * 60 ) );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "0.450 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "0.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "0.450 TTR").amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "1.450 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "0.750 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket->seconds == 86400 );
      BOOST_REQUIRE( bucket->open == ZATTERA_GENESIS_TIME );
      BOOST_REQUIRE( bucket->ztr.high == ASSET( "0.450 TTR " ).amount );
      BOOST_REQUIRE( bucket->ztr.low == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.open == ASSET( "1.500 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.close == ASSET( "0.450 TTR" ).amount );
      BOOST_REQUIRE( bucket->ztr.volume == ASSET( "2.950 TTR" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.high == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.low == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.open == ASSET( "0.750 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.close == ASSET( "0.250 TBD" ).amount );
      BOOST_REQUIRE( bucket->non_ztr.volume == ASSET( "1.500 TBD" ).amount );
      bucket++;

      BOOST_REQUIRE( bucket == bucket_idx.end() );

      auto order = order_hist_idx.begin();

      BOOST_REQUIRE( order->time == fill_order_a_time );
      BOOST_REQUIRE( order->op.current_owner == "bob" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == ASSET( "1.500 TTR" ) );
      BOOST_REQUIRE( order->op.open_owner == "alice" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == ASSET( "0.750 TBD" ) );
      order++;

      BOOST_REQUIRE( order->time == fill_order_b_time );
      BOOST_REQUIRE( order->op.current_owner == "sam" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == ASSET( "0.500 TTR" ) );
      BOOST_REQUIRE( order->op.open_owner == "alice" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == ASSET( "0.250 TBD" ) );
      order++;

      BOOST_REQUIRE( order->time == fill_order_c_time );
      BOOST_REQUIRE( order->op.current_owner == "alice" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == ASSET( "0.250 TBD" ) );
      BOOST_REQUIRE( order->op.open_owner == "sam" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == ASSET( "0.500 TTR" ) );
      order++;

      BOOST_REQUIRE( order->time == fill_order_c_time );
      BOOST_REQUIRE( order->op.current_owner == "bob" );
      BOOST_REQUIRE( order->op.current_orderid == 0 );
      BOOST_REQUIRE( order->op.current_pays == ASSET( "0.450 TTR" ) );
      BOOST_REQUIRE( order->op.open_owner == "alice" );
      BOOST_REQUIRE( order->op.open_orderid == 0 );
      BOOST_REQUIRE( order->op.open_pays == ASSET( "0.250 TBD" ) );
      order++;

      BOOST_REQUIRE( order == order_hist_idx.end() );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

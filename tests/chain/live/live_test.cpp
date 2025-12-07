#include <boost/test/unit_test.hpp>

#include <zattera/protocol/exceptions.hpp>
#include <zattera/protocol/hardfork.hpp>

#include <zattera/chain/database.hpp>
#include <zattera/chain/zattera_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "../../fixtures/database_fixture.hpp"

#include <iostream>

using namespace zattera;
using namespace zattera::chain;
using namespace zattera::protocol;

#ifndef IS_TEST_MODE

BOOST_FIXTURE_TEST_SUITE( live_tests, live_database_fixture )


BOOST_AUTO_TEST_CASE( retally_votes )
{
   try
   {
      flat_map< account_name_type, share_type > expected_votes;

      const auto& by_account_witness_idx = db->get_index< witness_vote_index >().indices();

      for( const auto& vote: by_account_witness_idx )
      {
         if( expected_votes.find( vote.witness ) == expected_votes.end() )
            expected_votes[ vote.witness ] = db->get< account_object, by_name >( vote.account ).witness_vote_weight();
         else
            expected_votes[ vote.witness ] += db->get< account_object, by_name >( vote.account ).witness_vote_weight();
      }

      db->retally_witness_votes();

      const auto& witness_idx = db->get_index< witness_index, by_name >();

      for( const auto& witness : witness_idx )
      {
         BOOST_REQUIRE_EQUAL( witness.votes.value, expected_votes[ witness.owner ].value );
      }
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif

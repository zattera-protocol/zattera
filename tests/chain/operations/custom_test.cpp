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

BOOST_AUTO_TEST_CASE( check_custom_authorities )
{
   custom_operation op;
   op.required_auths.insert( "alice" );
   op.required_auths.insert( "bob" );

   flat_set< account_name_type > auths;
   flat_set< account_name_type > expected;

   op.get_required_owner_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   op.get_required_posting_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   expected.insert( "alice" );
   expected.insert( "bob" );
   op.get_required_active_authorities( auths );
   BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_CASE( check_custom_json_authorities )
{
   custom_json_operation op;
   op.required_auths.insert( "alice" );
   op.required_posting_auths.insert( "bob" );

   flat_set< account_name_type > auths;
   flat_set< account_name_type > expected;

   op.get_required_owner_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   expected.insert( "alice" );
   op.get_required_active_authorities( auths );
   BOOST_REQUIRE( auths == expected );

   auths.clear();
   expected.clear();
   expected.insert( "bob" );
   op.get_required_posting_authorities( auths );
   BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_CASE( check_custom_binary_authorities )
{
   ACTORS( (alice) )

   custom_binary_operation op;
   op.required_owner_auths.insert( "alice" );
   op.required_active_auths.insert( "bob" );
   op.required_posting_auths.insert( "sam" );
   op.required_auths.push_back( db->get< account_authority_object, by_account >( "alice" ).posting );

   flat_set< account_name_type > acc_auths;
   flat_set< account_name_type > acc_expected;
   vector< authority > auths;
   vector< authority > expected;

   acc_expected.insert( "alice" );
   op.get_required_owner_authorities( acc_auths );
   BOOST_REQUIRE( acc_auths == acc_expected );

   acc_auths.clear();
   acc_expected.clear();
   acc_expected.insert( "bob" );
   op.get_required_active_authorities( acc_auths );
   BOOST_REQUIRE( acc_auths == acc_expected );

   acc_auths.clear();
   acc_expected.clear();
   acc_expected.insert( "sam" );
   op.get_required_posting_authorities( acc_auths );
   BOOST_REQUIRE( acc_auths == acc_expected );

   expected.push_back( db->get< account_authority_object, by_account >( "alice" ).posting );
   op.get_required_authorities( auths );
   BOOST_REQUIRE( auths == expected );
}

BOOST_AUTO_TEST_SUITE_END()
#endif

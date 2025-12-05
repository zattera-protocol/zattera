#pragma once

#include <zattera/protocol/zattera_operations.hpp>

#include <zattera/chain/evaluator.hpp>

namespace zattera { namespace chain {

using namespace zattera::protocol;

ZATTERA_DEFINE_EVALUATOR( account_create )
ZATTERA_DEFINE_EVALUATOR( account_create_with_delegation )
ZATTERA_DEFINE_EVALUATOR( account_update )
ZATTERA_DEFINE_EVALUATOR( transfer )
ZATTERA_DEFINE_EVALUATOR( transfer_to_vesting )
ZATTERA_DEFINE_EVALUATOR( witness_update )
ZATTERA_DEFINE_EVALUATOR( account_witness_vote )
ZATTERA_DEFINE_EVALUATOR( account_witness_proxy )
ZATTERA_DEFINE_EVALUATOR( withdraw_vesting )
ZATTERA_DEFINE_EVALUATOR( set_withdraw_vesting_route )
ZATTERA_DEFINE_EVALUATOR( comment )
ZATTERA_DEFINE_EVALUATOR( comment_options )
ZATTERA_DEFINE_EVALUATOR( delete_comment )
ZATTERA_DEFINE_EVALUATOR( vote )
ZATTERA_DEFINE_EVALUATOR( custom )
ZATTERA_DEFINE_EVALUATOR( custom_json )
ZATTERA_DEFINE_EVALUATOR( custom_binary )
ZATTERA_DEFINE_EVALUATOR( feed_publish )
ZATTERA_DEFINE_EVALUATOR( convert )
ZATTERA_DEFINE_EVALUATOR( limit_order_create )
ZATTERA_DEFINE_EVALUATOR( limit_order_cancel )
ZATTERA_DEFINE_EVALUATOR( report_over_production )
ZATTERA_DEFINE_EVALUATOR( limit_order_create2 )
ZATTERA_DEFINE_EVALUATOR( escrow_transfer )
ZATTERA_DEFINE_EVALUATOR( escrow_approve )
ZATTERA_DEFINE_EVALUATOR( escrow_dispute )
ZATTERA_DEFINE_EVALUATOR( escrow_release )
ZATTERA_DEFINE_EVALUATOR( claim_account )
ZATTERA_DEFINE_EVALUATOR( create_claimed_account )
ZATTERA_DEFINE_EVALUATOR( request_account_recovery )
ZATTERA_DEFINE_EVALUATOR( recover_account )
ZATTERA_DEFINE_EVALUATOR( change_recovery_account )
ZATTERA_DEFINE_EVALUATOR( transfer_to_savings )
ZATTERA_DEFINE_EVALUATOR( transfer_from_savings )
ZATTERA_DEFINE_EVALUATOR( cancel_transfer_from_savings )
ZATTERA_DEFINE_EVALUATOR( decline_voting_rights )
ZATTERA_DEFINE_EVALUATOR( reset_account )
ZATTERA_DEFINE_EVALUATOR( set_reset_account )
ZATTERA_DEFINE_EVALUATOR( claim_reward_balance )
ZATTERA_DEFINE_EVALUATOR( delegate_vesting_shares )
ZATTERA_DEFINE_EVALUATOR( witness_set_properties )

} } // zattera::chain

#pragma once

#include <fc/container/flat.hpp>
#include <zattera/protocol/operations.hpp>
#include <zattera/protocol/transaction.hpp>

#include <fc/string.hpp>

namespace zattera { namespace app {

using namespace fc;

void operation_get_impacted_accounts(
   const zattera::protocol::operation& op,
   fc::flat_set<protocol::account_name_type>& result );

void transaction_get_impacted_accounts(
   const zattera::protocol::transaction& tx,
   fc::flat_set<protocol::account_name_type>& result
   );

} } // zattera::app

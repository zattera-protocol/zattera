#pragma once

#include <zattera/protocol/block.hpp>

namespace zattera { namespace chain {

struct transaction_notification
{
   transaction_notification( const zattera::protocol::signed_transaction& tx ) : transaction(tx)
   {
      transaction_id = tx.id();
   }

   zattera::protocol::transaction_id_type          transaction_id;
   const zattera::protocol::signed_transaction&    transaction;
};

} }

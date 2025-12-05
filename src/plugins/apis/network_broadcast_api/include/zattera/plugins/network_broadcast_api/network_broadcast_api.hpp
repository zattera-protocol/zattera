#pragma once
#include <zattera/protocol/block.hpp>

#include <zattera/plugins/p2p/p2p_plugin.hpp>
#include <zattera/plugins/chain/chain_plugin.hpp>
#include <zattera/plugins/json_rpc/utility.hpp>

#include <fc/optional.hpp>
#include <fc/variant.hpp>
#include <fc/vector.hpp>

#include <boost/thread/mutex.hpp>

namespace zattera { namespace plugins { namespace network_broadcast_api {

using std::vector;
using fc::variant;
using fc::optional;
using zattera::plugins::json_rpc::void_type;

using zattera::protocol::signed_transaction;
using zattera::protocol::transaction_id_type;
using zattera::protocol::signed_block;

struct broadcast_transaction_args
{
   signed_transaction   trx;
   int32_t              max_block_age = -1;
};

typedef void_type broadcast_transaction_return;

struct broadcast_block_args
{
   signed_block   block;
};

typedef void_type broadcast_block_return;

namespace detail{ class network_broadcast_api_impl; }

class network_broadcast_api
{
   public:
      network_broadcast_api();
      ~network_broadcast_api();

      DECLARE_API(
         (broadcast_transaction)
         (broadcast_block)
      )

   private:
      std::unique_ptr< detail::network_broadcast_api_impl > my;
};

} } } // zattera::plugins::network_broadcast_api

FC_REFLECT( zattera::plugins::network_broadcast_api::broadcast_transaction_args,
   (trx)(max_block_age) )

FC_REFLECT( zattera::plugins::network_broadcast_api::broadcast_block_args,
   (block) )

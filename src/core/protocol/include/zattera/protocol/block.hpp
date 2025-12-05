#pragma once
#include <zattera/protocol/block_header.hpp>
#include <zattera/protocol/transaction.hpp>

namespace zattera { namespace protocol {

   struct signed_block : public signed_block_header
   {
      checksum_type calculate_merkle_root()const;
      vector<signed_transaction> transactions;
   };

} } // zattera::protocol

FC_REFLECT_DERIVED( zattera::protocol::signed_block, (zattera::protocol::signed_block_header), (transactions) )

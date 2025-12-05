#pragma once
#include <zattera/chain/account_object.hpp>
#include <zattera/chain/block_summary_object.hpp>
#include <zattera/chain/comment_object.hpp>
#include <zattera/chain/global_property_object.hpp>
#include <zattera/chain/history_object.hpp>
#include <zattera/chain/zattera_objects.hpp>
#include <zattera/chain/transaction_object.hpp>
#include <zattera/chain/witness_objects.hpp>
#include <zattera/chain/database.hpp>

namespace zattera { namespace plugins { namespace block_api {

using namespace zattera::chain;

struct api_signed_block_object : public signed_block
{
   api_signed_block_object( const signed_block& block ) : signed_block( block )
   {
      block_id = id();
      signing_key = signee();
      transaction_ids.reserve( transactions.size() );
      for( const signed_transaction& tx : transactions )
         transaction_ids.push_back( tx.id() );
   }
   api_signed_block_object() {}

   block_id_type                 block_id;
   public_key_type               signing_key;
   vector< transaction_id_type > transaction_ids;
};

} } } // zattera::plugins::database_api

FC_REFLECT_DERIVED( zattera::plugins::block_api::api_signed_block_object, (zattera::protocol::signed_block),
                     (block_id)
                     (signing_key)
                     (transaction_ids)
                  )

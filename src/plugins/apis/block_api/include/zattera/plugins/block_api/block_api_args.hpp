#pragma once
#include <zattera/plugins/block_api/block_api_objects.hpp>

#include <zattera/protocol/types.hpp>
#include <zattera/protocol/transaction.hpp>
#include <zattera/protocol/block_header.hpp>

#include <zattera/plugins/json_rpc/utility.hpp>

namespace zattera { namespace plugins { namespace block_api {

/* get_block_header */

struct get_block_header_args
{
   uint32_t block_num;
};

struct get_block_header_return
{
   optional< block_header > header;
};

/* get_block */
struct get_block_args
{
   uint32_t block_num;
};

struct get_block_return
{
   optional< api_signed_block_object > block;
};

} } } // zattera::block_api

FC_REFLECT( zattera::plugins::block_api::get_block_header_args,
   (block_num) )

FC_REFLECT( zattera::plugins::block_api::get_block_header_return,
   (header) )

FC_REFLECT( zattera::plugins::block_api::get_block_args,
   (block_num) )

FC_REFLECT( zattera::plugins::block_api::get_block_return,
   (block) )


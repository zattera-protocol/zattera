#pragma once

#include <zattera/chain/zattera_object_types.hpp>
#include <zattera/protocol/types.hpp>

namespace zattera { namespace chain {

   using zattera::protocol::chain_id_type;

   /**
    * @class chain_property_object
    * @brief Stores immutable chain configuration set at genesis
    * @ingroup object
    * @ingroup implementation
    *
    * This object is created once during genesis and stores the chain ID
    * and address prefix. These values are immutable after genesis and
    * are used to verify that a database matches the expected network.
    */
   class chain_property_object : public object< chain_property_object_type, chain_property_object >
   {
      chain_property_object() = delete;

      public:
         template< typename Constructor, typename Allocator >
         chain_property_object( Constructor&& c, allocator< Allocator > a )
            : chain_id_name( a ), address_prefix( a )
         {
            c( *this );
         }

         id_type           id;

         chain_id_type     chain_id;           ///< Chain ID hash (immutable after genesis)
         shared_string     chain_id_name;      ///< Human-readable chain ID (e.g., "zattera", "testnet")
         shared_string     address_prefix;     ///< Address prefix (e.g., "ZTR", "TST")
   };

   typedef multi_index_container<
      chain_property_object,
      indexed_by<
         ordered_unique< tag< by_id >,
            member< chain_property_object, chain_property_object::id_type, &chain_property_object::id > >
      >,
      allocator< chain_property_object >
   > chain_property_index;

} } // zattera::chain

FC_REFLECT( zattera::chain::chain_property_object,
            (id)
            (chain_id)
            (chain_id_name)
            (address_prefix)
          )
CHAINBASE_SET_INDEX_TYPE( zattera::chain::chain_property_object, zattera::chain::chain_property_index )

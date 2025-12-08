#pragma once
#include <string>
#include <zattera/protocol/types.hpp>

namespace zattera { namespace protocol {

   struct runtime_config_data
   {
      chain_id_type   chain_id;
      std::string     chain_id_name;
      std::string     address_prefix;
   };

   const runtime_config_data& get_runtime_config();

   // Convenience accessors
   inline chain_id_type get_chain_id() { return get_runtime_config().chain_id; }
   inline std::string get_chain_id_name() { return get_runtime_config().chain_id_name; }
   inline std::string get_address_prefix() { return get_runtime_config().address_prefix; }

   // Set runtime configuration
   void set_chain_id( const chain_id_type& chain_id, const std::string& chain_id_name );
   void set_address_prefix( const std::string& address_prefix );

} } // zattera::protocol

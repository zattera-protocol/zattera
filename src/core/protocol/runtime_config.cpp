#include <zattera/protocol/runtime_config.hpp>
#include <zattera/protocol/config.hpp>
#include <fc/crypto/sha256.hpp>

namespace zattera { namespace protocol {

   static runtime_config_data _config = {
      fc::sha256::hash( ZATTERA_CHAIN_ID_NAME ),
      ZATTERA_CHAIN_ID_NAME,
      ZATTERA_ADDRESS_PREFIX
   };

   const runtime_config_data& get_runtime_config()
   {
      return _config;
   }

   void set_chain_id( const chain_id_type& chain_id, const std::string& chain_id_name )
   {
      _config.chain_id = chain_id;
      _config.chain_id_name = chain_id_name;
   }

   void set_address_prefix( const std::string& address_prefix )
   {
      _config.address_prefix = address_prefix;
   }

} } // zattera::protocol

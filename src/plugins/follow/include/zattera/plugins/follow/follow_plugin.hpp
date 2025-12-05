#pragma once
#include <zattera/plugins/follow/follow_operations.hpp>

#include <zattera/plugins/chain/chain_plugin.hpp>

#include <zattera/chain/generic_custom_operation_interpreter.hpp>


#define ZATTERA_FOLLOW_PLUGIN_NAME "follow"


namespace zattera { namespace plugins{ namespace follow {

namespace detail { class follow_plugin_impl; }

using namespace appbase;
using zattera::chain::generic_custom_operation_interpreter;

class follow_plugin : public appbase::plugin< follow_plugin >
{
   public:
      follow_plugin();
      virtual ~follow_plugin();

      APPBASE_PLUGIN_REQUIRES( (zattera::plugins::chain::chain_plugin) )

      static const std::string& name() { static std::string name = ZATTERA_FOLLOW_PLUGIN_NAME; return name; }

      virtual void set_program_options(
         options_description& cli,
         options_description& cfg ) override;
      virtual void plugin_initialize( const variables_map& options ) override;
      virtual void plugin_startup() override;
      virtual void plugin_shutdown() override;

      uint32_t max_feed_size = 500;
      fc::time_point_sec start_feeds;

      std::shared_ptr< generic_custom_operation_interpreter< follow_plugin_operation > > _custom_operation_interpreter;

   private:
      std::unique_ptr< detail::follow_plugin_impl > my;
};

} } } //zattera::follow

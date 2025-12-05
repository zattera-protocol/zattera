#include <zattera/plugins/follow/follow_operations.hpp>

#include <zattera/protocol/operation_util_impl.hpp>

namespace zattera { namespace plugins{ namespace follow {

void follow_operation::validate()const
{
   FC_ASSERT( follower != following, "You cannot follow yourself" );
}

void reblog_operation::validate()const
{
   FC_ASSERT( account != author, "You cannot reblog your own content" );
}

} } } //zattera::plugins::follow

ZATTERA_DEFINE_OPERATION_TYPE( zattera::plugins::follow::follow_plugin_operation )

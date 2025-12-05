#pragma once

#include <vector>

namespace zattera{ namespace plugins { namespace p2p {

#ifdef IS_TEST_NET
const std::vector< std::string > default_seeds;
#else
const std::vector< std::string > default_seeds = {
   "seed-east.zattera.club:2001",         // zattera
   "seed-central.zattera.club:2001",      // zattera
   "seed-west.zattera.club:2001",         // zattera
};
#endif

} } } // zattera::plugins::p2p

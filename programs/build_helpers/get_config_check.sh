#!/bin/bash
diff -u \
   <(cat src/core/protocol/get_config.cpp | grep 'result[[]".*"' | cut -d '"' -f 2 | sort | uniq) \
   <(cat src/core/protocol/include/zattera/protocol/config.hpp | grep '[#]define\s\+[A-Z0-9_]\+' | cut -d ' ' -f 2 | sort | uniq)
exit $?


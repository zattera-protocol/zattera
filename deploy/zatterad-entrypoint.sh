#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# if we're not using PaaS mode then start zatterad traditionally with sv to control it
if [[ ! "$DEPLOY_TO_PAAS" ]]; then
  mkdir -p /etc/service/zatterad
  cp /etc/zatterad/runit/zatterad.run /etc/service/zatterad/run
  chmod +x /etc/service/zatterad/run
  runsv /etc/service/zatterad
elif [[ "$IS_TEST_NODE" ]]; then
  /usr/local/bin/zatterad-test-bootstrap.sh
else
  /usr/local/bin/zatterad-paas-bootstrap.sh
fi

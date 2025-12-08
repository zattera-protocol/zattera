#!/bin/bash

VERSION=`cat /etc/zatterad-version`

if [[ "$USE_HIGH_MEMORY" ]]; then
  ZATTERAD="/usr/local/zatterad-high/bin/zatterad"
else
  ZATTERAD="/usr/local/zatterad-low/bin/zatterad"
fi

# copy config from image (data dir is cleaned on every PaaS restart)
if [[ "$ZATTERAD_NODE_MODE" == "fullnode" ]]; then
  cp /etc/zatterad/fullnode.config.ini $HOME/config.ini
elif [[ "$ZATTERAD_NODE_MODE" == "broadcast" ]]; then
  cp /etc/zatterad/broadcast.config.ini $HOME/config.ini
elif [[ "$ZATTERAD_NODE_MODE" == "ahnode" ]]; then
  cp /etc/zatterad/ahnode.config.ini $HOME/config.ini
elif [[ "$ZATTERAD_NODE_MODE" == "witness" ]]; then
  cp /etc/zatterad/witness.config.ini $HOME/config.ini
elif [[ "$ZATTERAD_NODE_MODE" == "testnet" ]]; then
  cp /etc/zatterad/testnet.config.ini $HOME/config.ini
elif [[ -f "/etc/zatterad/config.ini" ]]; then
  cp /etc/zatterad/config.ini $HOME/config.ini
else
  cp /etc/zatterad/fullnode.config.ini $HOME/config.ini
fi

# clean out data dir since it may contain stale data from previous runs
rm -rf $HOME/*

chown -R zatterad:zatterad $HOME

ARGS=""

# if user did pass in desired seed nodes, use the ones the user specified
if [[ ! -z "$ZATTERAD_SEED_NODES" ]]; then
    for NODE in $ZATTERAD_SEED_NODES ; do
        ARGS+=" --p2p-seed-node=$NODE"
    done
fi

NOW=`date +%s`

ZATTERAD_FEED_START_TIME=`expr $NOW - 1209600`
ARGS+=" --follow-start-feeds=$ZATTERAD_FEED_START_TIME"

ZATTERAD_PROMOTED_START_TIME=`expr $NOW - 604800`
ARGS+=" --tags-start-promoted=$ZATTERAD_PROMOTED_START_TIME"

if [[ ! "$DISABLE_BLOCK_API" ]]; then
   ARGS+=" --plugin=block_api"
fi

cd $HOME

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/zatterad.nginx.conf /etc/nginx/nginx.conf

# get blockchain state from an S3 bucket
echo [zatterad:bootstrap][INFO] beginning download and decompress of s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.lz4
finished=0
count=1
if [[ "$USE_RAMDISK" ]]; then
  mkdir -p /mnt/ramdisk
  mount -t ramfs -o size=${RAMDISK_SIZE_IN_MB:-51200}m ramfs /mnt/ramdisk
  ARGS+=" --shared-file-dir=/mnt/ramdisk/blockchain"
  # try five times to pull in shared memory file
  while [[ $count -le 5 ]] && [[ $finished == 0 ]]
  do
    rm -rf $HOME/blockchain/*
    rm -rf /mnt/ramdisk/blockchain/*
    if [[ "$ZATTERAD_NODE_MODE" == "broadcast" ]]; then
      aws s3 cp s3://$S3_BUCKET/broadcast-$VERSION-latest.tar.lz4 - | lz4 -d | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
    elif [[ "$ZATTERAD_NODE_MODE" == "ahnode" ]]; then
      aws s3 cp s3://$S3_BUCKET/ahnode-$VERSION-latest.tar.lz4 - | lz4 -d | tar x --wildcards 'blockchain/block*' 'blockchain/*rocksdb-storage*' -C /mnt/ramdisk 'blockchain/shared*'
    else
      aws s3 cp s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.lz4 - | lz4 -d | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
    fi
    if [[ $? -ne 0 ]]; then
      sleep 1
      echo [zatterad:bootstrap][WARN] unable to pull blockchain state from S3 - attempt $count
      (( count++ ))
    else
      finished=1
    fi
  done
  chown -R zatterad:zatterad /mnt/ramdisk/blockchain
else
  while [[ $count -le 5 ]] && [[ $finished == 0 ]]
  do
    rm -rf $HOME/blockchain/*
    if [[ "$ZATTERAD_NODE_MODE" == "broadcast" ]]; then
      aws s3 cp s3://$S3_BUCKET/broadcast-$VERSION-latest.tar.lz4 - | lz4 -d | tar x
    elif [[ "$ZATTERAD_NODE_MODE" == "ahnode" ]]; then
      aws s3 cp s3://$S3_BUCKET/ahnode-$VERSION-latest.tar.lz4 - | lz4 -d | tar x
    else
      aws s3 cp s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.lz4 - | lz4 -d | tar x
    fi
    if [[ $? -ne 0 ]]; then
      sleep 1
      echo [zatterad:bootstrap][WARN] unable to pull blockchain state from S3 - attempt $count
      (( count++ ))
    else
      finished=1
    fi
  done
fi
if [[ $finished == 0 ]]; then
  if [[ ! "$IS_SNAPSHOT_NODE" ]]; then
    echo [zatterad:bootstrap][ERROR] unable to pull blockchain state from S3 - exiting
    exit 1
  else
    echo [zatterad:bootstrap][ERROR] shared memory file for $VERSION not found, creating a new one by replaying the blockchain
    if [[ "$USE_RAMDISK" ]]; then
      mkdir -p /mnt/ramdisk/blockchain
      chown -R zatterad:zatterad /mnt/ramdisk/blockchain
    else
      mkdir blockchain
    fi
    aws s3 cp s3://$S3_BUCKET/block_log-latest blockchain/block_log
    if [[ $? -ne 0 ]]; then
      echo [zatterad:bootstrap][WARN] unable to pull latest block_log from S3, will sync from scratch.
    else
      ARGS+=" --replay-blockchain --force-validate"
    fi
    touch /tmp/isnewsync
  fi
fi

# for appbase tags plugin loading
ARGS+=" --tags-skip-startup-update"

cd $HOME

if [[ "$IS_SNAPSHOT_NODE" ]]; then
  touch /tmp/issyncnode
  chown www-data:www-data /tmp/issyncnode
fi

chown -R zatterad:zatterad $HOME/*

# let's get going
cp /etc/nginx/zatterad-proxy.conf.template /etc/nginx/zatterad-proxy.conf
echo server 127.0.0.1:8091\; >> /etc/nginx/zatterad-proxy.conf
echo } >> /etc/nginx/zatterad-proxy.conf
rm /etc/nginx/sites-enabled/default
cp /etc/nginx/zatterad-proxy.conf /etc/nginx/sites-enabled/default
/etc/init.d/fcgiwrap restart
service nginx restart
exec chpst -uzatterad \
    $ZATTERAD \
        --webserver-ws-endpoint=127.0.0.1:8091 \
        --webserver-http-endpoint=127.0.0.1:8091 \
        --p2p-endpoint=0.0.0.0:2001 \
        --data-dir=$HOME \
        $ARGS \
        $ZATTERAD_EXTRA_OPTS \
        2>&1&
SAVED_PID=`pgrep -f p2p-endpoint`
echo $SAVED_PID > /var/run/zatterad.pid
mkdir -p /etc/service/zatterad
if [[ ! "$IS_SNAPSHOT_NODE" ]]; then
  cp /etc/zatterad/runit/zatterad-paas-monitor.run /etc/service/zatterad/run
else
  cp /etc/zatterad/runit/zatterad-snapshot-uploader.run /etc/service/zatterad/run
fi
chmod +x /etc/service/zatterad/run
runsv /etc/service/zatterad

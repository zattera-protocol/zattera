#!/bin/bash
#params:
# - tested zatterad path
# - reference zatterad path
# - tested data dir
# - reference data dir
# - logs dir path
# - stop replay at block
# - number of jobs (optional)
# - --dont-copy-config (optional), if passed config.init files are not copied from test directories
#
# WARNING: use absolute paths instead of relative!
#
# sudo ./docker_build_and_run.sh ~/zattera-test/tested/build/programs/zatterad/zatterad ~/zattera-test/reference/build/programs/zatterad/zatterad ~/zattera-test/tested/data ~/zattera-test/reference/data ~/zattera-test/logs 5000000 12

if [ $# -lt 6 -o $# -gt 8 ]
then
   echo "Usage: tested_zatterad_path reference_zatterad_path tested_data_dir reference_data_dir"
   echo "       logs_dir stop_replay_at_block [jobs [--dont-copy-config]"
   echo "Example: ~/zattera-test/tested/build/programs/zatterad/zatterad ~/zattera-test/reference/build/programs/zatterad/zatterad ~/zattera-test/tested/data ~/zattera-test/reference/data"
   echo "         ~/zattera-test/logs 5000000 12"
   echo "         if <jobs> not passed, <nproc> will be used."
   exit -1
fi

echo $*

JOBS=0

if [ $# -eq 7 ]
then
   JOBS=$7
fi

docker build -t smoketest ../ -f Dockerfile
[ $? -ne 0 ] && echo docker build FAILED && exit -1

docker system prune -f

if [ -e $5 ]; then
   rm -rf $5/*
else
   mkdir -p $5
fi

docker run -v $1:/tested_zatterad -v $2:/reference_zatterad -v $3:/tested_data_dir -v $4:/reference_data_dir -v $5:/logs_dir -v /run:/run \
   -e STOP_REPLAY_AT_BLOCK=$6 -e JOBS=$JOBS -e COPY_CONFIG=$8 -p 8090:8090 -p 8091:8091 smoketest:latest

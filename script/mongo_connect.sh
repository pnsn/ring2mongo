#!/bin/bash
#create mongo container linked ot running ringymongo container, Remove container on exit
docker run -it --link ringymongo:mongo --rm mongo sh -c 'exec mongo "$MONGO_PORT_27017_TCP_ADDR:$MONGO_PORT_27017_TCP_PORT/test"'

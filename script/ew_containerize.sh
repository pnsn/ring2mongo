#!/bin/bash


dir=$(pwd)
#run this line if you haven't created a mongo container
#docker run -it --name ringymongo -d mongo
#run this one if you have created a mongo container
docker start ringymongo 
docker run -it --name ring2mongo  --rm --link ringymongo:mongo -v $dir/params:/eworm/run/params joncon/ring2mongo:0.3 /bin/bash

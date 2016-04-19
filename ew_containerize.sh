#!/bin/bash
#create initial container
#once container is created you can run with 
#docker start eworm
#and connect with 
#docker exec -it eworm /bin/bash
dir=$(pwd)
# docker run -it --name eworm -v $dir/params:/eworm/run/params joncon/eworm:7.8 startstop
docker run -it --name ring2mongo --rm -v $dir/params:/eworm/run/params joncon/ring2mongo:0.1 /bin/bash
# ring2mongo
An earthworm 64bit 7.8 routine for reading tracebuf2 data from a ring and saving to a mongodb capped colleciton that acts as a ring. This ring can then be used to created archived collections. See https://github.com/pnsn/quickshake
## Details
ring2mongo only writes trace data to mongodb. It is not concerned with serving the data. 

Trace Data are written to a single collection and indexed on key (SCNL) and starttime. It is recommended that you create a capped collection to keep the colllection from growing too large.
1) DB name: waveform
2f) Collection: ring

To set up mongo do the folllowing:
* <code> > use waveforms </code>
* <code> > db.createCollection( "ring", { capped: true, size: SOME_SIZE_IN_BYTES } ) </code>
* <code> > db.cwaves.createIndex({"key": 1, "starttime": 1}) </code>



## Docker
Use as a docker routine for local testing and configuration. Docker has not been tested for production

<code>./ew_containerize</code>
This script will do the following: 
* Mount the current $dir/params to the ew params dir, which will put your local files into the ew params directory.
* Start an interactive shell and put you in the mounted params dir

Next run the startstop script in the params dir
<code>startstop.sh</code>

### Configuration
Add any configure files from /eworm/earthworm_[version]/params to /eworm/run/params

### Issues
Using startstop as an entrypoint does not work since the child processes don't run.

A earthworm 7.8 module for reading tracebuf2 data from a WAVE_RING and save to mongodb

## Standard Install
### Mongo-C Driver
Follow the directions to install Mongo-C Driver
https://github.com/mongodb/mongo-c-driver
I found that building from source was the only way to get it to work on Centos7
You'll probably have to edit the following Env variables. These paths are RHEL/CENTOS 6.5 & 7
* <code> echo "export CPATH=$CPATH:/usr/local/include/libmongoc-1.0:/usr/local/include/libbson-1.0" >> ~/.bashrc </code>
* <code> export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib" >> ~/.bashrc </code>
* <code> export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkgconfig" >> ~/.bashrc </code>
* <code> source ~/.bashrc </code>
*Ensure openssl and openssl-devel are installed before you run ./configure to set the SSL compile flag to true.



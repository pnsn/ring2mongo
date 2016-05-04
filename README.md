#ring2mongo
A earthworm 64bit 7.8 routine for reading tracebuf2 data from a ring and saving to mongodb. 
##Details
ring2mongo only writes trace data to mongodb. It is not concerned with serving the data. 

Trace Data are written to a single collection and indexed on key (SCNL) and starttime. It is recommended that you create a capped collection to keep the colllection from growing too large.
1) DB name: waveform
2) Collection: continuous

To set up mongo do the folllowing:
* <code> > use waveform </code>
* <code> > db.createCollection( "continuous", { capped: true, size: SOME_SIZE_IN_BYTES } ) </code>
* <code> > db.continuous.createIndex({"key": 1, "starttime": 1}) </code>



##Usage 

###Docker
Use as a docker routine for local testing and configuration. Docker has not been tested for production

<code>./ew_containerize</code>
This script will do the following: 
* Mount the current $dir/params to the ew params dir, which will put your local files into the ew params directory.
* Start an interactive shell and put you in the mounted params dir

Next run the startstop script in the params dir
<code>startstop.sh</code>

####Configuration
Add any configure files from /eworm/earthworm_[version]/params to /eworm/run/params

####Issues
Using startstop as an entrypoint does not work since the child processes don't run.

A earthworm 7.8 module for reading tracebuf2 data from a WAVE_RING and save to mongodb

###Standard Install
Stay tuned...

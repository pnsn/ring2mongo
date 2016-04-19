#!/usr/bin/python
# latte.py
# wrapper for sniffwave to rank and report data latency
# Victor Kress, PNSN 2014/9/16
# $Id$

import sys
import time
import re
import math
from subprocess import Popen,PIPE

retime=re.compile(r'\(([0-9\.]+)\)')
relat=re.compile(r'D:([\- 0-9\.]+)s')
resncl=re.compile(r'^[A-Z0-9]+\.[A-Z0-9]+\.[A-Z0-9]+\.[A-Z0-9-]+')

usage='''Usage: latte.py RING_NAME [options]
where [options] includes all of the standard options for sniffwave
except 'n' and 'y'.  If time is not specified a default of 10 seconds
will be assumed. First 10 seconds of data will not be included in
statistics.
'''

default_seconds=10

def parseline(s):
    scnl=s.lstrip().split(' ',1)[0]
    if s.rstrip()[-1]!=']':
        return None
    if not resncl.match(scnl): #line does not start with valid scnl
        return None
    try:
        (starttime,endtime)=map(float,retime.findall(s))
        latency=float(relat.search(s).group(1))
    except:
        print 'could not parse\n%s'%s
        return None
    if latency<0.0:
        print '%f second latency for %s. Ignoring.'%(latency,scnl)
        return None
    return (scnl,starttime,endtime,latency)

def getStationDict(url='http://pnsn.org/stations.json'):
    '''gets station list from PNSN json feed and puts data into
    dictionary indexed on station and network name.
    stadict['sta']['net'].keys()=elev,lat,lon,type
    @return stadict - station dictionary'''
    import urllib
    import json
    u=urllib.urlopen(url)
    if not u:
        print 'getStationDict() could not open %s'%url
        return None
    l=u.readlines()
    u.close()
    d=json.loads(l[0],'UTF-8')
    stadict={}
    for s in d: #put into simpler station-keyed dictionary
        if not stadict.has_key(s['sta']):
            stadict[s['sta']]={}
        stadict[s['sta']][s['net']]={'elev':float(s['elev']),
                                     'lat':float(s['lat']),
                                     'lon':float(s['lon']),
                                     'type':s['type']}
    return stadict

if __name__=="__main__":
    latlon=False
    seconds=None
    if len(sys.argv)>1:
        #set up command string
        if '--latlon' in sys.argv:  #add lat-lon to output
            latlon=getStationDict()
            sys.argv.remove('--latlon')
        larg=sys.argv[-1]
        if larg.isdigit():
            seconds=int(larg)
        if not seconds:
            seconds=default_seconds
            if larg=='y' or larg=='n':
                sys.argv[-1]='%s'%seconds
            else:
                sys.argv.append('%s'%seconds)
        cmd=['sniffwave']+sys.argv[1:]
        print 'running: "%s"'%' '.join(cmd) 
        unixstart=time.time()
    else:
        print usage
        sys.exit(0)

    #run program
    print 'Accumulating statistics for %d seconds.  Please wait.'%seconds
    p=Popen(cmd,stdout=PIPE)
    output = p.stdout.readlines()
    scnldict={}
    for l in output:
        p=parseline(l)
        if p:
            (scnl,starttime,endtime,latency)=p
            if endtime>unixstart:
                packlen=endtime-starttime
                if scnldict.has_key(scnl):
                    scnldict[scnl]['sp']+=packlen
                    scnldict[scnl]['ssp']+=packlen*packlen
                    scnldict[scnl]['sl']+=latency
                    scnldict[scnl]['ssl']+=latency*latency
                    scnldict[scnl]['n']+=1
                else:
                    scnldict[scnl]={'sp':packlen,
                                    'ssp':packlen*packlen,
                                    'sl':latency,
                                    'ssl':latency*latency,
                                    'n':1}
    results=[]
    for scnl in scnldict.keys():
        n=scnldict[scnl]['n']
        ml=scnldict[scnl]['sl']/n
        mlss=scnldict[scnl]['ssl']/n
        mp=scnldict[scnl]['sp']/n
        mpss=scnldict[scnl]['ssp']/n
        sdp=sdl=0.
        sdsq=mlss-(ml*ml)
        if sdsq>0.:
            sdl=math.sqrt(sdsq)
        sdsq=mpss-(mp*mp)
        if sdsq>0.:
            sdp=math.sqrt(sdsq)
        results.append((scnl.strip(),mp,sdp,ml,sdl,n))
    results.sort(key=lambda x:x[3])
    header='%16s %6s %6s %6s %6s %s'%('scnl','plen','sdlen','ltncy','sdlat','n')
    if latlon:
        header+='%10s %10s'%('latitude','longitude')
    print header
    for item in results:
        outstr='%16s %6.3f %6.3f %6.3f %6.3f %d'%item
        if latlon:
            (sta,ch,net,loc)=item[0].split('.')
            if latlon.has_key(sta) and latlon[sta].has_key(net):
                outstr+='%10f %10f'%(latlon[sta][net]['lat'],
                                     latlon[sta][net]['lon'])
            else:
                outstr+=' no location for %s.%s'%(sta,net)
        print outstr
    sys.exit(0)
            

import dpkt
import csv
import sys
import os
import time
import math
import win32com.client
from re import compile # import re

nmeaChecksumRegExStr = r"""\*[0-9A-F][0-9A-F]"""
nmeaChecksumRE = compile(nmeaChecksumRegExStr)

fieldlist =('UTS',
             'Lat',
             'Lon',
             'SOG',
             'COGT',
             'COGM',
             'Variation',
             'Depth',
             'CrseWind',
             'RelWind',
             'TrueWind',
             'TWSpd',
             'RWSpd',
             'SpdParWind',
             'BtHDG',
             'BtWatSpd',
             'WPLat',
             'WPLong',
             'WPRteCrse',
             'XTE',
             'BrngWP',
             'DistWP',
             'VMG',
             'Waypoint'
             )

trt_record = dict((field,'0.0') for field in fieldlist)

oldUTS = '0.0'

global RWS, RWA, RWC, TWS, TWA, TWC, SOG, record_date, old_date
RWS = 0.0
RWA = 0.0
RWC = 0
TWS = 0.0
TWA = 0.0
TWC = 0
SOG = 0.0
COG = 0.0
record_date = '140101'
old_date = '1'


def make_UTS(nmea_string):
    global old_date, record_date
    if len(nmea_string) > 5:
        if long(nmea_string[0:6]) + 1 < long(old_date):
            record_date = '{0}'.format(long(record_date) + 1)        
        old_date = nmea_string[0:6]    
        return record_date + ' ' + nmea_string[0:2]+':'+ nmea_string[2:4]+':'+ nmea_string[4:6] + ' Z'
    else:
        return ''
    
def nmea_to_deg(nmea_string):
    if len(nmea_string) > 0:
        decimal = nmea_string.find('.')
        deg = nmea_string[0:decimal-2]
        minutes = nmea_string[decimal-2:]
        deci_degrees = '{0:.5f}'.format(float(minutes)/60)
        deg = deg + deci_degrees[1:]
        return deg
    else:
        return '0.00000'

def VTW(VAW,BAW,SOG):
    VTW_value = math.sqrt(pow((VAW * math.sin(BAW)),2) + pow((VAW * math.cos(BAW)- SOG),2))
    return VTW_value

def BTW(VAW,BAW,SOG):
    BTW_value = math.atan((VAW * math.sin(BAW))/(VAW * math.cos(BAW)-SOG))
    if BAW < 3.1416:
        if BTW_value < 0:
            BTW_value = math.pi + BTW_value
    if BAW > 3.1416:
        if BTW_value > 0:
            BTW_value = math.pi + BTW_value
        if BTW_value < 0:
            BTW_value = math.pi * 2 + BTW_value
    return BTW_value

def VAW(VTW,BTW,SOG):
    VAW_value = math.sqrt(pow((VTW * math.sin(BTW)),2) + pow((VTW * math.cos(BTW)+ SOG), 2))
    return VAW_value

def BAW(VTW,BTW,SOG):
    BAW_value = math.atan((VTW * math.sin(BTW))/(VTW * math.cos(BTW)+ SOG))
    if BTW < 3.1416:
        if BAW_value < 0:
            BAW_value = math.pi+ BAW_value
    if BTW > 3.1416:
        if BAW_value > 0:
            BAW_value = math.pi + BAW_value
        if BAW_value < 0:
            BAW_value = math.pi * 2 + BAW_value    
    return BAW_value

def update_wind_record():
    if float(trt_record['SOG']) > 0:
        if float(trt_record['RelWind']) > 0.0:
            trt_record['TWSpd'] = '{0:.2f}'.format(VTW(float(trt_record['RWSpd']),
                                float(trt_record['RelWind']),float(trt_record['SOG'])))
            trt_record['CrseWind'] = '{0:.5f}'.format(BTW(float(trt_record['RWSpd']),
                                float(trt_record['RelWind']),float(trt_record['SOG'])))
            trt_record['TrueWind'] = '{0:.5f}'.format(float(trt_record['CrseWind']) + float(trt_record['COGT']))

        else:
            trt_record['RelWind'] = '{0:.5f}'.format(BAW(float(trt_record['TWSpd']),
                                float(trt_record['TrueWind']),float(trt_record['SOG'])))
            trt_record['RWSpd'] = '{0:.2f}'.format(VAW(float(trt_record['TWSpd']),
                                float(trt_record['TrueWind']),float(trt_record['SOG'])))
    
def clear_trt_record():
    global RWS, RWA, RWC, TWS, TWA, TWC, SOG
    oldUTS = trt_record['UTS']
    for field in fieldlist:
        trt_record[field] = '0.0'    
    trt_record['UTS'] = oldUTS
    
    RWS = 0.0
    RWA = 0.0
    RWC = 0
    TWS = 0.0
    TWA = 0.0
    TWC = 0
    SOG = 0.0
    COG = 0.0
    

class switch(object):
    value = None
    def __new__(class_, value):
        class_.value = value
        return True

def case(*args):
    return any((arg == switch.value for arg in args))

def checksumStr(data,verbose=False):
      """
      Take a NMEA 0183 string and compute the checksum.
      Checksum is calculated by xor'ing everything between $ or ! and the *
      """
      end = data.find('*')
      start=0
      if data[0] in ('$','!'): start=1
      if -1 != end: data=data[start:end]
      else: data=data[start:]
      if verbose: print 'checking on:',start,end,data
      sum=0
      for c in data: sum = sum ^ ord(c)
      sumHex = "%x" % sum
      if len(sumHex)==1: sumHex = '0'+sumHex
      return sumHex.upper()

def parsebuffer(inbuffer,start_pointer):
    end_message = inbuffer.find('*',start_pointer)
    start_message = inbuffer.rfind('$',start_pointer,end_message)
    nmea_str = inbuffer[start_message:end_message+3]
    return nmea_str, end_message

def parserecord(nmea_str):
    chksum = nmea_str.find('*')
    if chksum != -1:
        if checksumStr(nmea_str) == nmea_str[chksum+1:chksum+3]:
            nmea_pieces = nmea_str.split(',')
            nmea_msg_type = nmea_pieces[0]
            while switch(nmea_msg_type[3:]):
                
                if case("GLL"):
                    if nmea_pieces[6] == 'A':
                        trt_record['UTS'] = make_UTS(nmea_pieces[5])
                        if nmea_pieces[2] =='S':
                            trt_record['Lat'] = '-'+ nmea_to_deg(nmea_pieces[1])
                        else:
                            trt_record['Lat'] = nmea_to_deg(nmea_pieces[1])
                        if nmea_pieces[4] =='W':
                            trt_record['Lon'] = '-'+ nmea_to_deg(nmea_pieces[3])
                        else:
                            trt_record['Lon'] = nmea_to_deg(nmea_pieces[3])
                    break
                
                if case("RMA"):
                    if nmea_pieces[2] == 'A':
                        trt_record['UTS'] = make_UTS(nmea_pieces[1])
                        if nmea_pieces[4] =='S':
                            trt_record['Lat'] = '-'+ nmea_to_deg(nmea_pieces[3])
                        else:
                            trt_record['Lat'] = nmea_to_deg(nmea_pieces[3])
                        if nmea_pieces[6] =='W':
                            trt_record['Lon'] = '-'+ nmea_to_deg(nmea_pieces[5])
                        else:
                            trt_record['Lon'] = nmea_to_deg(nmea_pieces[5])
                                
                        if len(nmea_pieces[7]) > 0:
                            global SOG
                            SOG = float(nmea_pieces[7])
                            trt_record['SOG'] = '{0:.2}'.format(SOG)
                            
                        if len(nmea_pieces[8]) > 0:
                            if math.isnan(float(nmea_pieces[8])) == False:
                                trt_record['COGT'] = '{0:.5}'.format(math.radians(float(nmea_pieces[8])))
                        
                        if nmea_pieces[11] =='E':
                            trt_record['Variation'] = '-'+ '{0:.5}'.format(math.radians(float(nmea_pieces[10])))
                        else:
                            trt_record['Variation'] = '{0:.5}'.format(math.radians(float(nmea_pieces[10])))
                        
                    break
                
                if case("RMB"):
                    if nmea_pieces[2] == 'A':
                        trt_record['UTS'] = make_UTS(nmea_pieces[1])
                        if nmea_pieces[4] =='S':
                            trt_record['Lat'] = '-'+ nmea_to_deg(nmea_pieces[3])
                        else:
                            trt_record['Lat'] = nmea_to_deg(nmea_pieces[3])
                        if nmea_pieces[6] =='W':
                            trt_record['Lon'] = '-'+ nmea_to_deg(nmea_pieces[5])
                        else:
                            trt_record['Lon'] = nmea_to_deg(nmea_pieces[5])
                                
                        if len(nmea_pieces[7]) > 0:
                            SOG = float(nmea_pieces[7])
                            trt_record['SOG'] = '{0:.2}'.format(SOG)
                            
                        if len(nmea_pieces[8]) > 0:
                            if math.isnan(float(nmea_pieces[8])) == False:
                                trt_record['COGT'] = '{0:.5}'.format(math.radians(float(nmea_pieces[8])))
                        
                        if nmea_pieces[11] =='E':
                            trt_record['Variation'] = '-'+ '{0:.5}'.format(math.radians(float(nmea_pieces[10])))
                        else:
                            trt_record['Variation'] = '{0:.5}'.format(math.radians(float(nmea_pieces[10])))
                    break
                
                if case("RMC"):
                    if nmea_pieces[2] == 'A':
                        trt_record['UTS'] = make_UTS(nmea_pieces[1])
                        if nmea_pieces[4] =='S':
                            trt_record['Lat'] = '-'+ nmea_to_deg(nmea_pieces[3])
                        else:
                            trt_record['Lat'] = nmea_to_deg(nmea_pieces[3])
                        if nmea_pieces[6] =='W':
                            trt_record['Lon'] = '-'+ nmea_to_deg(nmea_pieces[5])
                        else:
                            trt_record['Lon'] = nmea_to_deg(nmea_pieces[5])
                                
                        if len(nmea_pieces[7]) > 0:
                            if math.isnan(float(nmea_pieces[7])) == False:
                                SOG = float(nmea_pieces[7])
                                trt_record['SOG'] = '{0:.2}'.format(SOG)
                                if trt_record['BtWatSpd'] == '0.0':
                                    trt_record['BtWatSpd'] = trt_record['SOG']
                            
                        if len(nmea_pieces[8]) > 0:
                            if math.isnan(float(nmea_pieces[8])) == False:
                                COG = float(nmea_pieces[8])
                                trt_record['COGT'] = '{0:.5}'.format(math.radians(COG))
                        
                        if len(nmea_pieces[10]) > 0:
                            if math.isnan(float(nmea_pieces[10])) == False:
                                Var = '{0:.5}'.format(math.radians(float(nmea_pieces[10])))
                                if nmea_pieces[11] =='E':
                                    trt_record['Variation'] = '-' + Var
                                else:
                                    trt_record['Variation'] = Var
                    break
                
                if case("MWV"):
                    global RWS, RWA, RWC, TWS, TWA, TWC
                    if len(nmea_pieces[1]) > 1:
                        if nmea_pieces[2] == 'R':
                            RWA = RWA + float(nmea_pieces[1])
                            RWC = RWC + 1
                            trt_record['RelWind'] = '{0:.5}'.format(math.radians(RWA/RWC))
                            if nmea_pieces[4] == 'N':
                                RWS = RWS + float(nmea_pieces[3])
                                trt_record['RWSpd'] = '{0:.2}'.format(RWS/RWC)
                        if nmea_pieces[2] == 'T':
                            TWA = TWA + float(nmea_pieces[1])
                            TWC = TWC + 1
                            trt_record['TrueWind'] = '{0:.5}'.format(math.radians(TWA/TWC))
                            if nmea_pieces[4] == 'N':
                                TWS = TWS + float(nmea_pieces[3])
                                trt_record['TWSpd'] = '{0:.2}'.format(TWS/TWC)
                    break
                if case("VTG"):
                    break
                if case("DPT"):
                    break
                if case("GGA"):
                    break
                if case("ZDA"):
                    break
                if case("VLW"):
                    break
                if case("AAM"):
                    break
                if case("HDG"):
                    break
                if case("VHW"):
                    break
                if case("MWD"):
                    break
                if case("TLL"):
                    break
                if case("TTM"):
                    break
                if case("XDR"):
                    break
                if case("GSA"):
                    break
                if case("GSV"):
                    break
                if case("BWR"):
                    break
                if case("BOD"):
                    break
                if case("APB"):
                    break
                if case("XTE"):
                    break
                if case("MTW"):
                    break
                if case("BWC"):
                    break
                if case("DBT"):
                    break
                if case("MTW"):
                    break
                if case("VDM"):
                    break
                print "NMEA code not parsed", nmea_msg_type
                break
        
    #time.sleep(.1)



filename = "C:/Users/Public/NMEA/VDR/V21-140514.txt"
filename = "C:/Users/Public/pcap/i30-131213.pcap"
################################################################################

filename = raw_input("Enter the NMEA Data file (pcap or txt):")

if os.path.isfile(filename):
    print ("OK", filename)
else:
    print ("Can't find:",filename)
    sys.stderr.write("Cannot open file for reading\n")
    sys.exit(-1)

ext = filename.find('.')
ext_type = filename[ext:]
file_hdr = filename[:ext]
trtfile = file_hdr + '.trt'
boat_type = file_hdr.find('-')
record_date = '20'+ file_hdr[boat_type+1:]
trt_recorder = csv.DictWriter(open(trtfile, "wb"),fieldlist)

nmeafile =  open (filename,'rb')
clear_trt_record()

if ext_type == '.pcap':
    for ts, buf in dpkt.pcap.Reader(nmeafile):
        eth=dpkt.ethernet.Ethernet(buf)
        if eth.type == dpkt.ethernet.ETH_TYPE_IP:
            ip = eth.data
            if ip.p == 6:
                tcp = ip.data
                if tcp.sport == 10110:
                    end_pointer = len(tcp.data)
                    start_pointer = 0
                    while end_pointer > 0:                   
                        nmea_str,end_pointer = parsebuffer(tcp.data,start_pointer)
                        start_pointer = end_pointer +1
                        parserecord(nmea_str)
                        if trt_record['UTS'] != '' and trt_record['UTS'] != oldUTS:
                            if float(trt_record['SOG']) > .3:
                                update_wind_record()
                                trt_recorder.writerow(trt_record)
                                oldUTS = trt_record['UTS']
                                clear_trt_record()
                            
                        
                    
else:
    for record in nmeafile:
        parserecord(record)
        if trt_record['UTS'] != '' and trt_record['UTS'] != oldUTS:
            if float(trt_record['SOG']) > .3:
                update_wind_record()
                trt_recorder.writerow(trt_record)
                oldUTS = trt_record['UTS']
                clear_trt_record()
            

       

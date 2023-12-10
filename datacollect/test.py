import sys
VERSION = ''
with open('/inc/version.inc') as f:
    lines = f.readlines()
    VERSION = lines[0].split('=')[1].strip()
sys.path.append('/opt/version/' + VERSION)
sys.path.append('/opt/version/' + VERSION + '/python-libs')
from str_util import print_happy_line
from securitymanager.pysecuritymanager import SecurityManager as SMC
import os
import time
import json
import pandas as pd
import yaml



smc = SMC(ip='localhost', port=9379, passwd="refZa2KcTj$QjyXk")
ok1, bSpotList = smc.get_instrument_list("BINANCE", "SPOT")
ok2, xSpotList = smc.get_instrument_list("XT", "SPOT")
ok3, gSpotList = smc.get_instrument_list("GATEIO", "SPOT")
ok4, hSpotList = smc.get_instrument_list("HUOBI", "SPOT")
ok5, fSpotList = smc.get_instrument_list("FTX", "SPOT")

print_happy_line()
# print(len(bSpotList),json.dumps(bSpotList))
print_happy_line()
if ok1 and ok2:
    interList = list(set(bSpotList).intersection(set(xSpotList)))
    print("binance xt interList")
    interList = [inst for inst in interList if (
        '3S' not in inst and '3L' not in inst)]
    print(len(interList), json.dumps(interList))
print_happy_line()
if ok3 and ok2:
    print("gateio xt interList")
    interList = list(set(xSpotList).intersection(set(gSpotList)))
    print(len(interList))
    interList = [inst for inst in interList if (
        '3S' not in inst and '3L' not in inst and '5S' not in inst and '5L' not in inst)]
    print(len(interList), json.dumps(interList))

print_happy_line()
if ok4 and ok2:
    print("huobi xt interList")
    interList = list(set(xSpotList).intersection(set(hSpotList)))
    interList = [inst for inst in interList if (
        '3S' not in inst and '3L' not in inst)]
    print(len(interList), json.dumps(interList))
print_happy_line()
if ok5 and ok2:
    print("ftx xt interList")
    interList = list(set(xSpotList).intersection(set(fSpotList)))
    interList = [inst for inst in interList if (
        '3S' not in inst and '3L' not in inst)]
    print(len(interList), json.dumps(interList))

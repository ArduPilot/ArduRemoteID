#!/usr/bin/env python3
'''
perform a secure parameter change on a ArduRemoteID node via DroneCAN
user must supply a private key corresponding to one of the public keys on the node
'''

import dronecan, time, sys, random, base64, struct
from dronecan import uavcan

try:
    import monocypher
except ImportError:
    print("Please install monocypher with: python3 -m pip install pymonocypher")
    sys.exit(1)


# get command line arguments
from argparse import ArgumentParser
parser = ArgumentParser(description='secure_command')
parser.add_argument("--bitrate", default=1000000, type=int, help="CAN bit rate")
parser.add_argument("--node-id", default=100, type=int, help="local CAN node ID")
parser.add_argument("--target-node", default=None, type=int, help="target node ID")
parser.add_argument("--private-key", default=None, type=str, help="private key file")
parser.add_argument("--bus-num", default=1, type=int, help="MAVCAN bus number")
parser.add_argument("--signing-passphrase", help="MAVLink2 signing passphrase", default=None)
parser.add_argument("--timeout", help="DroneCAN message timeout", type=float, default=3)
parser.add_argument("uri", default=None, type=str, help="CAN URI")
parser.add_argument("paramop", default=None, type=str, help="parameter operation")
args = parser.parse_args()

should_exit = False

if args.target_node is None:
    print("Must specify target node ID")
    should_exit = True

if args.private_key is None:
    print("Must specify private key file")
    should_exit = True

if should_exit:
    sys.exit(1)

SECURE_COMMAND_GET_REMOTEID_SESSION_KEY = dronecan.dronecan.remoteid.SecureCommand.Request().SECURE_COMMAND_GET_REMOTEID_SESSION_KEY
SECURE_COMMAND_SET_REMOTEID_CONFIG = dronecan.dronecan.remoteid.SecureCommand.Request().SECURE_COMMAND_SET_REMOTEID_CONFIG
session_key = None
sequence = random.randint(0, 0xFFFFFFFF)
last_session_key_req = 0
last_set_config = 0

# Initializing a DroneCAN node instance.
node = dronecan.make_node(args.uri, node_id=args.node_id, bitrate=args.bitrate)
node.can_driver.set_bus(args.bus_num)

if args.signing_passphrase is not None:
    node.can_driver.set_signing_passphrase(args.signing_passphrase)

# Initializing a node monitor
node_monitor = dronecan.app.node_monitor.NodeMonitor(node)

def get_session_key_response(reply):
    if not reply:
        # timed out
        return
    global session_key
    session_key = bytearray(reply.response.data)
    print("Got session key")

def get_private_key():
    '''get private key, return 32 byte key or None'''
    if args.private_key is None:
        return None
    try:
        d = open(args.private_key,'r').read()
    except Exception as ex:
        return None
    ktype = "PRIVATE_KEYV1:"
    if not d.startswith(ktype):
        return None
    return base64.b64decode(d[len(ktype):])
    
def make_signature(seq, command, data):
    '''make a signature'''
    private_key = get_private_key()
    d = struct.pack("<II", seq, command)
    d += data
    if command != SECURE_COMMAND_GET_REMOTEID_SESSION_KEY:
        if session_key is None:
            print("No session key")
            raise Exception("No session key")
        d += session_key
    return monocypher.signature_sign(private_key, d)

def request_session_key():
    '''request a session key'''
    global sequence
    sig = make_signature(sequence, SECURE_COMMAND_GET_REMOTEID_SESSION_KEY, bytes())
    node.request(dronecan.dronecan.remoteid.SecureCommand.Request(
            sequence=sequence,
            operation=SECURE_COMMAND_GET_REMOTEID_SESSION_KEY,
            sig_length=len(sig),
            data=sig,
            timeout=args.timeout),
        args.target_node,
        get_session_key_response)
    sequence = (sequence+1) % (1<<32)
    print("Requested session key")

def config_change_response(reply):
    if not reply:
        # timed out
        return
    result_map = {
        0: "ACCEPTED",
        1: "TEMPORARILY_REJECTED",
        2: "DENIED",
        3: "UNSUPPORTED",
        4: "FAILED" }
    result = result_map.get(reply.response.result, "invalid")
    print("Got change response: %s" % result)
    sys.exit(reply.response.result)
    
def send_config_change():
    '''send remoteid config change'''
    global sequence
    req = args.paramop.encode('utf-8')
    sig = make_signature(sequence, SECURE_COMMAND_SET_REMOTEID_CONFIG, req)
    node.request(dronecan.dronecan.remoteid.SecureCommand.Request(
            sequence=sequence,
            operation=SECURE_COMMAND_SET_REMOTEID_CONFIG,
            sig_length=len(sig),
            data=req+sig,
            timeout=args.timeout),
        args.target_node,
        config_change_response)
    sequence = (sequence+1) % (1<<32)
    print("Requested config change")
    
def update():
    now = time.time()
    global last_session_key_req, last_set_config, session_key
    if session_key is None and now - last_session_key_req > 2.0:
        last_session_key_req = now
        request_session_key()
    if session_key is not None and now - last_set_config > 2.0:
        last_set_config = now
        send_config_change()

while True:
    try:
        update()
        node.spin(timeout=0.1)
    except Exception as ex:
        print(ex)

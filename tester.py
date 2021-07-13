import sph
import ctypes
import struct
# pip install python-osc
#from pythonosc import osc_message_builder

class tester(object):
    def handleSocket(self, msg, type, name, uuid, *args, **kwargs):
        # just pop the first string and return the rest
        t = msg.popstr()
        print("Python test actor: Message received: {}".format(t) )
        #msg = osc_message_builder.OscMessageBuilder(address="/Hello")
        #msg.add_arg("hello from python")
        #osc = msg.build()
        # does not work: return ("/testpython", [0,2,3,4.14,"blaa", ctypes.c_int32(32)]) #osc.dgram
        return ("/testpython", [0,2,3,4.14,"blaa", struct.pack("I", 32)]) #osc.dgram

    def handleStop(self, *args):
        print("Stopping actor")
        return None

import sph
import ctypes
import struct
from pyglet import *
from pyglet.gl import *
from pythonosc import osc_message_builder

class pygletvisualiser(object):
    event_loop = app.EventLoop()
    timeout = 16
    
    x = 100
    y = 50
    xs = 1.0
    ys = 1.0
    
    def __init__(self):
        # width of window
        self.width = 400
        self.height = 400
        self.title = "HELLO PYGLET"
        self.pygletWindow = window.Window(self.width, self.height, self.title)
        
        # creating a batch object
        self.batch = pyglet.graphics.Batch()
        
        color = (50, 225, 30)
        self.rec1 = shapes.Rectangle(self.x, self.y, 10, 10, color = color, batch = self.batch)
    
    def on_render(self):
        self.x += self.xs * .016 * 100 # 50 pixels per second
        self.y += self.ys * .016 * 100 
        
        if self.x > self.width:
            self.x = self.width
            self.xs *= -1
        elif self.x < 0:
            self.x = 0
            self.xs *= -1
        
        if self.y > self.height:
            self.y = self.height
            self.ys *= -1
        elif self.y < 0:
            self.y = 0
            self.ys *= -1
            
        self.rec1.x = self.x
        self.rec1.y = self.y
        
        self.batch.draw()
        
    def handleTimer(self, *args):
        # Update the pyglet window's rendered display
        self.event_loop.clock.tick()
        self.pygletWindow.dispatch_events()
        
        self.pygletWindow.clear()
        
        self.on_render()
        
        self.pygletWindow.flip()
        return None
        
    def handleSocket(self, msg, type, name, uuid, *args, **kwargs):
        t = msg
        print("Pygletvisualiser: Message received: {}".format(t) )
        return None

    def handleStop(self, *args):
        # Close the pyglet window
        self.event_loop.exit()
        self.pygletWindow.dispatch_events()
        
        print("Stopping actor")
        return None

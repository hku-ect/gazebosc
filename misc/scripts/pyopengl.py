import OpenGL
from OpenGL.GL import *
from OpenGL.GLUT import *
from OpenGL.GLU import *

import sph
import ctypes
import struct
import operator

class pyopengl(object):
    # TODO: Add a way to render UI elements from this actor (pass-through capabilities)
    w,h= 500,500
    timeout = 33
    
    cubeVertices = ((1,1,1),(1,1,-1),(1,-1,-1),(1,-1,1),(-1,1,1),(-1,-1,-1),(-1,-1,1),(-1,1,-1))
    cubeEdges = ((0,1),(0,3),(0,4),(1,2),(1,7),(2,5),(2,3),(3,6),(4,6),(4,7),(5,6),(5,7))
    cubeQuads = ((0,3,6,4),(2,5,6,3),(1,2,5,7),(1,0,4,7),(7,4,6,5),(2,3,0,1))
    
    positions = []
    rotations = []

    def __init__(self):
        glutInit()
        glutInitDisplayMode(GLUT_RGBA)
        glutInitWindowSize(self.w, self.h)
        glutInitWindowPosition(0, 0)
        self.wind = glutCreateWindow("PCAP Visualiser")
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);
        glutDisplayFunc(self.showScreen)
        # glutIdleFunc(self.showScreen)
        
        glViewport(0, 0, self.w, self.h)
        gluPerspective(65, (self.w/self.h), 0.1, 50.0)
        glTranslatef(0.0, -10.0, -10.0)

    def showScreen(self):
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        
        # TODO: draw origin grid
        # Draw origin grid
        self.originGrid()
        
        for p in self.positions:
            glPushMatrix()
            self.wireCube(p, (0.1,0.1,0.1))
            glPopMatrix()
        
        glutSwapBuffers()

    # Sphactor functions

    def handleTimer(self, *args):
        if self.wind > 0:
            glutSetWindow(self.wind)
            if glutGetWindow() == self.wind:
                glutMainLoopEvent()
                self.showScreen()
            else:
                self.wind = 0
        pass
        
    def handleSocket(self, address, data, *args, **kwargs):
        endFloats = self.get_contiguous_floats(data)
        # print(address)
        # print(data)
        self.positions = []
        self.rotations = []
        
        #TODO: rotations, how to recognize if length-6 are all positions, or positions and rotations???
        if len(endFloats) % 7 == 0:
            for i in range(0,len(endFloats), 7):
                self.positions.append((endFloats[i], endFloats[i+1], endFloats[i+2]))
                # self.rotations.append((endFloats[i+3],endFloats[i+4],endFloats[i+5],endFloats[i+6]))
        elif len(endFloats) % 6 == 0:
            for i in range(0,len(endFloats), 6):
                self.positions.append((endFloats[i], endFloats[i+1], endFloats[i+2]))
                # self.rotations.append((endFloats[i+3],endFloats[i+4],endFloats[i+5], 1))
        elif len(endFloats) % 3 == 0:
            for i in range(0,len(endFloats), 3):
                self.positions.append((endFloats[i], endFloats[i+1], endFloats[i+2]))
        pass
       
    def handleStop(self, *args):
        glutSetWindow(self.wind)
        if glutGetWindow() == self.wind:
            glutDestroyWindow(self.wind)
        pass

    # Helper functions
    
    def originGrid(self):
        glPushMatrix()
        # Draw origin grid
        glLineWidth(1)
        glColor3f(0.5, 0.5, 0.5)  # Set color to gray
        glBegin(GL_LINES)
        
        # Draw horizontal lines
        for i in range(-10, 11):
            glVertex3f(-10, 0, i)
            glVertex3f(10, 0, i)
        
        # Draw vertical lines
        for i in range(-10, 11):
            glVertex3f(i, 0, -10)
            glVertex3f(i, 0, 10)
        
        glEnd()
        glPopMatrix()
        
    def get_contiguous_floats(self, tup):
        floats = []
        for item in reversed(tup):
            if isinstance(item, float):
                floats.append(item)
            elif floats:
                break
        return tuple(reversed(floats))
       
    def wireCube(self, p, s):
        glBegin(GL_LINES)
        for cubeEdge in self.cubeEdges:
            for cubeVertex in cubeEdge:
                glVertex3fv(tuple(map(operator.add, tuple(map(operator.mul, self.cubeVertices[cubeVertex], s)), p)))
        glEnd()
    
    def solidCube(self):
        glBegin(GL_QUADS)
        for cubeQuad in self.cubeQuads:
            for cubeVertex in cubeQuad:
                glVertex3fv(self.cubeVertices[cubeVertex])
        glEnd()
    
    def square2D(self, x, y, w, h):
        glBegin(GL_QUADS)
        glVertex2f(x, y)
        glVertex2f(x+w, y)
        glVertex2f(x+w, y+w)
        glVertex2f(x, y+w)
        glEnd()
    
    def square3D(self, x, y, z, w, h):
        glBegin(GL_QUADS)
        glVertex3f(x, y, z)
        glVertex3f(x+w, y, z)
        glVertex3f(x+w, y+w, z)
        glVertex3f(x, y+w, z)
        glEnd()
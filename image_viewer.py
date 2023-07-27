# import pyqtgraph.examples
# pyqtgraph.examples.run()

import signal
import zmq


from time import perf_counter

import numpy as np

import pyqtgraph as pg
from pyqtgraph.Qt import QtCore


signal.signal(signal.SIGINT, signal.SIG_DFL)

context = zmq.Context()

socket = context.socket(zmq.SUB)
socket.connect('tcp://localhost:5555')
socket.setsockopt(zmq.SUBSCRIBE, b'image')


app = pg.mkQApp("Image view")

## Create window with GraphicsView widget
win = pg.GraphicsLayoutWidget()
win.show()  ## show widget alone in its own window
win.setWindowTitle('Image view')
view = win.addViewBox()

## lock the aspect ratio so pixels are always square
view.setAspectLocked(True)

## Create image item
img = pg.ImageItem(border='w')
view.addItem(img)

## Set initial view bounds
view.setRange(QtCore.QRectF(0, 0, 600, 600))

updateTime = perf_counter()
elapsed = 0

timer = QtCore.QTimer()
timer.setSingleShot(True)
# not using QTimer.singleShot


def updateData():
    global img, updateTime, elapsed, socket

    message = socket.recv_multipart()[0]
    data = message.removeprefix(b'image ')
    frame = np.frombuffer(data, dtype=np.uint8).reshape((100, 100))
    # print(f'Received: {message}')

    ## Display the data
    img.setImage(frame)

    timer.start(1)
    now = perf_counter()
    elapsed_now = now - updateTime
    updateTime = now
    elapsed = elapsed * 0.9 + elapsed_now * 0.1

    print(f"{1 / elapsed:.1f} fps")

timer.timeout.connect(updateData)
updateData()


if __name__ == '__main__':
    pg.exec()
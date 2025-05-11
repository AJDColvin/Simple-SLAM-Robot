# Receive Lidar data from Arduino via USB (Serial), and send to base station via web sockets
# RasPi = SERVER, Base station (laptop) = CLIENT
import serial
import socket
import threading

HEADER = 4
SERIAL_PORT = '/dev/ttyACM0'
BAUDRATE = 115200

SOCKET_PORT = 5050
SERVER = "192.168.0.97"   
ADDR = (SERVER, SOCKET_PORT)
FORMAT = 'utf-8'


#Setup serial conection 
try: 
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1)
    ser.reset_input_buffer()
    print(f'Serial connection established @ {SERIAL_PORT}')

except Exception as e: 
    print(e)
    print('NO CONNECTION')

# Set up sockets
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #  AF_INET = type of adresses to look for
server.bind(ADDR) # bind to address and port

server.listen()
print(f'Socket connection established @ {SOCKET_PORT}')


def readAndSend(conn, addr):
    print('in readAndSend')

    while True:
        if ser.in_waiting > 0:
            
            data = ser.readline()  # Take RAW serial data from Arduino. Cuts off at \n.
            
            data_length = len(data)
            data_length_encoded = str(data_length).encode(FORMAT)
            data_length_encoded += b' ' * (HEADER - len(data_length_encoded)) # Pad data
            print(f'HEADER: {data_length_encoded}', end ="    ")
            print(f'DATA: {data}')


            conn.send(data_length_encoded)  # <HEADER>
            conn.send(data)  # <DATA>



while True:

    conn, addr = server.accept()  # Accept clients 

    thread = threading.Thread(target = readAndSend, args = (conn, addr))
    thread.start()





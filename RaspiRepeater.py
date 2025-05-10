# Receive Lidar data from Arduino via USB (Serial), and send to base station via web sockets
# RasPi = SERVER, Base station (laptop) = CLIENT
import serial
import socket

# Receive data frame from Arduino

SERIAL_PORT = '/dev/ttyACM1'
BAUDRATE = 115200

SOCKET_PORT = 5050
SERVER = "192.168.0.50"   
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


while True:

    conn, addr = server.accept() # Accept clients #TODO: THIS IS BLOCKING OPEN NEW THREAD

    # Read serial inputs from Arudino

    print("in the loop")
    if ser.in_waiting > 0:
        print('data found')
        try: 
            data = ser.readline().decode().rstrip() # take RAW serial data from Arduino
            print(data)
            
        except: # ignore if the data is invalid 
            print("[ERROR] Couldn't receive serial from Arduino")

    # Send to client (base station)
    conn.send(data)



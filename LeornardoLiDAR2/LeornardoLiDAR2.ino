#include <SoftwareSerial.h>
#include <string.h>

// serial communication packet specification
/* 
PACKET STRUCTURE


    <HEAD> <ANGLE> <SPEED_LSB> <SPEED_MSB> <16 BYTES FOR DATA> <CHECKSUM_LSB> <CHECKSUM_MSB>

    Of which inside the 16 bytes of data, there are four sets of the following bytes: 

    <DISTANCE_LSB><DISTANCE_MSB><SIGNAL_STR_LSB><SIGNAL_STR_MSB>

*/

// index of the bytes
#define ANGLE_IDX 1
#define SPEED_LSB 2
#define SPEED_MSB 3
#define DATA_1 4
#define DATA_2 8
#define DATA_3 12
#define DATA_4 16 
#define CHECKSUM_LSB 20
#define CHECKSUM_MSB 21

#define PACKET_SIZE 22  // size of packet
#define DATA_SIZE 7 // angle, speed, distance x 4, irradiance, validity

#define RX_PIN 0       // rx pin from the sensor
#define BAUDRATE_SENSOR 115200  // baudrate of the sensor
#define BAUDRATE 115200


int data[DATA_SIZE]; // [angle, speed, distance 1, distance 2, distance 3, distance 4, checksum]
uint8_t packet[PACKET_SIZE];    // packet buffer
const unsigned char HEAD_BYTE = 0xFA;   // start byte of the packet
unsigned int packetIndex      = 0;      // packet index
uint8_t receivedByte    = 0;    // received byte
uint8_t lowStrengthFlag = 0; 
bool PACKET_OK  = true;         // if the packet is valid
bool waitPacket = true;         // true if waiting for a packet

int currentSpeed    = 0; 
int baseSpeed       = 0; 

// sensor ouput data
int angle = 0; 
int speed = 0; 

void setup() {

    Serial1.begin(BAUDRATE);

    // setup serial Arduino -> PC
    pinMode(RX_PIN, INPUT);
    Serial.begin(BAUDRATE);
    Serial.println("IM WORKING");

    for (int idx = 0; idx < PACKET_SIZE; idx++) packet[idx] = 0;    // initialize packet buffer
}


void loop() {

    // check if any packet if arrived
    if (Serial1.available() > 0) {
        // Serial.println("Serial1 available");

        receivedByte = Serial1.read();

        if (waitPacket) { // wait for a new packet to arrive
            if (receivedByte == HEAD_BYTE) {
                packetIndex = 0;    // initialise packet index
                waitPacket = false;
                packet[packetIndex++] = receivedByte;
            }

        } else {  // if currently receiving packet

            if (packet[0] == HEAD_BYTE) { // ensure the head of the packet is valid
                
                packet[packetIndex++] = receivedByte; // store received byte
                
                if (packetIndex >= PACKET_SIZE) { // if packet buffer is full
                    // Serial.println("Buffer full");
                    waitPacket = true; // wait for a new packet
                    decodePacket(packet, PACKET_SIZE); // process the packet
                    sendData(data, DATA_SIZE);       
                }
            }
        }
    }
}


void decodePacket(uint8_t packet[], int packetSize) {
    int data_idx = 0; 

    for (int idx = 0; idx < DATA_SIZE; idx++) data[idx] = 0;  // initialise data array

    for (int i = 0; i < packetSize; i++){
      // Serial.print("0x"); Serial.print(packet[i]); Serial.print('\t');
    
        if (i == 0) {   // header byte
          // Serial.print("data: ");
          continue;
        }
        else if (i == 1) {
            uint16_t angle = (packet[i] - 0xA0) * 4;  // convert to values between 0 ~ 360
            if (angle > 360) return; 
            // Serial.print(angle); 
            data[data_idx++] = angle;
            // Serial.print('\t');
        }
        else if (i == 2) {
            int speed = 0x00; 
            speed |= ( (packet[3]<<8) | packet[2]);     

            // an attempt to smoothen the speed readings since they sometimes spikes due to unknown issue
            currentSpeed = abs(speed/64-currentSpeed) > 100 ? currentSpeed*0.95 + (speed/64)*0.05: speed/64; 
            // Serial.print(currentSpeed);
            data[data_idx++] = currentSpeed;
            // Serial.print('\t');
        }
        else if (i == 4 || i == 8 || i == 12 || i == 16) {
            uint16_t distance = 0x00;
            distance |= ((packet[i+1]&0x3F) << 8) | packet[i]; 
//            Serial.print("Distance: ");
//            Serial.println(distance);
            data[data_idx++] = distance;
        }
      
    }

    uint16_t chksum = checksum(packet, (unsigned int)(packet[PACKET_SIZE-2] + (packet[PACKET_SIZE-1]<<8)),PACKET_SIZE-2);
    data[data_idx++] = chksum; 
    
}

// send a packet of size packetSize to Python prog
int sendData(int data[], int dataSize) { 
  for (int i = 0; i < dataSize; i++) {
    Serial.print(data[i]); 
    Serial.print('\t'); 
  }
  Serial.println();  
}

uint16_t checksum(uint8_t packet[], uint16_t sum, uint8_t size)
{
    uint32_t chk32 = 0;
    uint16_t data[size/2]; 
    uint8_t  sensorData[size]; 

    for (int i = 0; i < size; i++) sensorData[i] = packet[i];

    for (int i = 0; i < size/2; i++) {
        data[i] = ((sensorData[i*2+1] << 8) + sensorData[i*2]);
        chk32 = (chk32 << 1) + data[i];
    }

    uint32_t checksum=(chk32 & 0x7FFF) + (chk32 >> 15);
    return  (uint16_t) (checksum & 0x7FFF) == sum;
}

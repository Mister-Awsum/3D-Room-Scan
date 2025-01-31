# Import Required Libraries

import serial
import math

s = serial.Serial('COM4',115200)

s.open
s.reset_output_buffer()
s.reset_input_buffer()

# Creates new file for writing new points

f = open("points.xyz", "w") 

# Variables

PhaseAngle = 0
x = 0
runs = 3
count = 0

# Main Loop

while(count < runs):
    raw = s.readline()
    data = raw.decode("utf-8") # Decodes byte input from UART into string 
    data = data[0:-2] # Removes carriage return and newline from string
    
    if (data != None):
        angle = (PhaseAngle/512)*2*math.pi # Angle based on motor rotation
        r = int(data[3])
        y = r*math.cos(angle) # y calculation
        z = r*math.sin(angle) # z calculation
        f.write('{} {} {}\n'.format(x,y,z)) # Write data to points.xyz
        PhaseAngle += 32
    
    if (PhaseAngle == 512): #reset number of steps after a full rotation
        PhaseAngle = 0
        x += 100             # 10 cm Displacement each run
        count += 1
        
    print(data)
    
f.close() #close file when done so data saves

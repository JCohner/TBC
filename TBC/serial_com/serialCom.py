import serial
ser = serial.Serial()
ser.baudrate = 115200
ser.port = 'COM22'

ser.open()
message = 'Hello World!'
print('Sending message: ' + message)
ser.write(message.encode())

received = ser.read(len(message.encode()))

print('Message received: ' + received.decode())
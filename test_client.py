#!/usr/bin/env python3
import socket
import struct
import json

def test_login():
    # Create login request
    data = {'username': 'testuser', 'password': 'testpass'}
    payload = json.dumps(data).encode('utf-8')
    length = struct.pack('!I', len(payload))
    # Use 0x04 for LoginRequest to match simple_server.py expectation
    message = b'\x04' + length + payload

    print(f"Sending login request: {message.hex()}")

    # Connect to server
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 8080))

    # Send request
    sock.sendall(message)

    # Receive response
    response = sock.recv(1024)
    print(f"Received response: {response.hex()}")

    # Parse response
    if len(response) >= 5:
        msg_type = response[0]
        resp_length = struct.unpack('!I', response[1:5])[0]
        resp_payload = response[5:5+resp_length]
        print(f"Response type: {msg_type:02x}, length: {resp_length}")
        print(f"Response payload: {resp_payload.decode('utf-8')}")

        resp_data = json.loads(resp_payload.decode('utf-8'))
        print(f"Login result: {resp_data}")

    sock.close()

if __name__ == "__main__":
    test_login()
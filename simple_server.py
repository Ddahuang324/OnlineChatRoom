#!/usr/bin/env python3
"""
Simple Chat Server Demo
Listens on localhost:8080, accepts login requests, and responds with success.
"""

import socket
import struct
import json
import threading
import time

def handle_client(client_socket, client_address):
    print(f"New connection from {client_address}")
    try:
        buffer = b''
        message_count = 0
        while True:
            data = client_socket.recv(1024)
            if not data:
                print(f"No more data, total messages processed: {message_count}")
                break
            buffer += data
            print(f"Received {len(data)} bytes, buffer now: {len(buffer)} bytes")
            print(f"Raw buffer: {buffer.hex()}")

            # Try to parse messages from buffer
            while len(buffer) >= 5:  # Minimum message: 1 byte type + 4 bytes length
                if len(buffer) < 5:
                    break
                message_type = buffer[0]
                if len(buffer) < 9:  # Need at least type + length + some data
                    print(f"Buffer too small for message, type={message_type:02x}")
                    break
                length_bytes = buffer[1:5]
                length = struct.unpack('!I', length_bytes)[0]
                print(f"Message type: {message_type:02x}, expected payload length: {length}")

                total_needed = 5 + length
                if len(buffer) < total_needed:
                    print(f"Not enough data, need {total_needed}, have {len(buffer)}")
                    break

                payload = buffer[5:total_needed]
                buffer = buffer[total_needed:]

                message_count += 1
                print(f"Processing message {message_count}, type {message_type:02x} with payload: {payload}")

                if message_type == 0x04:  # LoginRequest
                    try:
                        data_str = payload.decode('utf-8')
                        print(f"Payload as string: {data_str}")
                        data = json.loads(data_str)
                        username = data.get('username', '')
                        password = data.get('password', '')

                        print(f"Login attempt: username='{username}', password='{password}'")

                        # Simple authentication: accept any non-empty username/password
                        if username and password:
                            response = {
                                "success": True,
                                "message": "Login successful",
                                "userId": f"user_{username}"
                            }
                            print("Login accepted")
                        else:
                            response = {
                                "success": False,
                                "message": "Invalid credentials",
                                "userId": ""
                            }
                            print("Login rejected")

                        # Serialize response
                        response_json = json.dumps(response).encode('utf-8')
                        response_length = struct.pack('!I', len(response_json))

                        # Send response (type 0x03 for LoginResponse)
                        response_data = b'\x03' + response_length + response_json
                        print(f"Sending response: {response_data.hex()}")
                        client_socket.sendall(response_data)
                        print("Sent login response")

                    except (json.JSONDecodeError, UnicodeDecodeError) as e:
                        print(f"Invalid JSON in login request: {e}")
                        # Send error response
                        error_response = {
                            "success": False,
                            "message": "Invalid request format",
                            "userId": ""
                        }
                        error_json = json.dumps(error_response).encode('utf-8')
                        error_length = struct.pack('!I', len(error_json))
                        client_socket.sendall(b'\x03' + error_length + error_json)

                else:
                    print(f"Unknown message type: {message_type:02x}")

    except Exception as e:
        print(f"Error handling client {client_address}: {e}")
        import traceback
        traceback.print_exc()
    finally:
        client_socket.close()
        print(f"Connection closed for {client_address}, total messages: {message_count}")

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(('127.0.0.1', 8080))
    server_socket.listen(5)

    print("Chat Server Demo started on 127.0.0.1:8080")
    print("Waiting for connections...")

    try:
        while True:
            client_socket, client_address = server_socket.accept()
            client_thread = threading.Thread(target=handle_client, args=(client_socket, client_address))
            client_thread.daemon = True
            client_thread.start()
    except KeyboardInterrupt:
        print("\nServer shutting down...")
    finally:
        server_socket.close()

if __name__ == "__main__":
    main()
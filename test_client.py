import socket

def send_cmd(sock, cmd):
    sock.sendall(cmd)
    response = sock.recv(4096)
    print(f"Sent: {cmd.strip()}")
    print(f"Received: {response.decode('utf-8').strip()}\n")

def main():
    host = "127.0.0.1"
    port = 6379
    
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((host, port))
        
        # 1. PING
        # *1\r\n$4\r\nPING\r\n
        send_cmd(s, b"*1\r\n$4\r\nPING\r\n")
        
        # 2. SET hello world
        # *3\r\n\r\nSET\r\n\r\nhello\r\n\r\nworld\r\n
        send_cmd(s, b"*3\r\n$3\r\nSET\r\n$5\r\nhello\r\n$5\r\nworld\r\n")
        
        # 3. GET hello
        # *2\r\n\r\nGET\r\n\r\nhello\r\n
        send_cmd(s, b"*2\r\n$3\r\nGET\r\n$5\r\nhello\r\n")
        
        s.close()
    except ConnectionRefusedError:
        print("Error: Could not connect to server. Is it running?")

if __name__ == "__main__":
    main()

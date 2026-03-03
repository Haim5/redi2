import socket
import threading
import time
import sys

def worker(thread_id, num_ops, host, port, results):
    """A worker function that SETs a key and then GETs it to verify correctness."""
    success_count = 0
    failure_count = 0
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((host, port))
            for i in range(num_ops):
                key = f"key-{thread_id}-{i}"
                expected_value = f"value-{i}"
                
                # SET command
                set_cmd = f"*3\r\n$3\r\nSET\r\n${len(key)}\r\n{key}\r\n${len(expected_value)}\r\n{expected_value}\r\n".encode('utf-8')
                s.sendall(set_cmd)
                s.recv(4096) # Consume the "+OK\r\n" response
                
                # GET command
                get_cmd = f"*2\r\n$3\r\nGET\r\n${len(key)}\r\n{key}\r\n".encode('utf-8')
                s.sendall(get_cmd)
                response = s.recv(4096).decode('utf-8', errors='ignore')

                # Simple RESP bulk string parser for "$<len>\r\n<value>\r\n"
                parts = response.split('\r\n', 2)
                if len(parts) > 1 and parts[0].startswith('$'):
                    actual_value = parts[1]
                    if actual_value == expected_value:
                        success_count += 1
                    else:
                        failure_count += 1
                else:
                    failure_count += 1 # Failed to parse response

    except Exception as e:
        print(f"Thread {thread_id}: Error - {e}")
        failure_count = num_ops - success_count # Assume remaining ops failed
    
    results[thread_id] = (success_count, failure_count)

def main():
    host = "127.0.0.1"
    port = 6379
    
    num_threads = 10      # Number of concurrent clients
    ops_per_thread = 100  # Number of SET/GET verification pairs per client

    results = {}
    threads = []
    
    print(f"Starting {num_threads} concurrent clients, each performing {ops_per_thread} SET/GET operations...")
    
    start_time = time.time()

    for i in range(num_threads):
        thread = threading.Thread(target=worker, args=(i, ops_per_thread, host, port, results))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

    end_time = time.time()
    
    total_successes = sum(res[0] for res in results.values())
    total_failures = sum(res[1] for res in results.values())
    
    total_ops = total_successes + total_failures
    duration = end_time - start_time
    ops_per_sec = (total_ops * 2) / duration # *2 because each op is a SET+GET pair

    print("\n--- Test Summary ---")
    print(f"Successful Verifications: {total_successes}")
    print(f"Failed Verifications:     {total_failures}")
    print("---")
    print(f"Total operations: {total_ops}")
    print(f"Total time: {duration:.2f} seconds")
    print(f"Operations per second: {ops_per_sec:.2f}")

if __name__ == "__main__":
    main()

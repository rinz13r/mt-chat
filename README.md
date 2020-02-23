# Chat Program in C

## Steps to run
```
mkdir bin
make all
```

1. Start the server
```
bin/server <port>
```

2. Connect a client
```
bin/client <server_ip> <port> <name> <channel>
```
channel must be an integer b/w 1-100

## Performance Measurement
```
chmod +x test/test.sh
./test/test.sh <#client> <#pclient>
```
This will print the average delay of message transfer to the console with <#client> listening clients and <#pclient> parallel sending clients.

#### Some Results

(single sender)

| #clients | delay (us)  |
|----------|-------------|
| 10       | 19370.67333 |
| 20       | 23308.862   |
| 30       | 29299.0337  |
| 40       | 29734.69487 |
| 50       | 29982.51261 |
| 60       | 29597.18246 |
| 70       | 30011.84403 |
| 80       | 37353.97513 |
| 90       | 44383.18298 |
| 100      | 39316.7413  |

(5 parallel senders)

| #clients | delay (us)  |
|----------|-------------|
| 10       | 16156.48    |
| 20       | 884.6805    |
| 30       | 5335.576667 |
| 40       | 27750.92475 |
| 50       | 62955.0802  |
| 60       | 119299.7718 |
| 70       | 163640.274  |
| 80       | 218698.0608 |
| 90       | 290843.3809 |
| 100      | 373537.9755 |

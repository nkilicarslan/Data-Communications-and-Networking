import pickle
import threading
import time
import signal

import socket
from sys import argv

updated_or_not = True


class Node:
    ip_address = "127.0.0.1" # IP address of the node
    def __init__(self, port_number):
        self.port_number = port_number # Port number of the node
        self.neighbours_port = []# Keep the all port numbers of the neighbors in a list
        self.neighbours_distance = [] # Keep the distance to all the neighbors in a list
        self.direct_neighbors = [] # Keep the direct neighbors in a list
        self.read_cost_file(str(port_number)+".costs")

    # read the cost file and store the information in the Node class
    def read_cost_file(self, cost_file):
        # open the cost file
        with open(cost_file, 'r') as file:
            # read the first line to get the total number of nodes in the network
            total_nodes = int(file.readline())
            # create the neighbor_distance list with the total number of nodes form the starting 3000 to 3000+total_nodes
            self.neighbours_port = [3000 + i for i in range(total_nodes)]
            # assign the distance to infinity for all the nodes and assign 0 for the current node
            self.neighbours_distance = [float('inf') if i != self.port_number - 3000 else 0 for i in range(total_nodes)]
            # read the rest of the lines to get the port number and distance to the neighbor
            for line in file:
                # split the line to get the port number and distance
                port, distance = line.split()
                # store the port number and distance in the neighbour_distance list
                self.neighbours_distance[int(port) - 3000] = int(distance)
                self.direct_neighbors.append(int(port))
    # send the distance vector to the neighbors
    def belmann_ford(self, distance_vector, neighbor_list):
        # iterate over the distance vector and find the index which value is 0
        index = -1
        for i in range(len(distance_vector)):
            if distance_vector[i] == 0:
                # get the index of the neighbor
                index = i
                break
        # update the distance vector
        for i in range(len(distance_vector)):
            if distance_vector[i] + self.neighbours_distance[index] < self.neighbours_distance[i]:
                self.neighbours_distance[i] = distance_vector[i] + self.neighbours_distance[index]
                global updated_or_not
                updated_or_not = True

    # send the distance vector to the neighbors wait 5 seconds if no update happens then close the connection
    def send_distance_vector(self):
        # in order to send the data we have to wait to listeners are ready to receive the data
        time.sleep(0.4)
        # calculate the end time to close the connection after 5 seconds
        end_time = time.time() + 5
        # get the global flag
        global updated_or_not
        # check the 5 seconds are over or not
        while time.time() < end_time:
            if updated_or_not == True:
                for neighbor in self.direct_neighbors:
                    # create a socket
                    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    s.connect((Node.ip_address, neighbor))
                    # send both the distance vector to the neighbor
                    appended_list = [self.neighbours_distance, self.neighbours_port]
                    # with using pickle we can send the list
                    s.send(pickle.dumps(appended_list))
                    # close the connection
                    s.close()
                updated_or_not = False
                end_time = time.time() + 5


    # listen for the distance vector from the neighbors with infinite timeout in the while loop
    def listen_for_distance_vector(self):
        # create a socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # bind the socket to the port number of the node
        s.bind((Node.ip_address, self.port_number))
        # listen for the connection
        s.listen()
        # set the timeout to 5 seconds
        s.settimeout(5)
        try:
            while True:
                # accept the connection
                conn, addr = s.accept()
                # receive the data from the neighbor
                data = conn.recv(1024)
                # print the distance vector
                get_the_data = pickle.loads(data)
                # get the distance vector and neighbor list from the received data
                self.belmann_ford(get_the_data[0], get_the_data[1])
                # close the connection
                conn.close()
                # set the timeout to 5 seconds again to restart the time
                s.settimeout(5)
        except Exception as e:
            # create a lock
            lock = threading.Lock()
            # acquire the lock
            lock.acquire()
            # close the socket
            s.close()
            time.sleep((self.port_number-3000)/100)
            # print the distance vector of the node as in the stated as in the assignment.pdf
            for i in range(len(self.neighbours_distance)):
                print(str(self.port_number) + " - " + str(self.neighbours_port[i]) + " | " + str(self.neighbours_distance[i]))
            print("")
            # release the lock
            lock.release()
            return


node = Node(int(argv[1]))
# create 2 threads to send and receive the distance vector
send_thread = threading.Thread(target=node.send_distance_vector)
receive_thread = threading.Thread(target=node.listen_for_distance_vector)
# start the threads
send_thread.start()
receive_thread.start()
# wait for the threads to finish
send_thread.join()
receive_thread.join()

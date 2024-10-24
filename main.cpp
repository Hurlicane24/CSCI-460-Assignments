#include <iostream>
#include <random>
#include <pthread.h>
#include <semaphore.h>
#include <fstream>

//Initialize random device, seed engine, and create uniform data distribution of ints >= 0 and <= 49, and >= 1 <= 50
std::random_device rando;
std::mt19937 gen(rando());

//Distribution for odd numbers
std::uniform_int_distribution<> dist1(0, 49);

//Distribution for even numbers
std::uniform_int_distribution<> dist2(1, 50);

//Initialize semaphores
sem_t buffer_full;
sem_t buffer_access;

//Create and open files
std::ofstream prod1file("producer1_out.txt");
std::ofstream prod2file("producer2_out.txt");
std::ofstream cons1file("consumer1_out.txt");
std::ofstream cons2file("consumer2_out.txt");

//Initialize mutex for printing
pthread_mutex_t print_mutex;

//Node struct
struct Node {
    int data;
    Node *next;
    Node *prev;
};

//Buffer
class DoublyLinkedList {
private:
    Node *head;
    Node *tail;
    int max_size;
    int current_size;

public:

    //Constructor
    DoublyLinkedList() {
        head = nullptr;
        tail = nullptr;
        max_size = 50;
        current_size = 0;
    }

    //This method prints the current state of the list
    void printList() {

        //If list is empty, notify user
        if(head == nullptr) {
            std::cout << "[]" << std::endl;
        }

        //Else, print nodes in the list
        else {
            Node *current = head;
            std::cout << "[";
            while (current != nullptr) {
                if(current->next == nullptr) {
                    std::cout << current->data << "]";
                }
                else {
                    std::cout << current->data << ", ";
                }
                current = current->next;
            }
            std::cout << std::endl;
        }
    }

    //This method writes the buffer state to the output files
    void write(bool modification, std::ofstream &file) {
        //If the buffer has not yet been modified, use these write instructions
        if(modification == false) {
            file << "Before Modification: ";

            if(head == nullptr) {
                file << "[]" << std::endl;
            }
            else {
                Node *current  = head;
                file << "[";
                while(current != nullptr) {
                    if(current->next == nullptr) {
                        file << current->data << "]";
                    }
                    else {
                        file << current->data << ", ";
                    }
                    current = current->next;
                }
                file << std::endl;
            }
        }

        //If the buffer has been modified, use these write instructions
        else {
            file << "After Modification: ";

            if(head == nullptr) {
                file << "[]" << std::endl;
            }
            else {
                Node *current  = head;
                file << "[";
                while(current != nullptr) {
                    if(current->next == nullptr) {
                        file << current->data << "]";
                    }
                    else {
                        file << current->data << ", ";
                    }
                    current = current->next;
                }
                file << std::endl;
            }
        }

    }

    //This method inserts a node into the correct location in the list
    void Insert(int data) {
        Node *node = new Node;
        node->data = data;

        //If Linked List is empty, insert node into the head/tail
        if(head == nullptr) {
            node->next = nullptr;
            node->prev = nullptr;
            head = tail = node;
            current_size++;
        }

        //If non-empty, insert node into correct location
        else {

            //If list is full, reject insert operation (SHOULD NOT RUN BECAUSE OF SEMAPHORE)
            if(current_size == max_size) {
                std::cout << "List is full!" << std::endl;
            }

            //If node is smaller than everything in the list, make it the new head
            else if(node->data <= head->data) {
                head->prev = node;
                node->next = head;
                node->prev = nullptr;
                head = node;
                current_size++;
            }

            //If node is greater than everything in the list, make it the new tail
            else if(node->data >= tail->data) {
                node->next = nullptr;
                node->prev = tail;
                tail->next = node;
                tail = node;
                current_size++;
            }

            //Else, search list for correct position and insert
            else {
                Node *current = head;
                while(current != nullptr) {
                    if(node->data >= current->data && node->data <= current->next->data) {
                        node->next = current->next;
                        node->prev = current;
                        current->next->prev = node;
                        current->next = node;
                        current_size++;
                        break;
                    }

                    current = current->next;
                }
            }
        }
    };

    //This method deletes the head node
    void deleteHead() {

        //If List is empty, do nothing
        if(head == nullptr) {
            return;
        }
        Node *temp = head;
        head = head->next;

        //If the list is now empty after removal, don't update head->prev
        if(head != nullptr) {
            head->prev = nullptr;
        }

        //Free node
        delete(temp);
        current_size--;
        sem_post(&buffer_full);
    }

    //This method returns the current size of the buffer
    int getSize() {
        return(current_size);
    }

    //This method returns the head of the buffer
    int getHead() {
        if(head == nullptr) {
            return(-1);
        }
        else{
            return(head->data);
        }
    }

};

//Initialize buffer
DoublyLinkedList Buffer;

//Producer 1 Thread
void* producer1func(void* arg) {
    pthread_mutex_lock(&print_mutex);
    std::cout << "Producer 1 started!" << std::endl;
    pthread_mutex_unlock(&print_mutex);

    while(true) {
        int randomOdd = dist1(gen) * 2 + 1; //Generate random odd integer between 1 and 100 (inclusive)

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 1 Generated: " << randomOdd << std::endl;
        pthread_mutex_unlock(&print_mutex);

        sem_wait(&buffer_full); //Wait if buffer is full
        sem_wait(&buffer_access); //Wait if buffer is being accessed

        bool modification = false;
        Buffer.write(modification, prod1file); //Write buffer state before modification

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 1 accessed buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);

        Buffer.Insert(randomOdd); //Insert generated node into the buffer

        modification = true;
        Buffer.write(modification, prod1file); //Write buffer state after modification

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 1 Inserted " << randomOdd << std::endl;
        std::cout << "Current Buffer Size: " << Buffer.getSize() << std::endl;
        std::cout << "Current State of Buffer: ";
        Buffer.printList();
        pthread_mutex_unlock(&print_mutex);

        sem_post(&buffer_access); //Give up control of the buffer

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 1 left the buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);
    }
    pthread_mutex_lock(&print_mutex);
    std::cout << "Producer 1 has been terminated" << std::endl;
    pthread_mutex_unlock(&print_mutex);
    pthread_exit(nullptr);
}

//Producer 2 Thread
void* producer2func(void* arg) {
    pthread_mutex_lock(&print_mutex);
    std::cout << "Producer 2 started!" << std::endl;
    pthread_mutex_unlock(&print_mutex);

    while(true) {
        int randomEven = dist2(gen) * 2; //Generate a random even integer between 1 and 100 (inclusive)

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 2 Generated: " << randomEven << std::endl;
        pthread_mutex_unlock(&print_mutex);

        sem_wait(&buffer_full); //Wait if buffer is full
        sem_wait(&buffer_access); //Wait if buffer is being accessed

        bool modification = false;
        Buffer.write(modification, prod2file); //Write state of buffer before modification

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 2 accessed buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);

        Buffer.Insert(randomEven); //Insert generated node into the buffer

        modification = true;
        Buffer.write(modification, prod2file); //Write state of buffer after modification

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 2 inserted: " << randomEven << std::endl;
        std::cout << "Current Buffer Size: " << Buffer.getSize() << std::endl;
        std::cout << "Current State of Buffer: ";
        Buffer.printList();
        pthread_mutex_unlock(&print_mutex);

        sem_post(&buffer_access); //Give up access of the buffer

        pthread_mutex_lock(&print_mutex);
        std::cout << "Producer 2 left the buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);

    }
    pthread_mutex_lock(&print_mutex);
    std::cout << "Producer 2 has been terminated" << std::endl;
    pthread_mutex_unlock(&print_mutex);
    pthread_exit(nullptr);
}

//Consumer 1 Thread
void* consumer1func(void* arg) {
    pthread_mutex_lock(&print_mutex);
    std::cout << "Consumer 1 started!" << std::endl;
    pthread_mutex_unlock(&print_mutex);

    while(true) {
        int toRemove;

        sem_wait(&buffer_access); //Attempt to gain access to buffer

        pthread_mutex_lock(&print_mutex);
        std::cout << "Consumer 1 accessed buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);

        toRemove = Buffer.getHead(); //Get head of buffer
        bool removed = false;

        //If head contains an odd int, and head is not nullptr, remove head
        if(toRemove != -1) {
            if(toRemove % 2 != 0) {
                bool modification = false;
                Buffer.write(modification, cons1file); //Write state of buffer before modification
                Buffer.deleteHead(); //Consume head
                modification = true;
                Buffer.write(modification,cons1file); //Write state of buffer after modification
                removed = true;

                pthread_mutex_lock(&print_mutex);
                std::cout << "Consumer 1 Removed: " << toRemove << std::endl;
                pthread_mutex_unlock(&print_mutex);
            }
        }

        //For debugging
        pthread_mutex_lock(&print_mutex);
        if(removed == false) {
            std::cout << "Consumer 1 Removed: Nothing" << std::endl;
        }
        std::cout << "Current Buffer Size: " << Buffer.getSize() << std::endl;
        std::cout << "Current State of Buffer: ";
        Buffer.printList();
        pthread_mutex_unlock(&print_mutex);

        sem_post(&buffer_access); //Give up buffer access

        pthread_mutex_lock(&print_mutex);
        std::cout << "Consumer 1 left the buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);
    }
    pthread_mutex_lock(&print_mutex);
    std::cout << "Consumer 1 has been terminated" << std::endl;
    pthread_mutex_unlock(&print_mutex);
    pthread_exit(nullptr);
}

//Consumer 2 Thread
void* consumer2func(void* arg) {
    pthread_mutex_lock(&print_mutex);
    std::cout << "Consumer 2 started!" << std::endl;
    pthread_mutex_unlock(&print_mutex);

    while(true) {
        int toRemove;

        sem_wait(&buffer_access); //Attempt to get buffer access

        //For debugging
        pthread_mutex_lock(&print_mutex);
        std::cout << "Consumer 2 accessed buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);

        toRemove = Buffer.getHead(); //Get head
        bool removed = false;

        //If head contains an even int, and head is not nullptr, remove head
        if(toRemove != -1) {
            if(toRemove % 2 == 0) {
                bool modification = false;
                Buffer.write(modification, cons2file); //Write state of buffer before modification
                Buffer.deleteHead(); //Consume head
                modification = true;
                Buffer.write(modification, cons2file); //Write state of buffer after modification
                removed = true;

                pthread_mutex_lock(&print_mutex);
                std::cout << "Consumer 2 Removed: " << toRemove << std::endl;
                pthread_mutex_unlock(&print_mutex);
            }
        }

        //For debugging
        pthread_mutex_lock(&print_mutex);
        if(removed == false) {
            std::cout << "Consumer 2 Removed: Nothing" << std::endl;
        }
        std::cout << "Current Buffer Size: " << Buffer.getSize() << std::endl;
        std::cout << "Current State of Buffer: ";
        Buffer.printList();
        pthread_mutex_unlock(&print_mutex);

        sem_post(&buffer_access); //Give up access of the buffer

        pthread_mutex_lock(&print_mutex);
        std::cout << "Consumer 2 left the buffer" << std::endl;
        pthread_mutex_unlock(&print_mutex);
    }

    pthread_mutex_lock(&print_mutex);
    std::cout << "Consumer 2 has been terminated" << std::endl;
    pthread_mutex_unlock(&print_mutex);
    pthread_exit(nullptr);
}

int main() {

    //Initialize semaphores
    sem_init(&buffer_full, 0, 50);
    sem_init(&buffer_access, 0, 1);

    //Initialize threads and thread IDs
    pthread_t producer1, producer2, consumer1, consumer2;
    int producer1ID = 1;
    int producer2ID = 2;
    int consumer1ID = 3;
    int consumer2ID = 4;

    //Spawn producers and consumers
    pthread_create(&producer1, nullptr, producer1func, (void*)&producer1ID);
    pthread_create(&producer2, nullptr, producer2func, (void*)&producer2ID);
    pthread_create(&consumer1, nullptr, consumer1func, (void*)&consumer1ID);
    pthread_create(&consumer2, nullptr, consumer2func, (void*)&consumer2ID);

    //Let threads run until completion
    pthread_join(producer1, nullptr);
    pthread_join(producer2, nullptr);
    pthread_join(consumer1, nullptr);
    pthread_join(consumer2, nullptr);

    std::cout << "Finished" << std::endl;

    //Close files
    prod1file.close();
    prod2file.close();
    cons1file.close();
    cons2file.close();

    //Destroy semaphores and mutex
    sem_destroy(&buffer_full);
    sem_destroy(&buffer_access);
    pthread_mutex_destroy(&print_mutex);

    return(0);
}
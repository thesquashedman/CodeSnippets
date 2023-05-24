
using namespace std;

struct thread_message //Struct for sending the socket
{
  UdpSocket* sock;
};
int ackNumber; // var that holds the acks gotten from the server. -2 means no ack, -1 means timer interupt, and everything else is an ack number
int currentNumber; //Current number for the client
pthread_mutex_t mutex; //Mutex to synchronize the threads
pthread_cond_t cond; //Signal from timeout thread or acknowledgment thread
pthread_cond_t cond2; //Extra condition so that if the program times out it isn't overwritten by receiving an ack

void* waitForAcknowledgment(void* temp) //Thread that handles acknowledgments
{
  //cerr << "hello" << endl;
  //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //Added to see if I could cancel it, turns out I can't cancel it while it's blocking so it's unused
  while(true)
  {
    //pthread_testcancel();

    //Gets acknowledgment
    thread_message* message = (thread_message*) temp;
    int ack[MSGSIZE/4];
    message->sock->recvFrom( (char *) ack, MSGSIZE);
   
    pthread_mutex_lock(&mutex); //Waits to grab the mutex before writing the ack.
   
    if(ackNumber == -1) //If there was a timeout, we want it to take priority, so wait for resend before setting ackNumber
    {
      pthread_cond_init(&cond2, NULL);
      pthread_cond_wait(&cond2, &mutex);
    }
    ackNumber = ack[0]; //Write ack
    pthread_cond_signal(&cond); //Signal that there was an ack so that the main program can grab the mutex.
    pthread_mutex_unlock(&mutex);
   
  }
}

void* Timeout(void* temp) //Thread that handles timeout, sleeps for 1500 usecs and then sends signal if not canceled. Done like this so that the main thread isn't busy waiting
{
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //Not sure I need this but if I do makes the thread cancelable.
  usleep(1500); //Sleeps for 1500 usecs
  pthread_testcancel(); //Checks if the thread has been canceled
 
  pthread_mutex_lock(&mutex); //Grabs mutex and writes -1 to ackNumber to show that there has been a timeout
  ackNumber = -1;
  pthread_cond_signal(&cond); //signals to main thread that there has been a timeout

  pthread_mutex_unlock(&mutex);
  return NULL;
}
int clientSlidingWindow( UdpSocket &sock, const int max, int message[], int windowSize )
{
 
  int pipeFD[2];
  pipe(pipeFD);
  int pid = fork(); //Create a new process for this function. This is due to not being able to cancel the blocking ack thread, so is only way to release that thread
  if(pid == 0)
  {
    cerr << "client sliding window test: " << windowSize << endl;
    int inTransit = 0; //Number of messages waiting to be acked
    int complete = 0; //Number of messages acked
    int retransmits = 0;
    ackNumber = -2; //set ackNumber to default
    struct thread_message* data;

    //Creates ack thread  
    pthread_t newThread;
   
    data = new thread_message;
    data->sock = &sock;
    int threadNumber = pthread_create(&newThread, NULL, waitForAcknowledgment, (void*) data);
    if(threadNumber != 0)
    {
      cerr << "thread error" << endl;
    }
   
    cerr <<"thread" << threadNumber << endl;

    //While we don't have acknowledgments for all the messages sent
    while(complete < max)
    {  
      //While there are less messages in transit than the window size and the messages are within max, write messages to server
      while(inTransit < windowSize && complete + inTransit < max)
      {
        message[0] =  complete + inTransit;
        sock.sendTo( (char *) message, MSGSIZE);
        inTransit++;
      }

      //Create timer thread.
      pthread_t timeoutThread;
      int threadNumberT = pthread_create(&timeoutThread, NULL, Timeout, NULL);
     
      pthread_mutex_lock(&mutex);//Grabs mutex
      while(true)
      {
        pthread_cond_init(&cond, NULL);
        pthread_cond_signal(&cond2); //The special condition for
        pthread_cond_wait(&cond, &mutex); //Waits for signal from
        if(ackNumber < 0)  //Timeout, resend
        {
          retransmits++;
          cerr << "retransmit " << complete << endl;
          message[0] =  complete;
          sock.sendTo( (char *) message, MSGSIZE); //Retransmit
          ackNumber = -2;
          int threadNumberT = pthread_create(&timeoutThread, NULL, Timeout, NULL);
        }
        else if (ackNumber > complete && ackNumber <= complete + windowSize)//Send next if received the a non duplicate ack
        {
          inTransit -= (ackNumber - complete);
          complete = ackNumber;
          pthread_cancel(timeoutThread);
          break;
        }
        else //Duplicate, wait for timeout or correct ack.
        {
          cerr << "DUPLICATE, ack = " << ackNumber << " complete = " << complete << endl;  
        }
      }
      pthread_mutex_unlock(&mutex);
    }
    delete data;

    //Send -1 message 10 times to let the server know it can stop. Assume 10 is enough.
    for(int i = 0; i < 10; i++)
    {
      message[0] =  -1;
      sock.sendTo( (char *) message, MSGSIZE);
    }
    //Write retransmits to pipe
    close(pipeFD[0]);
    int out = dup(1);
    dup2(pipeFD[1], 1);
    write(pipeFD[1], &retransmits, sizeof(int));
    close(pipeFD[1]);
    dup2(out, 1);
    //return retransmits;
    exit(0);
  }
  else
  {
    //Parent process
    wait(NULL);
    close(pipeFD[1]); //Get restransmits from child process and return
    int in = dup(0);
    dup2(pipeFD[0], 0);
    //char buffer[sizeof(int)];
    int retransmits;
    read(pipeFD[0], &retransmits, sizeof(int));
    close(pipeFD[0]);
    dup2(in, 0);
    return retransmits;
  }
}
//Sliding window server
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[], int windowSize )
{
  cerr << "server early retrans: " << windowSize << endl;
  bool arr[max] = { false };
  // receive message[] max times
  int i = 0;
  while (true) {
    sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive
    if(message[0] == -1 & i >= max) //If message is -1, that means the client has received all acks so the server can end
    {
      break;
    }
    if(message[0] != i)
    {
      cerr << message[0] << " expected: " << i<< endl;
    }
    //Write to the buffer
    arr[message[0]] = true;
    if(message[0] == i)//When receiving the message it is waiting for, move along the buffer until it finds a message it doesn't have
    {
      while(i < max && arr[i] == true)
      {
        i++;
      }
    }
    //Send ack with message that it is waiting for
    message[0] = i;
   
    sock.ackTo((char*) message, MSGSIZE);
  }
  return;
 
} 
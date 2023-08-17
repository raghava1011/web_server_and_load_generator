/* run using: ./load_gen localhost <server port> <number of concurrent users>
   <think time (in s)> <test duration (in s)> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>

#include <pthread.h>
#include <sys/time.h>

FILE *log_file,*data;
int time_up;

// user info struct
struct user_info {
  // user id
  int id;

  // socket info
  int portno;
  char *hostname;
  float think_time;

  // user metrics
  int total_count;
  float total_rtt;
};

// error handling function
void error(char *msg) {
  perror(msg);
  exit(0);
}

// time diff in seconds
float time_diff(struct timeval *t2, struct timeval *t1) {
  return (t2->tv_sec - t1->tv_sec) + (t2->tv_usec - t1->tv_usec) / 1e6;
}

// user thread function
void *user_function(void *arg) {
  /* get user info */
  struct user_info *info = (struct user_info *)arg;

  int sockfd, n;
  char buffer[2048];
  struct timeval start, end;

  struct sockaddr_in serv_addr;
  struct hostent *server;
  info->think_time *= 1000000;

  while (1) {
    /* start timer */
    gettimeofday(&start, NULL);

    /* TODO: create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    /* TODO: set server attrs */
    server = gethostbyname(info->hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port = htons(info->portno);
    /* TODO: connect to server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    /* TODO: send message to server */
    bzero(buffer, 2048);
    strcpy(buffer,"GET /apart1/flat1/index.html HTTP/1.1");
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");
    bzero(buffer, 2048);
    /* TODO: read reply from server */
    n = read(sockfd, buffer, 2048);
    if (n < 0)
        error("ERROR reading from socket");
    //printf("Server response: %s\n", buffer);
    /* TODO: close socket */
    close(sockfd);
    /* end timer */
    gettimeofday(&end, NULL);

    /* if time up, break */
    if (time_up)
      break;

    /* TODO: update user metrics */
    info->total_count +=1;
    info->total_rtt += time_diff(&end,&start);

    /* TODO: sleep for think time */
    usleep(info->think_time);
  }

  /* exit thread */
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  int user_count, portno, test_duration;
  float think_time;
  char *hostname;

  if (argc != 6) {
    fprintf(stderr,
            "Usage: %s <hostname> <server port> <number of concurrent users> "
            "<think time (in s)> <test duration (in s)>\n",
            argv[0]);
    exit(0);
  }

  hostname = argv[1];
  portno = atoi(argv[2]);
  user_count = atoi(argv[3]);
  think_time = atof(argv[4]);
  test_duration = atoi(argv[5]);

  printf("Hostname: %s\n", hostname);
  printf("Port: %d\n", portno);
  printf("User Count: %d\n", user_count);
  printf("Think Time: %f s\n", think_time);
  printf("Test Duration: %d s\n", test_duration);

  /* open log file */
  log_file = fopen("load_gen.log","w");


  pthread_t threads[user_count];
  struct user_info info[user_count];
  struct timeval start, end;

  /* start timer */
  gettimeofday(&start, NULL);
  time_up = 0;
  for (int i = 0; i < user_count; ++i) {
    /* TODO: initialize user info */
    info[i].id = i;
    info[i].portno = portno;
    info[i].hostname = hostname;
    info[i].think_time = think_time;
    info[i].total_count = 0;
    info[i].total_rtt = 0;

    /* TODO: create user thread */
    int rc = pthread_create(&threads[i],NULL,user_function,(void *)&info[i]);
    fprintf(log_file,"created thread %d\n",i);
  }

  /* TODO: wait for test duration */
  sleep(test_duration);

  fprintf(log_file,"Woke up\n");

  /* end timer */
  time_up = 1;
  gettimeofday(&end, NULL);

  /* TODO: wait for all threads to finish */
  for(int i=0;i<user_count;i++)
    pthread_join(threads[i],NULL);

  /* TODO: print results */
  int total_request;
  float total_res_time;
  for(int i=0;i<user_count;i++)
  {
    total_res_time += info[i].total_rtt;
    total_request += info[i].total_count;
  }
  float avg_throughput, avg_res_time;
  avg_throughput = total_request/test_duration;
  avg_res_time = total_res_time/total_request;
  printf("user count = %d  think time = %fsec  test duration = %dsec  avg throughput = %f  avg res time =%f\n",user_count,think_time,test_duration,avg_throughput,avg_res_time);

  data = fopen("data.txt","a+");
  fprintf(data,"user count = %d  think time = %fsec  test duration = %dsec  avg throughput = %f  avg res time =%f\n",user_count,think_time,test_duration,avg_throughput,avg_res_time);

  /* close log file */
  fclose(log_file);
  fclose(data);

  return 0;
}
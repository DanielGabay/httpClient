#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ctype.h>

#define BUFF_SIZE 1024
#define DEFAULT_PORT "80"
#define IS_A_NUMBER 0
#define NOT_A_NUMBER -1
#define NOT_FOUND -1
#define BODY_FLAG "-p"
#define PARAMETERS_FLAG "-r"
#define INVALID -1
#define VALID 1

/*
@author: Danil Gabay
Date: 28/12/18

"Ex2" http client
==Description==
 This program handle user input from terminal and construct HTTP/1.0 request.
 There are 2 types of Request that can be executed by this program: GET && POST requests.
 Conditions:
    1)User must provide a valid URL (including http:// prefix) in order to be able to connect to server.
    2)User can provide -p flag and afterwords the text (ex. -p helloWorld) to make POST Request.
      the text must contain no spaces and special chars and placed right after -p flag.
    3)User can provide -r flag and afterwords a number n that points out how many parameters to include and than the parameters.
      each parameter must be in a key=value form (example: age=25)
 Note: The flags and url can be provided at any order, but must comply all conditions above.
 Any input that not imply the conditions, will cause the program to exit with usage msg.
 Any failure within sys.calls will cause the program to exit and prints the sys.call problem.
 Url form example: http://Host[:port]/Filepath (when port and filepath is optional)
 */

/*this struct holds all information in order to build an HTTP request*/
typedef struct Request{
    char * host;
    char * port;
    char * path;
    char * body;
    char * parameters;
    char * req_string;
}Request;

void init_request(Request *r);
void analyzeArgv(int argc, char *argv[],Request * r);
void parseUrl(char * s,Request * r);
int validateParameters(int argc, char *argv[], Request * r, int p_index);
int isValidParameter(char * s);
void buildParametersString(char *argv[], Request * r, int firstParamIndex, int n);
int connectToServer(Request * request);
int is_a_number(char * str);
int find(char ** arr, char * value,int length,int startIndex);
int charFirstOccur(char *s, char c);
void GET_request(Request * r);
void POST_request(Request * r);
void printUsageError();
void freeRequest(Request * r);
void endProgram(Request * r);
void endProgramNoUsage(Request * r);

/*init all fields to NULL*/
void init_request(Request *r){
    r->host = NULL;
    r->port = NULL;
    r->path = NULL;
    r->body = NULL;
    r->parameters = NULL;
    r->req_string = NULL;
}

/*
 * this functions takes a string that supposed to be an url, and parse it
 * into host,port and path.
 * the values are stored at Request * r attributes.
 * */
void parseUrl(char * url,Request * r){
    char * http = "http://";
    int http_len = strlen(http);
    if(memcmp(url,http,http_len) != 0) //end program if url don't start with http
        endProgram(r);
    char * no_http_url = NULL, * colon_p = NULL, * slash_p = NULL;
    int url_len = strlen(url);
    no_http_url = (char *)malloc(sizeof(char) * (url_len - http_len)+1); //temp string holding url with no http://prefix
    if(no_http_url == NULL){
        perror("malloc\n");
        endProgramNoUsage(r);
    }
    bzero(no_http_url,url_len-http_len+1);
    strncpy(no_http_url, url + http_len, url_len - http_len); //copy url no http:// prefix
    colon_p = strchr(no_http_url,':'); //pointer to the first ":"
    slash_p = strchr(no_http_url, '/'); // pointer to first "/"
    int port_len = 0,path_len = 0,host_len = 0;
    /*extract port*/
    if(colon_p == NULL) { // no ":" -> port should be DEFAULT PORT (80)
        r->port = (char *) malloc(sizeof(char) * sizeof(DEFAULT_PORT) + 1);
        if (r->port == NULL) {
            perror("malloc\n");
            free(no_http_url);
            endProgramNoUsage(r);
        }
        bzero(r->port, sizeof(DEFAULT_PORT) + 1);
        strcpy(r->port, DEFAULT_PORT);
    }
    else { // found ":"
        if(slash_p == NULL) // no "/"
            port_len = strlen(colon_p);
        else  //calculate port size between ':' to the first '/'
            port_len = strlen(colon_p) - strlen(slash_p);
        if(port_len == 1){ //only ":" prvided but no port number
            free(no_http_url);
            endProgram(r);
        }
        else { //both ":" and "/" found
            if(port_len <= 0) { //means that ":" is part of the path, and port should be DEFAULT_PORT(80)
                r->port = (char *)malloc(sizeof(char) * sizeof(DEFAULT_PORT) + 1);
                if(r->port == NULL){
                    perror("malloc\n");
                    free(no_http_url);
                    endProgramNoUsage(r);
                }
                bzero(r->port,sizeof(DEFAULT_PORT) + 1);
                strcpy(r->port, DEFAULT_PORT);
            }
            else { //means that ":" is before "/"
                r->port = (char *) malloc(sizeof(char) * port_len);
                if (r->port == NULL) {
                    perror("malloc\n");
                    free(no_http_url);
                    endProgramNoUsage(r);
                }
                bzero(r->port, port_len);
                strncpy(r->port, colon_p + 1, port_len - 1);
            }
        }
        if(is_a_number(r->port) == NOT_A_NUMBER) {
            free(no_http_url);
            endProgram(r);
        }
    }
    /*extract path*/
    if (slash_p != NULL){
        path_len = strlen(slash_p);
        r->path = (char *) malloc(sizeof(char) * path_len+1);
        if(r->path == NULL){
            perror("malloc\n");
            free(no_http_url);
            endProgramNoUsage(r);
        }
        bzero(r->path, path_len+1);
        strncpy(r->path, slash_p, path_len);
    }
    /*extract host*/
    int temp_len = strlen(no_http_url);
    host_len = temp_len - port_len-path_len;
    r->host = (char*)malloc(sizeof(char)*host_len+1);
    if(r->host == NULL){
        perror("malloc\n");
        free(no_http_url);
        endProgramNoUsage(r);
    }
    bzero(r->host, host_len+1);
    strncpy(r->host, no_http_url, host_len);
    free(no_http_url);
}

/*
 * this function analyze user arguments (from argv) and store the information at Request struct.
 * in case of wrong inputs, the program is going to end with usage msg.
 */
void analyzeArgv(int argc, char *argv[], Request * r){
    int host_i = -1;
    int body_i = find(argv,BODY_FLAG,argc,0); //will hold the first index of -p flag if provided
    int parameters_i = find(argv,PARAMETERS_FLAG,argc,0); //will hold the first index of -r flag if provided
    if(body_i != NOT_FOUND) {
        if(body_i+1 >= argc) //no text privded
            endProgram(r);
        r->body = (char *)malloc(strlen(argv[body_i+1]) + 1);
        if(r->body == NULL){
            perror("malloc\n");
            endProgramNoUsage(r);
        }
        bzero(r->body,strlen(argv[body_i+1]) + 1);
        strcpy(r->body,argv[body_i+1]); //copy text (at body_i+1 index)
    }
    int num_of_parameters = 0;
    if(parameters_i != NOT_FOUND) {
        if(parameters_i == body_i + 1) { //found -r as the text of -p. search for another -r flag
            parameters_i = find(argv, PARAMETERS_FLAG, argc, parameters_i + 1);
            if (parameters_i == NOT_FOUND) //if dont found another -r, end program
                endProgram(r);
        }
        if(parameters_i+1 >= argc) //-r is in the last index
            endProgram(r);
        else
            num_of_parameters = validateParameters(argc,argv,r,parameters_i);
    }
    int i;
    /*this loop finds the place of the URL by eliminating -p and -r cells if given.
     * if there are more than 1 cell that can be URL -> end program
     */
    for(i = 1; i < argc; i++){
        if(body_i != NOT_FOUND && (i == body_i || i == body_i+1))
            continue;
        if(parameters_i != NOT_FOUND && (i >= parameters_i && i <= parameters_i +num_of_parameters+1))
            continue;
        if(host_i != -1) //means that there are 2 or more cells that considered as url -> usage problem
            endProgram(r);
        host_i = i;
    }
    if(host_i == -1) //no url
        endProgram(r);
    parseUrl(argv[host_i],r); //parse URL into host, port, path
}

/*this function validate all parameters. if all parameters are valid, in returns the how many parameters recived.*/
int validateParameters(int argc, char *argv[], Request * r, int p_index){
    if(is_a_number(argv[p_index+1]) == NOT_A_NUMBER) //check if 'n' after -r flag is a number
        endProgram(r);
    int n = atoi(argv[p_index+1]);
    if(p_index + n + 1 >= argc) //check argv overflow
        endProgram(r);
    for(int i = p_index+2; i < p_index+2+n; i++)
        if(isValidParameter(argv[i]) == INVALID)
            endProgram(r);
    buildParametersString(argv,r,p_index+2,n);
    return n;
}

/*this function receive string and determine if its a in a valid parameter form (example: age=23)*/
int isValidParameter(char * s){
    int s_len = strlen(s);
    if(s_len < 3) //at lest 3 chars needed , 'a=b' for example
        return INVALID;
    int index = charFirstOccur(s,'=');
    if(index == 0 || index == s_len-1 || index == INVALID) // first/last sign is "=" or no "=" sign ->invalid
        return INVALID;
    return VALID;
}

/*this function receive parameters places in argv(by index) and construct the paramerters string form*/
void buildParametersString(char *argv[], Request * r, int firstParamIndex, int n){
    if(n == 0)
        return;
    int i = firstParamIndex;
    int size = 0;
    for(i;i < firstParamIndex+n;i++){
        size += strlen(argv[i]);
    }
    size += n+1; //for the first '?' and, each '&' between parameters and the last '\0'
    r->parameters = (char*)malloc(sizeof(char)*size);
    bzero(r->parameters,size);
    const char * c1 = "?";
    const char * c2 = "&";
    strcat(r->parameters,c1);
    i = firstParamIndex;
    for(i; i < firstParamIndex+n-1; i++){
        strcat(r->parameters,argv[i]);
        strcat(r->parameters,c2);
    }
    strcat(r->parameters,argv[i]);
}

/*this function tries to connect to server by hostname & port.
 *in case of failure -> exit and print error
 *o.w when connection is established -> return sockfd.
 */
int connectToServer(Request * request){
    int port = atoi(request->port);
    struct sockaddr_in sockaddr;
    struct hostent *hp;
    int sockfd = -1;

    if((hp = gethostbyname(request->host)) == NULL){
        herror("gethostbyname\n");
        endProgramNoUsage(request);
    }
    bcopy(hp->h_addr, &sockaddr.sin_addr, hp->h_length);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    sockfd = socket(PF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        perror("socket\n");
        endProgramNoUsage(request);
    }
    if(connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in)) == -1){
        perror("connect\n");
        endProgramNoUsage(request);
    }
    return sockfd;
}

/*return 0 if str contains only digits(is a number), return -1 o.works only for positive numbers!*/
int is_a_number(char * str){
    int i = 0;
    while(i < strlen(str)){
        if(isdigit(str[i]) == 0)
            return NOT_A_NUMBER;
        i++;
    }
    return IS_A_NUMBER;
}

/*return index of value if found in arr.return -1 o.w*/
int find(char ** arr, char * value,int length,int startIndex){
    for(int i = startIndex; i < length; i++) {
        if (strcmp(arr[i], value) == 0)
            return i;
    }
    return NOT_FOUND;
}

/*return the first index of a char in a string*/
int charFirstOccur(char *s, char c){
    int i = 0;
    while(s[i] != '\0'){
        if(s[i] == c)
            return i;
        i++;
    }
    return NOT_FOUND;
}
/*print usage, free allocate memory and exit*/
void endProgram(Request * r){
    printUsageError();
    freeRequest(r);
    exit(1);
}

/*construct GET request and store in in Request struct*/
void GET_request(Request * r){
    char * base = "GET HTTP/1.0\r\nHOST:\r\n\r\n";
    int base_len = strlen(base);
    int extra = 20; // for missed calculate
    int path_len = 0, host_len = 0, parameters_len = 0;
    host_len = strlen(r->host);
    if(r->path == NULL)
        path_len = 1;
    else
        path_len = strlen(r->path);
    if(r->parameters == NULL)
        parameters_len = 0;
    else
        parameters_len = strlen(r->parameters);

    int size = base_len+host_len+path_len+parameters_len+extra;
    r->req_string = (char *)malloc(sizeof(char)*size);
    if(r->req_string == NULL){
        perror("malloc\n");
        endProgramNoUsage(r);
    }

    if(r->path == NULL) {
        if(r->parameters != NULL) // no path, have parameters
           sprintf(r->req_string, "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n", r->parameters,r->host);
        else // no path, no parameters
            sprintf(r->req_string, "GET / HTTP/1.0\r\nHost: %s\r\n\r\n",r->host);
    }
    else {
        if(r->parameters != NULL) // have path, have parameters
            sprintf(r->req_string, "GET %s%s HTTP/1.0\r\nHost: %s\r\n\r\n", r->path,r->parameters, r->host);
        else{ // have path, no parameters
            sprintf(r->req_string, "GET %s HTTP/1.0\r\nHost: %s\r\n\r\n", r->path, r->host);
        }
    }
}

/*construct POST request and store in in Request struct*/
void POST_request(Request * r){
    char * base = "POST HTTP/1.0\r\nHOST:\r\nContent-Length:\r\n\r\n";
    int base_len = strlen(base);
    int extra = 20; // for missed calculate
    int path_len = 0, host_len = 0,body_len = 0, parameters_len = 0;
    host_len = strlen(r->host);
    body_len = strlen(r->body);
    if(r->path == NULL)
        path_len = 1;
    else
        path_len = strlen(r->path);
    if(r->parameters == NULL)
        parameters_len = 0;
    else
        parameters_len = strlen(r->parameters);

    int size = base_len+host_len+path_len+parameters_len+extra;
    r->req_string = (char *)malloc(sizeof(char)*size);
    if(r->req_string == NULL){
        perror("malloc\n");
        endProgramNoUsage(r);
    }

    if(r->path == NULL) {
        if(r->parameters != NULL) // no path, have parameters
            sprintf(r->req_string, "POST /%s HTTP/1.0\r\nHost: %s\r\nContent-length:%d\r\n\r\n%s", r->parameters,r->host,body_len,r->body);
        else // no path, no parameters
            sprintf(r->req_string, "POST / HTTP/1.0\r\nHost: %s\r\nContent-length:%d\r\n\r\n%s",r->host,body_len,r->body);
    }
    else {
        if(r->parameters != NULL) // have path, have parameters
            sprintf(r->req_string, "POST %s%s HTTP/1.0\r\nHost: %s\r\nContent-length:%d\r\n\r\n%s", r->path,r->parameters,r->host,body_len,r->body);
        else // have path, no parameters
            sprintf(r->req_string, "POST %s HTTP/1.0\r\nHost: %s\r\nContent-length:%d\r\n\r\n%s", r->path, r->host,body_len,r->body);
    }
}

/*this function should be called the end of the main or at any problem causes to exit*/
void freeRequest(Request * request) {
    if(request == NULL)
        return;
    if (request->host != NULL)
        free(request->host);
    if (request->port != NULL)
        free(request->port);
    if (request->path != NULL)
        free(request->path);
    if (request->body != NULL)
        free(request->body);
    if (request->parameters != NULL)
        free(request->parameters);
    if(request->req_string != NULL)
        free(request->req_string);
    free(request);
}

/*end program no usage msg*/
void endProgramNoUsage(Request * r){
    freeRequest(r);
    exit(1);
}

/*prints usage msg*/
void printUsageError(){
    printf("Usage: client [-p <text>] [-r n < pr1=value1 pr2=value2 …>] <URL>\n");
}

int main(int argc, char *argv[]){
    if(argc == 1){ /*if no arguments received*/
        printUsageError();
        exit(1);
    }

    int sockfd,sizeInBytes = 0;;
    char buff[BUFF_SIZE];
    Request * req = (Request*)malloc(sizeof(Request)); /*will hold all needed information in order to send HTTP request*/
    if(req == NULL){
        perror("malloc\n");
        printUsageError();
        exit(1);
    }

    /*----ANALYZING DATA FROM ARGUMENTS----*/
    init_request(req); /*init all Request struct fildes to NULL*/
    analyzeArgv(argc,argv,req); /*store all needed information from arguments in req*/

    /*----Choosing GET/POST request by information from user arguments----*/
    if(req->body == NULL) /*user wants to make GET request*/
        GET_request(req);
    else /*user wants to make POST request*/
        POST_request(req);


    printf("HTTP request =\n%s\nLEN = %d\n", req->req_string,(int) strlen(req->req_string));
    /*-----try connect to server-----*/
    sockfd = connectToServer(req);
    /*-----sending request to server-----*/
    if(write(sockfd, req->req_string, strlen(req->req_string)) < 0){
        perror("write\n");
        shutdown(sockfd, SHUT_RDWR);
        close(sockfd);
        freeRequest(req);
    }
    bzero(buff, BUFF_SIZE);
    int count = 0;
    /*----reading server respond----*/
    while((count = read(sockfd, buff, BUFF_SIZE - 1)) > 0){
        printf("%s", buff);
        sizeInBytes += count;
        bzero(buff, BUFF_SIZE); //clear buffer
    }
    if(sizeInBytes < 0)
        perror("read\n");
    else
        printf("\n Total received response bytes: %d\n", sizeInBytes);
    /*closing all resources and free allocated memory*/
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd);
    freeRequest(req);

    return 0;
}

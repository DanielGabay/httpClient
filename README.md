Authored by: Daniel Gabay. 
"ex2" http client

==Program Files==
client.c - main program
README.txt - instructions

==Description==
 This program takes user input from terminal and build an HTTP/1.0 request.
 There are 2 types of Request that can be executed by this program: GET && POST requests.
 Conditions to make a successfull request:
 1)User must provide a valid URL (including "http://" prefix) in order to be able to connect to server.
 2)User can provide -p flag and afterwords the text (ex. -p helloWorld) to make POST Request.
   the text must contain no spaces and special chars and placed right after -p flag.
 3)User can provide -r flag and afterwords a number n that points out how many parameters to include and than the parameters.
   each parameter must be in a key=value form (example: age=25)
 Note: The flags and url can be provided at any order, but must comply all conditions above.
 Any input that not imply the conditions, will cause the program to exit with usage msg.
 Any failure within sys.calls will cause the program to exit and prints the sys.call problem.
 Url form example: http://Host[:port]/Filepath (when port and filepath is optional)

==How to compile?==
compile: gcc -Wall -o client client.c
Run: ./client <input>

==Input:==
Input must be provided form terminal as mentioned above in order to make a successful request.
Any input problem will cause the program to exit.

==Output:==
For valid inputs, the program will print the request string, its size and in case of no failure
it will print also the response from the server.

//
// Created by shachar on 07/01/19.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <ctype.h>
#include "threadpool.h"
#include <time.h>
#include <dirent.h>

#define USAGE "Usage: server <port> <pool-size> <max-number-of-request>\n"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define TIME_SIZE 128
#define LISTEN_NUM 5
#define READ_SIZE 4001
#define FILE_BUFFER 512

#define HTTP_1_0 "HTTP/1.0 "
#define SERVER "Server: webserver/1.0\nDate: "
#define CONTENT_TYPE "Content-Type: "
#define CONTENT_LEN "Content-Length: "
#define CONNECTION_ "Connection: close"
#define LAST_MODIFIED "Last-Modified: "
#define SPACE "\r\n"

#define RETURN_FILE 2001
#define DIRECTORY_CONTENT 2002
#define RETURN_HTML_FILE 2003
#define ITS_NOT_OK -1
#define ITS_OK 1
/** methods */
int isNum(char* s);
void giveTime(char timebuf[]);
char *get_mime_type(char *name);
char* response_400();
char* response_501();
char* response_500();
char* response_404();
char* response_403();
char* response_302(char* path);
char* getFileHeader(char *file);
int pathAccsess(char* path);
char* dir_content(char* path);//dir content directory
int handle_Fun ( void * p );
int buildServer(int port);
int countDigit(int num);
int checkRequest(char *method, char *path, char *protocol);
void sendFileData(char* file,int newfd);

int main (int argc, char* argv[])
{
    int port = 0;
    int poolSize = 0;
    int maxRequest = 0;

    if (argc != 4)
    {
        printf(USAGE);
        exit(1);
    }
    if (isNum(argv[1]) != ITS_OK || isNum(argv[2]) != ITS_OK || isNum(argv[3]) != ITS_OK)
    {
        printf(USAGE);
        exit(1);
    }
    port = atoi(argv[1]);
    poolSize = atoi(argv[2]);
    maxRequest = atoi(argv[3]);

    threadpool* pool = create_threadpool(poolSize);
    if (pool == NULL)
    {
        printf(USAGE);
        exit(1);
    }
    int welcome_fd = buildServer(port);
    if (welcome_fd == ITS_NOT_OK)
    {
        destroy_threadpool(pool);
        exit(1);
    }
    int* fd_array = (int*)malloc(sizeof(int)* maxRequest);   // will save the fd_id of the accept
    memset(fd_array,0,sizeof(int)* maxRequest); // clear and put 0 in all the array

    for (int i = 0 ; i < maxRequest ; i++)
    {
        fd_array[i] = accept(welcome_fd,NULL,NULL);    // got a new job here
        if ( fd_array[i] < 0)
        {
            perror("accept");
            break;
        }
        dispatch(pool,handle_Fun, (void*)&fd_array[i]);
    }
    close(welcome_fd);
    destroy_threadpool(pool);
    free(fd_array);
    return 0;
}
/** check if the char* str is a number   */
int isNum(char* s)
{
    int i = 0;
    while(i < strlen(s))
    {
        if (isdigit(s[i]) == 0)    // check every digit if its num
            return 0;
        i++;
    }
    return ITS_OK;
}

/**build the server and open the main socket.*/
int buildServer(int port)
{
    int welcome_fd;
    struct sockaddr_in srv;

    if ((welcome_fd = socket(PF_INET, SOCK_STREAM, 0)) == 0){   //
        perror("socket\n");
        exit(1);
    }
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = htonl(INADDR_ANY);
    srv.sin_port = htons(port);

    if (bind(welcome_fd, (struct sockaddr *)&srv,sizeof(srv))<0){
        perror("bind\n");
        return ITS_NOT_OK;
    }
    if (listen(welcome_fd, LISTEN_NUM) < 0){
        perror("listen\n");
        return ITS_NOT_OK;
    }
    return welcome_fd;
}
/**return the 400 response .*/
char* response_400()
{
    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);

    giveTime(timebuf);

    size += strlen(HTTP_1_0) + strlen("400 Bad Request") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);
    size += strlen(CONTENT_TYPE) + strlen("text/html")+ strlen(SPACE);
    size += strlen(CONTENT_LEN) + strlen("113") + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE);
    size += 113 + strlen(SPACE)+strlen(SPACE)+strlen(SPACE)+1;


    char * response = malloc(sizeof(char) * size);
    if (response == NULL)
        return  NULL;
    bzero(response,sizeof(char) * size);

    sprintf(response,HTTP_1_0 "400 Bad Request" SPACE SERVER "%s" SPACE CONTENT_TYPE "text/html" SPACE CONTENT_LEN "113" SPACE CONNECTION_ SPACE SPACE,timebuf);
    strcat(response,"<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>"SPACE"<BODY><H4>400 Bad request</H4>"SPACE"Bad Request."SPACE"</BODY></HTML>");

    return response;
}
/**return the 501 response .*/
char* response_501()
{
    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);

    giveTime(timebuf);

    size += strlen(HTTP_1_0) + strlen("501 Not supported") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);
    size += strlen(CONTENT_TYPE) + strlen("text/html")+ strlen(SPACE);
    size += strlen(CONTENT_LEN) + strlen("129") + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE);
    size += 129 + strlen(SPACE)+strlen(SPACE)+strlen(SPACE)+1;


    char* response = malloc(sizeof(char) * size);
    if (response == NULL)
        return  NULL;
    bzero(response,sizeof(char) * size);

    sprintf(response,HTTP_1_0 "501 Not supported" SPACE SERVER "%s" SPACE CONTENT_TYPE "text/html" SPACE CONTENT_LEN "129" SPACE CONNECTION_ SPACE SPACE,timebuf);
    strcat(response,"<HTML><HEAD><TITLE>501 Not supported</TITLE></HEAD>"SPACE"<BODY><H4>501 Not supported</H4>"SPACE"Method is not supported." SPACE "</BODY></HTML>");

    return response;
}
/**return the 404 response .*/
char* response_404()
{
    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);

    giveTime(timebuf);

    size += strlen(HTTP_1_0) + strlen("404 Not Found") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);
    size += strlen(CONTENT_TYPE) + strlen("text/html")+ strlen(SPACE);
    size += strlen(CONTENT_LEN) + strlen("112") + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE);
    size += 112 + strlen(SPACE) +strlen(SPACE) +strlen(SPACE)+1;

    char* response = malloc(sizeof(char) * size);
    if (response == NULL)
        return  NULL;
    bzero(response,sizeof(char) * size);

    sprintf(response,HTTP_1_0 "404 Not Found" SPACE SERVER "%s" SPACE CONTENT_TYPE "text/html" SPACE CONTENT_LEN "112" SPACE CONNECTION_ SPACE SPACE,timebuf);
    strcat(response,"<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>"SPACE"<BODY><H4>404 Not Found</H4>"SPACE "File not found."SPACE "</BODY></HTML>");

    return response;

}
/**return the 403 response .*/
char* response_403()
{
    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);

    giveTime(timebuf);

    size += strlen(HTTP_1_0) + strlen("403 Forbidden") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);
    size += strlen(CONTENT_TYPE) + strlen("text/html")+ strlen(SPACE);
    size += strlen(CONTENT_LEN) + strlen("111") + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE);
    size += 111 + strlen(SPACE) +strlen(SPACE) +strlen(SPACE)+1;


    char* response = malloc(sizeof(char) * size);
    if (response == NULL)
        return  NULL;
    bzero(response,sizeof(char) * size);

    sprintf(response,HTTP_1_0 "403 Forbidden" SPACE SERVER "%s" SPACE CONTENT_TYPE "text/html" SPACE CONTENT_LEN "111" SPACE CONNECTION_ SPACE SPACE,timebuf);
    strcat(response,"<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>"SPACE "<BODY><H4>403 Forbidden</H4>"SPACE "Access denied."SPACE "</BODY></HTML>");

    return response;
}

/**return the 302 response .*/
char* response_302(char* path)
{
    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);

    giveTime(timebuf);

    size += strlen(HTTP_1_0) + strlen("302 Found") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);
    size += strlen("Location: ") + strlen(path) + strlen("/") +strlen("/") + strlen(SPACE);
    size += strlen(CONTENT_TYPE) + strlen("text/html")+ strlen(SPACE);
    size += strlen(CONTENT_LEN) + strlen("123") + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE)+1;
    size += 123;

    char* response = malloc(sizeof(char) * size);
    if (response == NULL)
        return  NULL;
    bzero(response,sizeof(char) * size);

    sprintf(response,HTTP_1_0 "302 Found" SPACE SERVER "%s" SPACE "Location: /%s/" SPACE  CONTENT_TYPE "text/html" SPACE CONTENT_LEN "123" SPACE CONNECTION_ SPACE SPACE,timebuf ,path);
    strcat(response,"<HTML><HEAD><TITLE>302 Found</TITLE></HEAD>"SPACE "<BODY><H4>302 Found</H4>"SPACE "Directories must end with a slash." SPACE "</BODY></HTML>");

    return response;
}
/**return the 500 response .*/
char* response_500()
{
    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);

    giveTime(timebuf);

    size += strlen(HTTP_1_0) + strlen("500 Internal Server Error") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);
    size += strlen(CONTENT_TYPE) + strlen("text/html")+ strlen(SPACE);
    size += strlen(CONTENT_LEN) + strlen("144") + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE);
    size += 144 + strlen(SPACE) +strlen(SPACE)+strlen(SPACE)+strlen(SPACE)+1;

    char* response = malloc(sizeof(char) * size);
    if (response == NULL)
        return  NULL;
    bzero(response,sizeof(char) * size);

    sprintf(response,HTTP_1_0 "500 Internal Server Error" SPACE SERVER "%s" SPACE CONTENT_TYPE "text/html" SPACE CONTENT_LEN "144" SPACE CONNECTION_ SPACE SPACE,timebuf);
    strcat(response,"<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>"SPACE"<BODY><H4>500 Internal Server Error</H4>"SPACE"Some server side error." SPACE "</BODY></HTML>");

    return response;

}/** construct the date */
void giveTime(char timebuf[])
{
    time_t now;
    now = time(NULL);
    strftime(timebuf, TIME_SIZE, RFC1123FMT, gmtime(&now));

//timebuf holds the correct format of the current time.
}

/**go to every folder in the path and check that i have the right Accsess to executing*/

int pathAccsess(char* path)
{
    struct stat fp;
    char* tempPath = (char *) malloc(sizeof(char) * (strlen(path) + 1));

    if(tempPath == NULL)
        return 500;

    char* addThis = NULL;
    char* folderInPath;
    strcpy(tempPath, path);  // copy of the path we dont want to change it
    folderInPath = strtok(tempPath, "/");
    if (folderInPath == NULL)
    {
        free(tempPath);
        return 404;
    }

    while (1) // untill we get to break ->we dont have another folder to check
    {
        if (stat(folderInPath, &fp) < 0)
        {
            free(tempPath);
            return ITS_NOT_OK;
        }
        if (S_ISDIR(fp.st_mode) && !(fp.st_mode & S_IXOTH))
        {
            free(folderInPath);
            return ITS_NOT_OK;
        }
        addThis = strtok(NULL, "/");
        if(!addThis)
            break;
        sprintf(folderInPath + strlen(folderInPath), "/%s", addThis);
    }
    free(folderInPath);
    return ITS_OK;
}
char *get_mime_type(char *name) {
    char *ext = strrchr(name, '.');
    if (!ext) return NULL;
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".au") == 0) return "audio/basic";
    if (strcmp(ext, ".wav") == 0) return "audio/wav";
    if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
    if (strcmp(ext, ".mp3") == 0) return "audio/mpeg";
    return NULL;
}

/** this function will return the right num of response that i need */
int checkRequest(char *method, char *path, char *protocol) {

    if (method == NULL || path == NULL || protocol == NULL || strlen(path) < 1)   // freeeeeeee
        return 400;
    if (strcmp(protocol, "HTTP/1.1") != 0 && strcmp(protocol, "HTTP/1.0") != 0)//not the version
        return 400;

    if (strcmp(method, "GET") != 0)
        return 501;

    struct stat sb;

    if ( stat(path, &sb) < 0)// couldn't find this path in on the current files in the directory.
    {
        return 404;
    }
    else{
        /// we got this path now we check if its folder:
        if (S_ISDIR(sb.st_mode))   // its folder!!
        {
            if (path[strlen(path) - 1] != '/')   // doesnt end with /
            {
                return 302;
            }
            else{                                                        // end with /
                int pathAcc = pathAccsess(path);
                if (pathAcc == ITS_NOT_OK) // dont have premisstion to enter the file/folder
                    return  403;
                else if (pathAcc == 500)
                    return 500;
                else if (pathAcc == 404)
                    return 404;

                char* checkFile =(char *) malloc((strlen(path) + strlen("index.html") + 1) * sizeof(char));
                if (checkFile == NULL)
                    return 500;
                bzero(checkFile,(strlen(path) + strlen("index.html") + 1) * sizeof(char) );
                strcpy(checkFile,path);
                strcat(checkFile,"index.html");

                struct stat htmlFile;

                if(stat(checkFile, &htmlFile) >= 0 && htmlFile.st_mode & S_IROTH)    // there is a file "index.html" in the folder!
                {
                    free(checkFile);
                    return RETURN_HTML_FILE;
                } else {
                 // there is not a file " index.html"
                    free(checkFile);
                    return  DIRECTORY_CONTENT;
                }
            }
        }
        else{             // should be a file!!!!!
            int pathAcc = pathAccsess((path));
            if (pathAcc == 500)
                return  500;
            else if (pathAcc == 404)
                return 404;

            if((S_ISREG(sb.st_mode) != ITS_OK) || !(sb.st_mode & S_IROTH) || pathAcc == ITS_NOT_OK) {
                return 403;
            }else
                return RETURN_FILE;  // return file content
        }
    }
}

/** return the file header */
char* getFileHeader(char *file)
{
    struct stat sb;

    if( stat(file, &sb) < 0)
        return NULL;   /// CHECKKKKKKKKKKKKKKKKKKKKKKKKKKK

    char lastModifiedTime[TIME_SIZE];
    bzero(lastModifiedTime,TIME_SIZE);
    strftime(lastModifiedTime, TIME_SIZE, RFC1123FMT, gmtime(&sb.st_mtime));

    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);

    giveTime(timebuf);
    char* type = get_mime_type(file);
    int type_size = 0;
    if (type)
        type_size = (int)strlen(type)+1;

    size += strlen(HTTP_1_0) + strlen("200 OK") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);
    size += strlen(CONTENT_TYPE) + type_size + strlen(SPACE);
    size += strlen(CONTENT_LEN) + countDigit((int)sb.st_size) + strlen(SPACE);
    size+= strlen(LAST_MODIFIED) + strlen(lastModifiedTime) + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE);
    size += (int) sb.st_size + 10 ;
    char * response = (char*) malloc(sizeof(char) * size );
    if (response == NULL)
        return NULL;
    bzero(response,sizeof(char) * size);
    if(type)
        sprintf(response,HTTP_1_0 "200 OK" SPACE SERVER "%s" SPACE CONTENT_TYPE "%s" SPACE CONTENT_LEN "%d" SPACE LAST_MODIFIED "%s" SPACE CONNECTION_ SPACE SPACE,timebuf,type ,(int)sb.st_size,lastModifiedTime);
    else
        sprintf(response,HTTP_1_0 "200 OK" SPACE SERVER "%s" SPACE CONTENT_LEN "%d" SPACE LAST_MODIFIED "%s" SPACE CONNECTION_ SPACE SPACE,timebuf ,(int)sb.st_size,lastModifiedTime);

    return response;
}

int countDigit(int num)
{
    int count = 0;
    while (num > 0)
    {
        count++;
        num = num /10 ;
    }
    return count;
}
/** the main function of the server program. will send this function to the threads. this function will deal with the request of the client */
int handle_Fun ( void * p )
{
    if( p == NULL)
        return 0;
    int readIsOk = 1; // will tell me if the read from buffer is ok.
    int mess = 0 ;
    int fd = (*(int*)p);    // the fd of the accept

    char readBuffer[READ_SIZE];
    bzero(readBuffer,READ_SIZE);

    int nbytes;
    if((nbytes = (int) read(fd, readBuffer, READ_SIZE - 1)) < 0)   //nothing to read
    {
        perror("read");
        readIsOk = 0;
        mess = 500;
    }
    char* method = strtok(readBuffer, " ");
    char* path = strtok(NULL, " ");
    char* protocol = strtok(NULL, "\r\n");  /// untill the /r

    if(path && strcmp(path,"/") == 0 )
        path = "./"; // change the path to "./" to get the current folder

    if ( path && path[0] == '/')
        path = path +1;
    if (readIsOk)
    mess = checkRequest(method, path, protocol);

    char* answer = NULL;  // will save the response headers
    char* htmlFile = NULL;  // will save the index.html file if i need to

    switch(mess)//the status of the request from thr client
    {
        case 500:{
            answer = response_500();
            break;
        }
        case 501:{
            answer = response_501();
            break;
        }
        case 400:{
            answer = response_400();
            break;
        }
        case 403:{
            answer = response_403();
            break;
        }
        case 404:{
            answer = response_404();
            break;
        }
        case 302:{
            answer = response_302(path);
            break;
        }
        case RETURN_HTML_FILE:{
         //   path = path+1; // dont need the /
            htmlFile = (char *) malloc((strlen(path) + strlen("index.html") + 1) * sizeof(char));
            if (htmlFile == NULL)
            {
                answer = NULL;
                break;
            }
            bzero(htmlFile,(strlen(path) + strlen("index.html") + 1) * sizeof(char));
            strcpy(htmlFile,path);
            strcat(htmlFile,"index.html");
            answer = getFileHeader(htmlFile);
            break;
        }
        case DIRECTORY_CONTENT:{
            answer = dir_content(path);
            break;

        case RETURN_FILE:  {
                answer = getFileHeader(path);
                break;
            }
        }
        default: {
            answer = NULL;
            break;
        }
    }

    if (answer == NULL)   // if there is problem so answer will be NULL
        answer = response_500();

    nbytes = 0;
    if((nbytes = (int) write(fd, answer, strlen(answer))) < 0)//write the error/status
    {
        perror("write");
        exit(1);
    }

    if (mess == RETURN_FILE )
        sendFileData(path,fd);
    else if (mess == RETURN_HTML_FILE)
    {
        sendFileData(htmlFile,fd);
        if(htmlFile)
            free(htmlFile);
    }
    close(fd);
    free(answer);

    return 1 ;
}
/** if the client want a folder that doesnt have */
char* dir_content(char* path)
{
    struct stat sb;

    if( stat(path, &sb) < 0)
        return NULL;

    char lastModifiedTime[TIME_SIZE];
    bzero(lastModifiedTime,TIME_SIZE);

    strftime(lastModifiedTime, TIME_SIZE, RFC1123FMT, gmtime(&sb.st_mtime));

    int size = 0;
    char timebuf[TIME_SIZE];
    bzero(timebuf,TIME_SIZE);
    giveTime(timebuf);

    size += strlen(HTTP_1_0) + strlen("200 OK") + strlen(SPACE);
    size += strlen(SERVER) + strlen(timebuf) + strlen(SPACE);

    size += strlen(CONTENT_TYPE) + strlen("text/html") + strlen(SPACE);
    size += strlen(CONTENT_LEN) + 22 + strlen(SPACE);

    size+= strlen(LAST_MODIFIED) + strlen(lastModifiedTime) + strlen(SPACE);
    size += strlen(CONNECTION_) + strlen(SPACE) +strlen(SPACE);

///////////////////////////untill here i counted the header ////////////////////////////////
    int folderContentSize = 0;

    folderContentSize += strlen("<HTML>") +strlen(SPACE);
    folderContentSize += strlen("<HEAD><TITLE>Index of </TITLE></HEAD>") + strlen(path) + strlen(SPACE) ;
    folderContentSize += strlen(SPACE) + strlen("<BODY>") +strlen(SPACE)+ strlen("<H4>Index of <path-of-directory></H4>") +strlen(SPACE);
    folderContentSize += strlen(SPACE) + strlen("<table CELLSPACING=8>")+strlen(SPACE);
    folderContentSize += strlen("<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>");
    folderContentSize += strlen(SPACE) + strlen(SPACE) ;
    /// after the folder items
    folderContentSize += strlen(SPACE) + strlen("</table>") + strlen(SPACE) +strlen(SPACE) + strlen("<HR>")+strlen(SPACE) +strlen(SPACE);
    folderContentSize += strlen("<ADDRESS>webserver/1.0</ADDRESS>") +strlen(SPACE) +strlen(SPACE) +strlen("</BODY></HTML>");
    ///for every itemmmm in the folder
    struct dirent **namelist;
    int countFiles = scandir(path, &namelist, NULL, alphasort);   // will save how many files/ folders i have the wanted folder

    for (int i = 0; i < countFiles ; i++)
    {
        folderContentSize += TIME_SIZE;  //  for each one i will get his last modified
        folderContentSize += 22;          // max length of file
        folderContentSize += strlen("<tr>") + strlen(SPACE) + strlen("<td><A HREF=\"\"></A></td><td></td>");
        folderContentSize += 2 * ((int) strlen(namelist[i]->d_name)); //the name of files will apprear twice
        folderContentSize += strlen(SPACE) + strlen("<td></td>");
        folderContentSize += strlen(SPACE) + strlen("/tr>")+strlen(SPACE);
    }
    size += folderContentSize;

    char * response = malloc(sizeof(char) * (size +1));
    if (response == NULL)
        return NULL;
    bzero(response,sizeof(char) * (size +1));

    char * responseHtml = malloc(sizeof(char)  *(folderContentSize +1));
    if (responseHtml == NULL)
        return NULL;
    bzero(responseHtml,sizeof(char) * (folderContentSize +1));

    sprintf(responseHtml + strlen(responseHtml),"<HTML>"SPACE "<HEAD><TITLE>Index of %s</TITLE></HEAD>" SPACE SPACE "<BODY>" SPACE "<H4>Index of %s</H4>",path,path);
    sprintf(responseHtml + strlen(responseHtml),SPACE SPACE "<table CELLSPACING=8>" SPACE "<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>" SPACE);

    for (int j = 0; j < countFiles ; j++)
    {
        char* pathToFile = (char*)malloc(strlen(path) + strlen(namelist[j]->d_name) + 1);
        if (pathToFile == NULL)
        {
            free(response);
            return NULL;
        }
        bzero(pathToFile,strlen(path) + strlen(namelist[j]->d_name) + 1);
        strcpy(pathToFile, path);
        strcat(pathToFile, namelist[j]->d_name);

        stat(pathToFile,&sb);
        strftime(lastModifiedTime, TIME_SIZE, RFC1123FMT, gmtime(&sb.st_mtime));   // last modified now will be of the file
        sprintf(responseHtml + strlen(responseHtml),"<tr>" SPACE "<td><A HREF=\"%s%s\">%s</A></td><td>%s</td>"SPACE,namelist[j]->d_name,S_ISDIR(sb.st_mode) ? "/" : "",namelist[j]->d_name,lastModifiedTime);

        if(S_ISDIR(sb.st_mode))                                                          //if its a folder.
            sprintf(responseHtml + strlen(responseHtml),"<td></td>" SPACE "</tr>" SPACE);
        else
            sprintf(responseHtml + strlen(responseHtml),"<td>%lu</td>" SPACE "</tr>" SPACE,sb.st_size);

        free (pathToFile);
        free(namelist[j]);
    }
    sprintf(responseHtml + strlen(responseHtml),SPACE "</table>" SPACE SPACE "<HR>" SPACE SPACE "<ADDRESS>webserver/1.0</ADDRESS>" SPACE SPACE "</BODY></HTML>");

    if( stat(path, &sb) < 0)
        return NULL;

    strftime(lastModifiedTime, TIME_SIZE, RFC1123FMT, gmtime(&sb.st_mtime));

    sprintf(response,HTTP_1_0 "200 OK" SPACE SERVER "%s" SPACE CONTENT_TYPE "text/html" SPACE CONTENT_LEN "%d" SPACE LAST_MODIFIED "%s" SPACE CONNECTION_ SPACE SPACE,timebuf,(int)strlen(responseHtml),lastModifiedTime);
    strcat(response,responseHtml);
    free(responseHtml);
    free(namelist);

    return response;
}

/** after we send the file header, we need to send the content of the file. */

void sendFileData(char* file,int newfd)
{
    int theFile = open(file,O_RDONLY);
    if(theFile < 0)
        return;

    unsigned char buffer[FILE_BUFFER];
    int checkRead = 0;
    int checkWrite = 0;
    while(1)      //there is something to read
    {
        checkRead = (int)read(theFile,buffer,sizeof(buffer)-1);
        if(checkRead == 0)     //nothing to read
            break;
        if(checkRead < 0)
        {
            perror("read");
            break;
        }
        checkWrite = (int)write(newfd, buffer, checkRead);      //write the quantity we read
        if(checkWrite < 0)
        {
            perror("write");
            close(theFile);
            return;
        }
    }
    close(theFile);
}

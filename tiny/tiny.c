/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr,
                    &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    doit(connfd);   // line:netp:tiny:doit
    Close(connfd);  // line:netp:tiny:close
  }
}

// doit function processes one HTTP Transaction.
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  /*
   * Rio Package: 짧은 카운트에서 발생할 수 있는 네트워크 프로그램 같은 응용에서 편리하고,
     안정적이고 효율적인 I/O 제공
  */
  rio_t rio;

  // Read request line and headers
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  // only GET method supported
  if(strcasecmp(method, "GET")){
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  read_requesthdrs(&rio);

  // 찾는 파일이 디스크 상에 있지 않으면, 에러메시지를 클라이언트에 보내고 리턴
  is_static = parse_uri(uri, filename, cgiargs);
  if(stat(filename, &sbuf)<0){
    clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    return;
  }

  // Serve static content
  // check if file is static
  if(is_static){
    // check file have read permission 
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }
  // Serve dynamic content
  else{
    // check file is excuteable
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
      clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);
  }
}

// send errorMSG to client
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // Build the HTTP response body
  sprintf(body,"<html><title>Tiny Error</title>");
  sprintf(body,"%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body,"%s%s : %s\r\n", body, errnum, shortmsg);
  sprintf(body,"%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body,"%s<hr><em>The Tiny Web server</em>\r\n", body);

  // Print the HTTP response
  sprintf(buf, "HTTP/1.0 %s %s \r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type : text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd,buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}


// Read and ignore request headers
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  // carriage return과 line feed 쌍으로 구성되어 있다는 점을 주목해라
  while(strcmp(buf, "\r\n")){ // EOF (한줄 전체가 개행 문자인 곳) 만날때까지 계속 읽기
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}


// Analysis HTTP URI
int parse_uri(char *uri, char *filename, char * cgiargs)
{
  char *ptr;

  // 실행파일의 홈디렉토리는 /cgi-bin이라고 가정한다
  if(!strstr(uri, "cgi-bin")){ // if for static content
    strcpy(cgiargs, ""); // delete cgi args 
    strcpy(filename, "."); // uri를 ./index.html같은 상대 리눅스 경로이름으로 변환한다.
    strcat(filename, uri); // strcat(*str1, *str2) : str2를 str1에 연결하고 NULL 문자로 결과 스트링 종료
    if(uri[strlen(uri) -1 ] == '/'){ // 만일 uri가 /문자로 끝난다면
      // 기본파일이름을 추가한다.
      strcat(filename, "home.html");
    }
    return 1;
  }
  else{ // if for Dynamic content
    // delete all cgi args
    ptr = index(uri, "?");
    if(ptr){
      strcpy(cgiargs, ptr+1);
      *ptr = "\0";
    }
    else{
      strcpy(cgiargs, "");
    }
    // 나머지 uri 부분을 상대 리눅스 파일 이름으로 변환한다.
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // Send response headers to client
  get_filetype(filename, filetype);
  // 파일 이름의 접미어 부분을 검사해서 파일 타입을 결정하고
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  // 클라이언트에 응답 줄과 응답 헤더를 보낸다.
  // 빈줄 한개가 헤더를 종료하고 있다는 점에 주목해야 한다.
  sprintf(buf, "%sServer : Tiny Web Server \r\n",buf);
  sprintf(buf, "%sConnection : close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);

  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // Send response body to clinet
  // 읽기 위해서 filename을 오픈하고 식별자를 얻어온다
  srcfd = Open(filename, O_RDONLY, 0);
  // 리눅스 mmap함수는 요청한 파일을 가상메모리 영역으로 매핑한다.
  // mmap함수를 호출하면 파일 srcfd의 첫 번째 filesize 바이트를 주소 srcp에서
  // 시작하는 사적 읽기-허용 가상메모리 영역으로 매핑한다.
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // 파일을 매핑한 후에 더이상 이 식별자는 팔요 없으며 그래서 이파일을 닫는다.
  Close(srcfd);
  // 실제로 파일을 클라이언트에게 전송한다.
  // Rio_writen 함수는 주소 srcp에서 시작하는 filesize바이트 를 클라이언트의 연결 식별자로 복사한다.
  Rio_writen(fd, srcp, filesize);
  // 매핑된 가상메모리 주소를 반환한다. <- 이것은 메모리 누수를 피하는 데 중요하다.
  Munmap(srcp, filesize);
}

// get_filetype - Derive file type from filename
// service static contents to client
void get_filetype(char *filename, char * filetype)
{
  if(strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if(strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if(strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if(strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  else 
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = {NULL};

  // Return first part of HTTP response
  sprintf(buf, "HTTP/1,0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // 자식 프로세스를 fork하고 그 후에 CGI 프로그램을 자식의 컨텍스트에서 실행하며 모든 종류의 동적 컨텐츠를 제공한다. 
  if(Fork() == 0) //child
  {
    // Real server would set all CGI vars here
    setenv("QUERY_STRING", cgiargs, 1);
    Dup2(fd, STDOUT_FILENO); // Redirect stdout to client
    Execve(filename, emptylist, environ); // Run CGI program
  }
  Wait(NULL); // Parent waits for and reaps child == 부모는 아이를 기다리고 거둔다
}
/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

// 두개의 정수를 더하는 CGI(Common Gateway Interface) Program
// 웹 서버 상에서 사용자 프로그램을 동작시키기 위한 조합이다.
int main(void) {
  char *buf, *p;
  // 👇 11.10
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE], arg3[MAXLINE], arg4[MAXLINE];
  int n1 =0, n2 =0;
  // Extract the two arguments
  // 👇 11.10
  if ((buf = getenv("QUERY_STRING")) != NULL) {
    p = strchr(buf, '&');
    *p = '\0'; // 👈 이거 도대체 뭔데?
    strcpy(arg1, buf);
    strcpy(arg2, p+1);
    // arg1문자열에서 =을 찾아라
    p = strchr(arg1, '=');
    *p = '\0';
    // arg1 문자열에서 =뒤에 있는 모든 문자 
    // ex) arg1 : num1=13 -> 13
    strcpy(arg3, p+1);

    p = strchr(arg2, '=');
    *p = '\0';
    strcpy(arg4, p+1);

    n1 = atoi(arg3);
    n2 = atoi(arg4);
  }
  // char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  // int n1 =0, n2 =0;
  // printf("#########################################\n");
  // printf("I'm IN adder.c\n");
  // printf("#########################################\n");
  // // Extract the two arguments
  // if((buf == getenv("QUERY_STRING")) != NULL){
  //   p = strchr(buf, '&');
  //   *p = '\0';
  //   strcpy(arg1, buf);
  //   strcpy(arg2, p+1);
  //   n1 = atoi(arg1);
  //   n2 = atoi(arg2);
  // }

  // Make the response body
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcome to add.com : ");
  sprintf(content, "%sTHE Internet addition portal. \r\n<p>", content);
  sprintf(content, "%sThe answer is : %d + %d = %d\r\n<p>", content, n1, n2, n1+n2);
  sprintf(content, "%sThanks for visiting!\r\n", content);

  // Generate the HTTP response
  printf("Connection : close \r\n");
  printf("Content-length : %d\r\n", (int)strlen(content));
  printf("Content-type : text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);

  exit(0);
}
/* $end adder */

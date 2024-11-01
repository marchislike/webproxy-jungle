/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 *
 * 이 코드는 매우 기본적인 HTTP/1.0 웹 서버로, 클라이언트가 요청한 정적(static) 또는 동적(dynamic) 콘텐츠를 제공함.
 * 서버는 GET 요청만을 지원하며, 요청을 수락하고 처리한 후 클라이언트에게 응답을 보낸다.
 */

#include "csapp.h" // 자체 제공 유틸리티 라이브러리. 파일 입출력, 네트워크, 메모리 등 다양한 유틸리티 함수가 포함되어 있음.

// 함수 프로토타입 선언: 나중에 정의될 함수들이며, 각 함수는 HTTP 요청을 처리하는 데 사용된다.
void doit(int fd);                                // 클라이언트 요청을 처리하는 함수
void read_requesthdrs(rio_t *rp);                // 클라이언트 요청 헤더를 읽는 함수
int parse_uri(char *uri, char *filename, char *cgiargs);  // URI를 파싱하여 파일 이름과 CGI 인자를 추출하는 함수
void serve_static(int fd, char *filename, int filesize);  // 정적 콘텐츠를 클라이언트에 전송하는 함수
void get_filetype(char *filename, char *filetype);  // 파일의 확장자에 따라 MIME 타입을 결정하는 함수
void serve_dynamic(int fd, char *filename, char *cgiargs);  // CGI 프로그램을 실행하여 동적 콘텐츠를 생성하는 함수
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);  // 클라이언트에게 에러 메시지를 보내는 함수

// main 함수: 서버의 진입점으로, 포트를 열고 클라이언트의 연결을 수락하여 요청을 처리하는 역할을 한다.
int main(int argc, char *argv[]) { // * = char **argv
  signal(SIGPIPE, SIG_IGN); //! SIGPIPE  

  int listenfd, connfd;  // listenfd는 서버가 요청을 듣기 위해 사용하는 대기용 소켓 디스크립터, connfd는 개별 클라이언트와의 연결에 사용되는 디스크립터
  char hostname[MAXLINE], port[MAXLINE];  // 클라이언트의 호스트 이름과 포트 번호를 저장할 버퍼
  socklen_t clientlen;  // 클라이언트의 소켓 주소 구조체 크기
  struct sockaddr_storage clientaddr;  // 클라이언트의 주소 정보를 저장하는 구조체

  /* Check command line args */
  if (argc != 2) {  // 서버는 명령줄 인자로 포트 번호를 받으며, 이를 체크한다. 프로그램명은 있는데 포트 번호가 없으면 오류 메시지를 출력하고 종료.
    fprintf(stderr, "usage: %s <port>\n", argv[0]);  // 표준 오류 출력 스트림으로 사용법을 출력
    exit(1);  // 비정상 종료
  }

/*소켓의 파일 디스크립터(호스트이름, 포트번호)를 저장했다가 반환 911p*/ 
  listenfd = Open_listenfd(argv[1]);  // 주어진 포트 번호로 소켓을 열고 서버가 클라이언트의 연결 요청을 들을 준비를 한다.
  while (1) {  // 무한 루프: 서버는 종료 없이 계속해서 연결을 받아야 한다.
    clientlen = sizeof(clientaddr);  // 클라이언트 주소 구조체의 크기를 저장 (Accept 함수에서 사용됨)
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // 클라이언트의 연결 요청을 수락. connfd는 새로운 연결의 파일 디스크립터
    // Getnameinfo를 사용하여 클라이언트의 주소정보로 호스트 이름과 포트 번호를 알아낸다. 
    //?<=> GetAddressinfo는 호스트명과 포트번호로 클라이언트의 주소정보를 얻는다.
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);  //SA 898p, getnameinfo 905p 클라이언트가 누구인지 찾는
    printf("Accepted connection from (%s, %s)\n", hostname, port);  // 호스트 이름과 포트 번호를 출력 (디버깅용)
    doit(connfd);   // 클라이언트의 요청을 처리하는 함수 호출 (HTTP 요청을 처리하고 응답을 보냄)
    Close(connfd);  // 클라이언트와의 연결을 종료. 자원을 해제하기 위해 필요
  }
}

/*
 * doit - 클라이언트로부터 받은 요청을 처리하는 핵심 함수
 * fd는 클라이언트와의 연결을 나타내는 파일 디스크립터. 이를 통해 데이터를 읽고 응답을 보냄.
 */
void doit(int fd) {
  int is_static;  // 요청된 콘텐츠가 정적 콘텐츠인지 동적 콘텐츠인지 구분
  struct stat sbuf;  // 요청된 파일의 메타데이터(크기, 접근 권한 등)를 저장하는 구조체
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];  // 클라이언트 요청을 처리하기 위한 버퍼
  char filename[MAXLINE], cgiargs[MAXLINE];  // 요청된 파일 이름과 CGI 인자
  rio_t rio;  // 읽기 버퍼를 위한 구조체. 이를 통해 클라이언트로부터 데이터를 효율적으로 읽음.

  /* Read Request line and headers */
  Rio_readinitb(&rio, fd);  // 클라이언트 소켓 디스크립터를 이용해 rio 버퍼 초기화. 효율적인 데이터 읽기를 위한 준비.
  Rio_readlineb(&rio, buf, MAXLINE);  // 클라이언트로부터 첫 번째 요청 라인을 읽음. (HTTP 메서드, URI, 버전 정보 포함)
  printf("Request headers:\n");  // 요청 헤더를 출력 (디버깅용)
  printf("%s", buf);  // 첫 번째 요청 라인 출력
  sscanf(buf, "%s %s %s", method, uri, version);  // 요청 라인에서 메서드, URI, 버전을 파싱

  // HTTP 메서드가 "GET"이 아닌 경우, Tiny는 이를 지원하지 않으므로 클라이언트에 에러 메시지 반환 //TODO : HEAD 메소드 -------------
  if (strcasecmp(method, "GET")) {  // strcasecmp는 대소문자 구분 없이 문자열을 비교
    clienterror(fd, method, "501", "Not implemented",
                "Tiny does not implement this method");  // 501 에러: 구현되지 않음
    return;
  }

  // 요청 헤더 읽기 (메서드가 GET인 경우에만 계속 진행)
  read_requesthdrs(&rio);  // 추가로 클라이언트 요청 헤더를 읽어들임. 헤더 내용은 실제로는 처리하지 않지만 출력.

  /* Parse URI from GET request */
  // URI를 파싱하여 정적 콘텐츠인지 동적 콘텐츠인지 판별. 파싱된 URI에서 파일 이름과 CGI 인자를 추출.
  is_static = parse_uri(uri, filename, cgiargs);  
  // stat 함수는 파일의 메타데이터를 가져오는데 사용. 파일이 존재하지 않으면 -1 반환.
  if (stat(filename, &sbuf) < 0) {  // 요청한 파일이 존재하지 않는 경우 404 에러 반환
    clienterror(fd, filename, "404", "Not found",
                "Tiny couldn't find this file");
    return;
  }

  // 파일이 정적 콘텐츠인 경우
  if (is_static) { 
    // 파일이 정규 파일이 아니거나, 읽기 권한이 없으면 403 에러 반환
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {  
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read this file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);  // 정적 파일을 클라이언트에 전송
  } 
  // 동적 콘텐츠인 경우 (CGI 프로그램 실행)
  else {  
    // 파일이 정규 파일이 아니거나 실행 권한이 없으면 403 에러 반환
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {  
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't run the CGI program");
      return;
    }
    serve_dynamic(fd, filename, cgiargs);  // CGI 프로그램을 실행하여 동적 콘텐츠 생성
  }
}

void clienterror(int fd, char *cause, char *errnum,
                  char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];  // 에러 응답을 저장할 버퍼들

  /* HTTP 응답 본문 생성 */
  sprintf(body, "<html><title>Tiny Error</title>");  // 에러 페이지의 HTML 제목 생성
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);  // 흰색 배경의 HTML 본문 시작
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);  // 에러 번호와 간단한 에러 메시지 추가
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);  // 자세한 에러 메시지와 에러 원인 추가
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);  // 페이지 하단에 서버 정보 추가

  /* HTTP 응답 헤더 출력 */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);  // HTTP 상태 줄 생성 (예: "HTTP/1.0 404 Not Found")
  Rio_writen(fd, buf, strlen(buf));  // 클라이언트에게 상태 줄 전송
  sprintf(buf, "Content-type: text/html\r\n");  // 콘텐츠 타입을 HTML로 지정
  Rio_writen(fd, buf, strlen(buf));  // 클라이언트에게 콘텐츠 타입 전송
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));  // 콘텐츠 길이를 설정
  Rio_writen(fd, buf, strlen(buf));  // 클라이언트에게 콘텐츠 길이 전송

  /* HTTP 응답 본문 출력 */
  Rio_writen(fd, body, strlen(body));  // 클라이언트에게 에러 메시지가 포함된 HTML 본문 전송
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];  // 요청 헤더를 저장할 버퍼

  while(Rio_readlineb(rp, buf, MAXLINE) > 0){  // !빈 줄("\r\n")이 나올 때까지 계속 헤더를 읽음
    if (strcmp(buf, "\r\n") == 0){
      break;
    }
    printf("%s", buf);  // 각 헤더를 출력 (디버깅 용도)
  }
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;  // URI에서 '?'를 찾기 위한 포인터

  if (!strstr(uri, "cgi-bin")){ /* 정적 콘텐츠 */
    strcpy(cgiargs, "");  // CGI 인자는 없으므로 빈 문자열로 설정
    strcpy(filename, ".");  // 현재 디렉토리에서 파일을 찾기 위해 파일 이름 앞에 '.'을 붙임
    strcat(filename, uri);  // URI를 파일 이름에 덧붙임
    if (uri[strlen(uri)-1] == '/')  // URI가 '/'로 끝나면 기본 파일로 home.html을 지정
      strcat(filename, "home.html");
    return 1;  // 정적 콘텐츠임을 반환
  }
  else{ /* 동적 콘텐츠 */
    ptr = index(uri, '?');  // URI에서 '?'를 찾음 (CGI 인자를 구분)
    if (ptr) {
      strcpy(cgiargs, ptr+1);  // '?' 뒤의 CGI 인자를 cgiargs에 복사
      *ptr = '\0';  // '?'를 '\0'로 바꿔 URI와 인자를 분리
    }
    else
      strcpy(cgiargs, "");  // 인자가 없으면 빈 문자열로 설정
    strcpy(filename, ".");  // 현재 디렉토리에서 파일을 찾기 위해 '.'을 붙임
    strcat(filename, uri);  // URI를 파일 이름에 덧붙임
    return 0;  // 동적 콘텐츠임을 반환
  }
}

void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;  // 파일 디스크립터
  char *srcp, filetype[MAXLINE], buf[MAXBUF];  // 파일을 메모리에 맵핑할 포인터, 파일 타입, 응답 버퍼

  /* 클라이언트에게 응답 헤더 전송 */
  get_filetype(filename, filetype);  // 파일의 MIME 타입을 결정
  sprintf(buf, "HTTP/1.0 200 OK\r\n");  //! 상태 줄 생성 (200 OK)
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);  // 서버 정보 추가
  sprintf(buf, "%sConnection: close\r\n", buf);  // !연결을 닫을 것임을 알림
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);  // 콘텐츠 길이 추가
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);  // MIME 타입 추가
  Rio_writen(fd, buf, strlen(buf));  // 클라이언트에게 응답 헤더 전송
  printf("Response headers:\n");
  printf("%s", buf);  // 응답 헤더를 출력 (디버깅 용도)

  /* 클라이언트에게 응답 본문 전송 */
  srcfd = Open(filename, O_RDONLY, 0);  // 파일을 읽기 모드로 염
  srcp = (char*)malloc(filesize); //*
  Rio_readn(srcfd, srcp, filesize); //* malloc

  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);  // !파일을 메모리에 매핑

  Close(srcfd);  // 파일 디스크립터를 닫음
  Rio_writen(fd, srcp, filesize);  // 매핑된 파일 내용을 클라이언트에 전송
  // Munmap(srcp, filesize);  // !매핑된 메모리를 해제
  free(srcp);//* malloc
}

void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))  // 파일 이름에 ".html"이 포함되면 HTML 파일로 간주
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))  // ".gif" 파일일 경우 GIF 이미지로 간주
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))  // ".png" 파일일 경우 PNG 이미지로 간주
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))  // ".jpg" 파일일 경우 JPEG 이미지로 간주
    strcpy(filetype, "image/jpeg");
  else if (strstr(filename, ".mp4"))  //! ".mp4" 파일일 경우 mp4 이미지로 간주
    strcpy(filetype, "video/mp4");
  else  // 그 외의 파일은 일반 텍스트로 간주
    strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };  // 응답을 저장할 버퍼와 빈 인자 리스트

  /* HTTP 응답 헤더 전송 */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");  // 상태 줄 생성 (200 OK)
  Rio_writen(fd, buf, strlen(buf));  // 클라이언트에게 상태 줄 전송
  sprintf(buf, "Server: Tiny Web Server\r\n");  // 서버 정보 추가
  Rio_writen(fd, buf, strlen(buf));  // 클라이언트에게 서버 정보 전송

  if (Fork() == 0) { /* 자식 프로세스 */
    /* 실제 서버에서는 여기에 CGI 환경 변수를 설정 */
    setenv("QUERY_STRING", cgiargs, 1);  // CGI 인자를 환경 변수로 설정
    Dup2(fd, STDOUT_FILENO);  // 표준 출력을 클라이언트 소켓으로 리다이렉트
    Execve(filename, emptylist, environ);  // CGI 프로그램 실행
  }
  Wait(NULL); /* 부모 프로세스는 자식 프로세스가 끝날 때까지 기다림 */
}
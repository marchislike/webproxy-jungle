/* 클라이언트 접속기 */

#include "csapp.h"

//프로그램 실행 시 OS로부터 전달받는 인자들을 담고 있는 main
// argc 프로그램에 전달된 인자의 수 (프로그램명 자체도 하나의 인자이므로 최소값은 1)
// argv 문자열 배열에 대한 포인터. 모든 명령줄 인자를 포함하고 있음(각 인자는 명령줄 한 줄인 문자열이고 char* 이다.)

int main(int argc, char **argv) // 명령줄 인자"command line arguments"를 처리하기 위해 사용되는 표준 매개변수
{
    int clientfd;
    char *host, *port, buf[MAXLINE]; //buf는 입력받는 데이터, 서버로부터 보낼 응답 데이터 등을 담은 배열(일시적 저장소)
    rio_t rio;

    if (argc != 3){ // 프로그램명, 호스트명, 포트번호
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        // usage: ./echo_client localhost 9999
        exit(0);
    }

    // 프로그램명 = argv[0] << ./echo ~~~ 로 실행했을 경우 "./echo"를 말함
    host = argv[1];
    port = argv[2];

    /* Open_clientfd 호스트명과 포트번호로 서버에 대한 연결을 설정
    1. 소켓 생성 - ipv4 or ipv6
    2. 주소 생성 - 호스트명을 ip주소로 변환
    3. 연결 - 변환된 ip주소와 포트를 사용해서 소켓을 통해 서버에 연결
    4. 오류 처리 */
    clientfd = Open_clientfd(host, port);
    Rio_readinitb(&rio, clientfd); // 버퍼 초기화(버퍼 시작위치, 현재 읽기 위치 설정)
    //* 왜 여기 위치에서 버퍼를 초기화?
    /* 연결이 성공적으로 이루어지지 않으면, 데이터를 읽거나 쓸 소켓이 없기 때문에, 
    버퍼 초기화를 진행할 필요가 없기에 연결 성공 후 버퍼를 초기화한다.
    이렇게 하면 연결 실패 시 불필요한 리소스 사용을 방지할 수 있다. */


    /* Fgets : 사용자로부터 표준 입력을 받아 버퍼에 저장 */
    /* MAXLINE 상수
    버퍼의 크기를 명확히 지정(지금은 충분히 큰 값)함으로써 오버플로우를 방지
    주로 읽기 작업에서 쓰이고, 
    쓰기 작업에서는 strlen(buf)를 사용하여 실제 데이터 길이만큼만 전송하기에 필요없다.
    */

    while (Fgets(buf, MAXLINE, stdin) != NULL ){ // 사용자의 입력이 존재하는 동안
    //? strlen(buf) 함수는 buf에 저장된 문자열의 길이를 반환하며, 이 값은 널 종료 문자(\0)를 제외한 실제 문자의 개수
        Rio_writen(clientfd, buf, strlen(buf)); // buf에 저장된 데이터를 연결된 서버에 전송
        Rio_readlineb(&rio, buf, MAXLINE);//서버로부터 받은 응답을 다시 buf에 저장
        Fputs(buf, stdout);//buf에서 응답데이터를 꺼내어 표준 출력방식으로 보여줌 
        // Fputs : 사용자가 입력한 걸 어떻게 서버가 처리했는지 아는 방법
    }
    Close(clientfd);
    exit(0);
}

/* Rio_writen
예를 들어, buf가 "hello\n"을 포함하고 있다면, strlen(buf)는 6을 반환하며, 
이는 "hello" 다섯 글자와 개행 문자 하나를 포함한 전체 길이를 의미합니다. 
따라서 Rio_writen은 정확히 이 6바이트를 서버로 전송하게 됩니다.
*/
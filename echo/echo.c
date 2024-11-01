#include "csapp.h"

//연결된 클라이언트의 파일 디스크립터인 connfd를 인자로 사용하여 
// 클라이언트로부터 데이터를 읽고 다시 그대로 돌려보내는 함수 echo
void echo(int connfd)
{
    size_t n; //바이트 수 n - 데이터 크기
    char buf[MAXLINE]; // 버퍼 - 데이터를 임시로 담아둘 공간
    rio_t rio; // connfd를 가리키는 구조체타입 포인터 rio

    //stdout - 표준출력 스트림에 대한 버퍼링 설정을 조정
    //NULL - 버퍼에 별도의 공간 할당 없이 스트림이 내부적으로 관리케함
    //_IONBF - printf함수를 통한 출력이 발생할 때마다 즉시 콘솔에 결과가 나타남
    // 0 -_IONBF 모드에서는 무시되는 파라미터
    // setvbuf(stdout, NULL, _IONBF, 0);

/* Rio_readinitb : rio 구조체를 초기화하고 connfd와 연결해서 데이터를 받을 준비를 함
rio 구조체는 connfd를 사용해서 데이터 읽기 위해 필요한 정보를 설정한다*/
// * 초기화 과정에서는 rio 구조체의 내부 버퍼와 포인터들이 적절히 설정되어, 
// * 데이터 읽기가 시작될 때 바로 사용될 수 있도록 한다.
    Rio_readinitb(&rio, connfd);

//* Rio_readlineb는 rio 구조체를 통해 소켓에서 데이터를 읽고, 
//한 줄의 데이터를 buf에 저장한 뒤, 그 줄의 길이를 반환합니다.
//반환된 줄의 길이(n)가 0이 아닐 때까지 계속해서 데이터를 읽고 처리하는 반복문이 실행됩니다.
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){ //
        printf("server received %d bytes\n", (int)n);
        Rio_writen(connfd, buf, n);
/* Rio_writen 함수는 buf에 저장된 데이터를 
클라이언트와 연결된 소켓(connfd)을 통해 전송하는 역할*/
    }
}

/* rio_t 구조체
rio_t 구조체는 Robust I/O (Rio) 라이브러리에 정의된 데이터 구조로, 
파일 또는 네트워크 소켓의 입력/출력 작업을 더 효율적으로 관리하기 위한 정보를 포함
*/

/* 버퍼링 유형
_IOFBF: 완전 버퍼링, 스트림에 데이터를 쓸 때 버퍼가 가득 차면 자동으로 출력
_IOLBF: 라인 버퍼링, 개행 문자를 만날 때마다 출력
_IONBF: 비버퍼링, 데이터가 입력되자마자 즉시 출력 ****
*/
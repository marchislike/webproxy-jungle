/* 서버단 */

#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv)
{
    /*listenfd는 서버가 클라이언트의 연결 요청을 !듣기 위해! 사용하는 소켓의 파일 디스크립터 
    connfd는 !연결된 클라이언트와의 통신!을 위한 소켓의 파일 디스크립터*/
    int listenfd, connfd; 
    socklen_t clientlen; //소켓 주소 구조체의 크기를 저장하는 변수. 소켓 주소의 길이를 나타내는 데 사용
    struct sockaddr_storage clientaddr; /* 소켓주소 저장소 크기의 구조체인 clientaddr *클라이언트측 주소 */
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if (argc != 2){ //프로그램명 외에 포트번호 인자 1개만 들어와야 함. "포트번호 포함해서 인자가 2개가 아닌 경우"
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        // usage : ./echo_serveri 9999
        exit(0);
    }

    // * Open_listenfd : 지정된 포트에서 들어오는 연결 요청을 기다리기 위해 사용
    // * 해당 포트에 대한 listen 소켓의 파일 디스크립터를 생성하고 반환
    listenfd = Open_listenfd(argv[1]);
    while(1){ 
        clientlen = sizeof(struct sockaddr_storage);// 소켓주소가 담긴 구조체의 사이즈가 클라이언트 요청의 사이즈
        /* Accept 함수는 연결 요청을 수락하고, 
        연결된 클라이언트와 통신할 수 있는 새로운 소켓 디스크립터 connfd를 반환*/
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        /* Getnameinfo 함수는 주어진 소켓 주소로부터 클라이언트의 호스트명과 포트번호를 추출 */
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0); //클라이언트측에서 온 요청에 대한 헤더 정보
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd);
        Close(connfd);
    }
    exit(0);
}
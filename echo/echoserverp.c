/* echo 서버로 동시성 구현
멀티프로세스(=자식 프로세스)
여러 클라이언트에서 접속해도 echo 함수가 동작하도록 하기 

부모가 자식을 Fork하고 각각의 새로운 연결 요청을 처리 
###############################################
부모 프로세스에서는 자식 프로세스가 클라이언트의 요청을 처리하는 동안, 
다시 listenfd를 통해 다른 클라이언트의 연결 요청을 계속 기다립니다.


*/

#include "csapp.h"
void echo(int connfd);

/* 
sigchld_handler 함수는 SIGCHLD 신호를 처리하기 위한 핸들러(함수)
부모 프로세스는 자식 프로세스가 종료될 때마다 
시그널 핸들러(sigchld_handler)를 통해 자식 프로세스의 종료를 처리 
>> 이는 자식 프로세스의 자원을 정리하고 시스템에서 자원을 회수하기 위함 */
void sigchld_handler(int sig) // int sig : SIGCHLD를 나타내는 신호의 번호
{   // WNOHANG - wait with no hang
    /* waitpid 특정 조건을 만족하는 자식 프로세스의 상태를 확인 후
        해당 자식 프로세스가 종료 시, 그 프로세스의 pid를 반환 */
    
    //*waitpid가 0보다 큰 값을 반환하는 동안(= 종료된 자식 PID를 반환하는 동안) 
    while (waitpid(-1, 0, WNOHANG) > 0 ); 
    /* 
    -1 : 부모가 가진 모든 자식 프로세스를 대상으로 하다
    0 : 자식 프로세스의 종료상태에 대한 정보를 얻지 않겠다
    WNOHANG : 자식 중 아무도 종료되지 않았으면 waitpid는 0을 반환하고 즉시 리턴
    -> 함수가 부모 프로세스 실행을 block하지 않고 즉시 계속 진행되도록 함
    
    */
    return; // void 이므로 단순히 종료
    //종료된 자식 프로세스가 더 이상 없을 때 waitpid는 0을 반환하고, while 문은 종료
}

int main(int argc, char **argv) // **argv = *argv[]
{
    // int listenfd, connfd, port;
    int listenfd, connfd;
    socklen_t clientlen = sizeof(struct sockaddr_in); //클라이언트측 정보 사이즈는 listen 서버측 소켓주소 구조체의 크기만큼이다.
    struct sockaddr_in clientaddr; //소켓의 주소에 대한 정보를 담은 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE];

    if(argc != 2){ //포트번호가 없으면
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    // port = atoi(argv[1]);

    Signal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(argv[1]); //서버 소켓 준비
    // listenfd = Open_listenfd(port);

    while (1){
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen); //클라이언트 연결 수락
        Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        if (Fork() == 0){ //? 0이면 = 자식이면 --> 자식 프로세스에서 실행될 코드!
        /* Fork는 현재 실행 중인 프로세스의 복사본을 만듦
        부모 프로세스에게는 자식 프로세스 id를 반환
        복사된 자식 프로세스에게는 0을 반환
        Fork()의 결과가 0인 경우는 자식 프로세스인 상태를 뜻하게 됨
        */
            Close(listenfd); /* 자식프로세스가 listening 소켓을 닫음 */
            echo(connfd); /* 클라이언트 요청 처리 */
            printf("Disconnected from (%s, %s)\n", client_hostname, client_port);
            Close(connfd); /* 클라이언틑와의 연결 종료*/
            exit(0); /* 자식 프로세스 종료 */
        }
        Close(connfd); /* 부모도 연결된 소켓을 닫음. - 리소스 관리*/
    }
}
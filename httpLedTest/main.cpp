/************
 ���������, ���������� �̿��Ͽ� LED�����ϱ� 
*************/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <wiringPi.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define	PORT		8080
#define LED			1  //GPIO 18

extern "C" int captureImage(int fd);

int kbhit()
{
	// 터미널에 대한 구조체
	struct termios oldt, newt;
	int ch, oldf;
	
	// 현재 터미널에 설정된 정보를 받아온다.
	tcgetattr(0, &oldt);	
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);	//정규 모드입력과 에코를 해제한다.
	tcsetattr(0, TCSANOW, &newt);		// 현재 터미널에 변경된 설정값을 적용한다.
	oldf = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, oldf | O_NONBLOCK); //Non-blocking모드로 설정한다.
	
	ch = getchar();
	
	tcsetattr(0, TCSANOW, &oldt);		//기존의 값으로 터미널 속성을 바로 변경한다.
	
	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}

int testKbhit(void)
{
	int	i=0;
	while(1)
	{
		if(kbhit())
		{
			switch(getchar())
			{
				case 'q':
					goto END;
					break;
			};
			i++;
			printf("%20d\t\t\r", i);
			usleep(100);
		}
	}
END:
		printf("Good Bye!\n");	
}

void *gpiofunction(void *arg)
{
	int ret =0;
	return (void*)ret;
}

int ledControl(int gpio, int onoff)
{
    pinMode(gpio, OUTPUT) ;             /* 핀(Pin)의 모드 설정 */
    digitalWrite(gpio, (onoff)?HIGH:LOW);   /* LED 켜고 끄기 */

    return 0;
}


int sendData(int fd, FILE* fp, char *ct, char *file_name)
{
    /* 클라이언트로 보낼 성공에 대한 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n";
    char end[ ] = "\r\n"; /* 응답 헤더의 끝은 항상 \r\n */
    char buf[BUFSIZ];
    int len;

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);
    fflush(fp);

    /* 파일 이름이 capture.bmp인 경우 이미지를 캡처한다. */ 
    if(!strcmp(file_name, "capture.bmp"))
        captureImage(fd);

    fd = open(file_name, O_RDWR); /* 파일을 연다. */
    do {
        len = read(fd, buf, BUFSIZ); /* 파일을 읽어서 클라이언트로 보낸다. */
        fwrite(buf, len, sizeof(char), fp);
    } while(len == BUFSIZ);

    fflush(fp);

    close(fd); /* 파일을 닫는다. */

    return 0;
}

void sendOk(FILE* fp)
{
    /* 클라이언트에 보낼 성공에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n\r\n";

    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}

void sendError(FILE* fp)
{
    /* 클라이언트로 보낼 실패에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 400 Bad Request\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n";
    char cnt_len[ ] = "Content-Length:1024\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n\r\n";

    /* 화면에 표시될 HTML의 내용 */
    char content1[ ] = "<html><head><title>BAD Connection</tiitle></head>";
    char content2[ ] = "<body><font size=+5>Bad Request</font></body></html>";

    printf("send_error\n");
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs(content1, fp);
    fputs(content2, fp);
    fflush(fp);
}

void *clnt_connection(void *arg)
{
     /* 스레드를 통해서 넘어온 arg를 int 형의 파일 디스크립터로 변환한다. */
    int clnt_sock = *((int*)arg), clnt_fd;
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[10], ct[BUFSIZ], type[BUFSIZ];
    char file_name[256], file_buf[256];
    char* type_buf;
    int i = 0, j = 0, len = 0;

    /* 파일 디스크립터를 FILE 스트림으로 변환한다. */
    clnt_read = fdopen(clnt_sock, "r");
    clnt_write = fdopen(dup(clnt_sock), "w");
    clnt_fd = clnt_sock;

    /* 한 줄의 문자열을 읽어서 reg_line 변수에 저장한다. */
    fgets(reg_line, BUFSIZ, clnt_read);

    /* reg_line 변수에 문자열을 화면에 출력한다. */
    fputs(reg_line, stdout);

    /* ‘ ’ 문자로 reg_line을 구분해서 요청 라인의 내용(메소드)를 분리한다. */
    strcpy(method, strtok(reg_line, " "));
    if(strcmp(method, "POST") == 0) { /* POST 메소드일 경우를 처리한다. */
        sendOk(clnt_write); /* 단순히 OK 메시지를 클라이언트로 보낸다. */
        fclose(clnt_read);
        fclose(clnt_write);

        return (void*)NULL;
    } else if(strcmp(method, "GET") != 0) { /* GET 메소드가 아닐 경우를 처리한다. */
        sendError(clnt_write); /* 에러 메시지를 클라이언트로 보낸다. */
        fclose(clnt_read);
        fclose(clnt_write);

        return (void*)NULL;
    }

    strcpy(file_name, strtok(NULL, " ")); /* 요청 라인에서 경로(path)를 가져온다. */
    if(file_name[0] == '/') { /* 경로가 ‘/’로 시작될 경우 /를 제거한다. */
        for(i = 0, j = 0; i < BUFSIZ; i++) {
            if(file_name[0] == '/') j++;
            file_name[i] = file_name[j++];
            if(file_name[i+1] == '\0') break;
        };
    }

    /* 라즈베리 파이를 제어하기 위한 HTML 코드를 분석해서 처리한다. */
    if(strstr(file_name, "?") != NULL) {
        char optLine[32];
        char optStr[4][16];
        char opt[8], var[8];
        char* tok;
        int i, count = 0;

        strcpy(file_name, strtok(file_name, "?"));
        strcpy(optLine, strtok(NULL, "?"));

        /* 옵션을 분석한다. */
        tok = strtok(optLine, "&");
        while(tok != NULL) {
            strcpy(optStr[count++], tok);
            tok = strtok(NULL, "&");
        };

        /* 분석한 옵션을 처리한다. */
        for(i = 0; i < count; i++) {
            strcpy(opt, strtok(optStr[i], "="));
            strcpy(var, strtok(NULL, "="));
            printf("%s = %s\n", opt, var);
            if(!strcmp(opt, "led") && !strcmp(var, "On")) { /* LED를 켠다. */
                ledControl(LED, 1);
            } else if(!strcmp(opt, "led") && !strcmp(var, "Off")) { /* LED를 끈다. */
                ledControl(LED, 0);
            }
        };
    }

    /* 메시지 헤더를 읽어서 화면에 출력하고 나머지는 무시한다. */
    do {
        fgets(reg_line, BUFSIZ, clnt_read);
        fputs(reg_line, stdout);
        strcpy(reg_buf, reg_line);
        type_buf = strchr(reg_buf, ':');
    } while(strncmp(reg_line, "\r\n", 2)); /* 요청 헤더는 ‘\r\n’으로 끝난다. */

    /* 파일의 이름을 이용해서 클라이언트로 파일의 내용을 보낸다. */
    strcpy(file_buf, file_name);
    sendData(clnt_fd, clnt_write, ct, file_name);

    fclose(clnt_read); /* 파일의 스트림을 닫는다. */
    fclose(clnt_write);

    pthread_exit(0); /* 스레드를 종료시킨다. */

    return (void*)NULL;
}

int main(int argc, char *argv[])
{
	int i;
	int serv_sock;
	pthread_t thread;
	struct sockaddr_in serv_addr, clnt_addr;
	unsigned int clnt_addr_size;
	socklen_t optlen;
	int option;
	pthread_t ptGpio;

	//testKbhit();
/*
	if(argc!=2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }
*/
	wiringPiSetup();
	
	//pthread_create(&ptGpio, NULL, gpiofunction, NULL);
	
	// 1. Socket()
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
	{
		perror("Error : socket()");
		return -1;
	}
	optlen=sizeof(option);
    option=TRUE;    
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &option, optlen);	
	
	// 2. bind
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);
	//serv_addr.sin_port = htons(atoi(argv[1]));
	if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))==-1)
	{
		perror("Error:bind()");
		return -1;
	}
	
	// 3. listen
	// 최대 10대의 클라이언트의 동시접속을 처리가능하도록 큐를 생성한다.
	if(listen(serv_sock, 10)==-1)
	{
		perror("Error:listen()");
		return -1;  
	}
	
	while(1)
	{
			int clnt_sock;
			
			clnt_addr_size = sizeof(clnt_addr);
			clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
			printf("Client IP : %s\n", inet_ntoa(clnt_addr.sin_addr));
			
			pthread_create(&thread, NULL, clnt_connection, &clnt_sock);
			pthread_join(thread, NULL);
	};
END:
		printf("Good Bye!\n");
			
		return 0;
}
	
	

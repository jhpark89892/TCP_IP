/************
 라즈베리파이, 웹페이지를 이용하여 LED제어하기 
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
	// �꽣誘몃꼸�뿉 ����븳 援ъ“泥�
	struct termios oldt, newt;
	int ch, oldf;
	
	// �쁽�옱 �꽣誘몃꼸�뿉 �꽕�젙�맂 �젙蹂대�� 諛쏆븘�삩�떎.
	tcgetattr(0, &oldt);	
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);	//�젙洹� 紐⑤뱶�엯�젰怨� �뿉肄붾�� �빐�젣�븳�떎.
	tcsetattr(0, TCSANOW, &newt);		// �쁽�옱 �꽣誘몃꼸�뿉 蹂�寃쎈맂 �꽕�젙媛믪쓣 �쟻�슜�븳�떎.
	oldf = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, oldf | O_NONBLOCK); //Non-blocking紐⑤뱶濡� �꽕�젙�븳�떎.
	
	ch = getchar();
	
	tcsetattr(0, TCSANOW, &oldt);		//湲곗〈�쓽 媛믪쑝濡� �꽣誘몃꼸 �냽�꽦�쓣 諛붾줈 蹂�寃쏀븳�떎.
	
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
    pinMode(gpio, OUTPUT) ;             /* ���(Pin)�쓽 紐⑤뱶 �꽕�젙 */
    digitalWrite(gpio, (onoff)?HIGH:LOW);   /* LED 耳쒓퀬 �걚湲� */

    return 0;
}


int sendData(int fd, FILE* fp, char *ct, char *file_name)
{
    /* �겢�씪�씠�뼵�듃濡� 蹂대궪 �꽦怨듭뿉 ����븳 �쓳�떟 硫붿떆吏� */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n";
    char end[ ] = "\r\n"; /* �쓳�떟 �뿤�뜑�쓽 �걹��� �빆�긽 \r\n */
    char buf[BUFSIZ];
    int len;

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);
    fflush(fp);

    /* �뙆�씪 �씠由꾩씠 capture.bmp�씤 寃쎌슦 �씠誘몄��瑜� 罹≪쿂�븳�떎. */ 
    if(!strcmp(file_name, "capture.bmp"))
        captureImage(fd);

    fd = open(file_name, O_RDWR); /* �뙆�씪�쓣 �뿰�떎. */
    do {
        len = read(fd, buf, BUFSIZ); /* �뙆�씪�쓣 �씫�뼱�꽌 �겢�씪�씠�뼵�듃濡� 蹂대궦�떎. */
        fwrite(buf, len, sizeof(char), fp);
    } while(len == BUFSIZ);

    fflush(fp);

    close(fd); /* �뙆�씪�쓣 �떕�뒗�떎. */

    return 0;
}

void sendOk(FILE* fp)
{
    /* �겢�씪�씠�뼵�듃�뿉 蹂대궪 �꽦怨듭뿉 ����븳 HTTP �쓳�떟 硫붿떆吏� */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n\r\n";

    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}

void sendError(FILE* fp)
{
    /* �겢�씪�씠�뼵�듃濡� 蹂대궪 �떎�뙣�뿉 ����븳 HTTP �쓳�떟 硫붿떆吏� */
    char protocol[ ] = "HTTP/1.1 400 Bad Request\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n";
    char cnt_len[ ] = "Content-Length:1024\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n\r\n";

    /* �솕硫댁뿉 �몴�떆�맆 HTML�쓽 �궡�슜 */
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
     /* �뒪�젅�뱶瑜� �넻�빐�꽌 �꽆�뼱�삩 arg瑜� int �삎�쓽 �뙆�씪 �뵒�뒪�겕由쏀꽣濡� 蹂��솚�븳�떎. */
    int clnt_sock = *((int*)arg), clnt_fd;
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[10], ct[BUFSIZ], type[BUFSIZ];
    char file_name[256], file_buf[256];
    char* type_buf;
    int i = 0, j = 0, len = 0;

    /* �뙆�씪 �뵒�뒪�겕由쏀꽣瑜� FILE �뒪�듃由쇱쑝濡� 蹂��솚�븳�떎. */
    clnt_read = fdopen(clnt_sock, "r");
    clnt_write = fdopen(dup(clnt_sock), "w");
    clnt_fd = clnt_sock;

    /* �븳 以꾩쓽 臾몄옄�뿴�쓣 �씫�뼱�꽌 reg_line 蹂��닔�뿉 ����옣�븳�떎. */
    fgets(reg_line, BUFSIZ, clnt_read);

    /* reg_line 蹂��닔�뿉 臾몄옄�뿴�쓣 �솕硫댁뿉 異쒕젰�븳�떎. */
    fputs(reg_line, stdout);

    /* ��� ��� 臾몄옄濡� reg_line�쓣 援щ텇�빐�꽌 �슂泥� �씪�씤�쓽 �궡�슜(硫붿냼�뱶)瑜� 遺꾨━�븳�떎. */
    strcpy(method, strtok(reg_line, " "));
    if(strcmp(method, "POST") == 0) { /* POST 硫붿냼�뱶�씪 寃쎌슦瑜� 泥섎━�븳�떎. */
        sendOk(clnt_write); /* �떒�닚�엳 OK 硫붿떆吏�瑜� �겢�씪�씠�뼵�듃濡� 蹂대궦�떎. */
        fclose(clnt_read);
        fclose(clnt_write);

        return (void*)NULL;
    } else if(strcmp(method, "GET") != 0) { /* GET 硫붿냼�뱶媛� �븘�땺 寃쎌슦瑜� 泥섎━�븳�떎. */
        sendError(clnt_write); /* �뿉�윭 硫붿떆吏�瑜� �겢�씪�씠�뼵�듃濡� 蹂대궦�떎. */
        fclose(clnt_read);
        fclose(clnt_write);

        return (void*)NULL;
    }

    strcpy(file_name, strtok(NULL, " ")); /* �슂泥� �씪�씤�뿉�꽌 寃쎈줈(path)瑜� 媛��졇�삩�떎. */
    if(file_name[0] == '/') { /* 寃쎈줈媛� ���/��숇줈 �떆�옉�맆 寃쎌슦 /瑜� �젣嫄고븳�떎. */
        for(i = 0, j = 0; i < BUFSIZ; i++) {
            if(file_name[0] == '/') j++;
            file_name[i] = file_name[j++];
            if(file_name[i+1] == '\0') break;
        };
    }

    /* �씪利덈쿋由� �뙆�씠瑜� �젣�뼱�븯湲� �쐞�븳 HTML 肄붾뱶瑜� 遺꾩꽍�빐�꽌 泥섎━�븳�떎. */
    if(strstr(file_name, "?") != NULL) {
        char optLine[32];
        char optStr[4][16];
        char opt[8], var[8];
        char* tok;
        int i, count = 0;

        strcpy(file_name, strtok(file_name, "?"));
        strcpy(optLine, strtok(NULL, "?"));

        /* �샃�뀡�쓣 遺꾩꽍�븳�떎. */
        tok = strtok(optLine, "&");
        while(tok != NULL) {
            strcpy(optStr[count++], tok);
            tok = strtok(NULL, "&");
        };

        /* 遺꾩꽍�븳 �샃�뀡�쓣 泥섎━�븳�떎. */
        for(i = 0; i < count; i++) {
            strcpy(opt, strtok(optStr[i], "="));
            strcpy(var, strtok(NULL, "="));
            printf("%s = %s\n", opt, var);
            if(!strcmp(opt, "led") && !strcmp(var, "On")) { /* LED瑜� 耳좊떎. */
                ledControl(LED, 1);
            } else if(!strcmp(opt, "led") && !strcmp(var, "Off")) { /* LED瑜� �걟�떎. */
                ledControl(LED, 0);
            }
        };
    }

    /* 硫붿떆吏� �뿤�뜑瑜� �씫�뼱�꽌 �솕硫댁뿉 異쒕젰�븯怨� �굹癒몄���뒗 臾댁떆�븳�떎. */
    do {
        fgets(reg_line, BUFSIZ, clnt_read);
        fputs(reg_line, stdout);
        strcpy(reg_buf, reg_line);
        type_buf = strchr(reg_buf, ':');
    } while(strncmp(reg_line, "\r\n", 2)); /* �슂泥� �뿤�뜑�뒗 ���\r\n��숈쑝濡� �걹�궃�떎. */

    /* �뙆�씪�쓽 �씠由꾩쓣 �씠�슜�빐�꽌 �겢�씪�씠�뼵�듃濡� �뙆�씪�쓽 �궡�슜�쓣 蹂대궦�떎. */
    strcpy(file_buf, file_name);
    sendData(clnt_fd, clnt_write, ct, file_name);

    fclose(clnt_read); /* �뙆�씪�쓽 �뒪�듃由쇱쓣 �떕�뒗�떎. */
    fclose(clnt_write);

    pthread_exit(0); /* �뒪�젅�뱶瑜� 醫낅즺�떆�궓�떎. */

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
	// 理쒕�� 10����쓽 �겢�씪�씠�뼵�듃�쓽 �룞�떆�젒�냽�쓣 泥섎━媛��뒫�븯�룄濡� �걧瑜� �깮�꽦�븳�떎.
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
	
	

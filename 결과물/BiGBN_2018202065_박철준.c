#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 1    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */
/* 양방향 고백엔 프로토콜로 구현*/


/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
// layer 5에서 전달되는 데이터 단위
struct msg {
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
// layer 4에서 전달되는 단위
struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];    //msg의 data
};


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int send_base_A;// A send의 window base num
int send_base_B;// B send의 window base num

int next_seqnum_A; // A send측의 다음 보낼 packet위치
int next_seqnum_B; // B send측의 다음 보낼 packet위치

int expected_seqnum_A; // A receive측의 원하는 seqnum번호
int expected_seqnum_B; // B receive측의 원하는 seqnum번호

int A_ackstate = 0;   //ack를 보내는 상태인지 아닌지 구별하는 변수
int B_ackstate = 0;   //ack를 보내는 상태인지 아닌지 구별하는 변수 

int A_my_acknum = 0;  //누적 ack을 갖는 변수, ack역할을 하지않을시 999를 가짐
int B_my_acknum = 0;  //누적 ack을 갖는 변수, ack역할을 하지않을시 999를 가짐

#define WINDOWSIZE 40   //window size N = 40으로 함
#define TIMEOUT 10 //TIME OUT을 10으로 함

struct pkt pkt_A_window[1024];  //A output에서 보내는 패킷을 저장하는 장소이며 추후에 윈도우가 됨 수 있음으로 장소의 사이즈는 넉넉히 잡아주며 정확한 윈도우 사이즈는 WINDOWSIZE로 정의
struct pkt pkt_B_window[1024];  //B output에서 보내는 패킷을 저장하는 장소이며 추후에 윈도우가 됨 수 있음으로 장소의 사이즈는 넉넉히 잡아주며 정확한 윈도우 사이즈는 WINDOWSIZE로 정의

//A side 변수를 초기화
void A_init(void)
{
    memset(pkt_A_window, 0, 1024);
    send_base_A = 1;
    next_seqnum_A = 1;
    expected_seqnum_A = 1;
    A_ackstate = 0;
    A_my_acknum = 0;
}

//B side 변수를 초기화
void B_init(void)
{
    memset(pkt_B_window, 0, 1024);
    send_base_B = 1;
    next_seqnum_B = 1;
    expected_seqnum_B = 1;
    B_ackstate = 0;
    B_my_acknum = 0;
}

//checksum을 계산하는 함수
//seqnum, acknum, payload의 각 문자의 아스키코드의 합을 반환
//모두 다 더함으로써 하나라도 틀린다면 판별가능함

int getchecksum(struct pkt packet)
{
    // checksum 구하는 함수
    //엑스트라 크래딧 방식으로는 구현하지 못하였습니다
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < sizeof(packet.payload) / sizeof(char); i++)
        checksum += packet.payload[i];    //모두 더하여 checksum설정
    return checksum;
}

struct pkt pkt_make(int seqnum, int acknum, char* data)
{
    //packet을 만드는 함수
    // 
    //새로운 packet을 생성하고 초기화하는 과정
    struct pkt new_pkt;
    new_pkt.seqnum = seqnum;
    new_pkt.acknum = acknum;
    for (int i = 0; i < sizeof(new_pkt.payload) / sizeof(char); i++) {
        new_pkt.payload[i] = data[i];
    }
    new_pkt.checksum = getchecksum(new_pkt);
    return new_pkt;
}

/* called from layer 5, passed the data to be sent to other side */
A_output(struct msg message)   
{
    //layer5에서 데이터 message를 받고 패킷을 만들어 A에서 B로의 패킷과 액크을 함께 보내기위해 호출되는 함수

    if (next_seqnum_A < send_base_A + WINDOWSIZE)
    {
        if (A_ackstate == 0)
            A_my_acknum = 999;   //다른 사이드의 sender에게서 받은 ack가 존재하지 않는 경우 
        struct pkt received_packet_for_layer5;    //보낼 packet
        received_packet_for_layer5 = pkt_make(next_seqnum_A, A_my_acknum, message.data); //message를 추출하고 packet을 생성함
        pkt_A_window[next_seqnum_A-1] = received_packet_for_layer5;  //A 사이드의 window에 packet을 저장
        tolayer3(0, received_packet_for_layer5);   //packet을 B_input으로 전송 보냄
        if (A_my_acknum!=999)   //ack를 같이 보내는 경우 출력
        {
            printf("A_output : send_packet with ACK (ACK = %d, seq = %d) : ", received_packet_for_layer5.acknum, received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        else //nak여서 acknum에 999를 보내는 경우 출력
        {
            printf("A_output : send_packet without ACK (seq = %d) : ", received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        A_ackstate = 0;   // ack를 보냈다고 가정하고 ACKstate 를 0 으로 변경해주어 ACK를 보내지 않는 상태를 의미하도록 사용함

        if (send_base_A == next_seqnum_A)
        {
            //stoptimer(0, (float)TIMEOUT);//안전한 켜기를 위한 타이머 껏다 키기
            starttimer(0, (float)TIMEOUT); //timer를 시작함
        }
            
        next_seqnum_A++;    //next sequence number를 1증가시킴
        
    }
    else
    {
        printf("A_output : Buffer is full.Drop the message.\n");

        //data 전송을 거부한다.
    }
}

B_output(struct msg message)  
{
    //layer5에서 데이터 message를 받고 패킷을 만들어 B에서 A로의 패킷과 액크을 함께 보내기위해 호출되는 함수
    if (next_seqnum_B < send_base_B + WINDOWSIZE)
    {
        if (B_ackstate == 0)
            B_my_acknum = 999;   //다른 사이드의 sender에게서 받은 ack가 존재하지 않는 경우  
        struct pkt received_packet_for_layer5;    //보낼 packet
        received_packet_for_layer5 = pkt_make(next_seqnum_B, B_my_acknum, message.data);  //message를 추출하고 packet을 생성함
        pkt_B_window[next_seqnum_B-1] = received_packet_for_layer5;  //B 사이드의 window에 packet을 저장
        tolayer3(1, received_packet_for_layer5);   //packet을 A_input으로 전송 보냄
        if (B_my_acknum!=999)   //ack를 같이 보내는 경우 출력
        {
            printf("B_output : send_packet with ACK (ACK = %d, seq = %d) : ", received_packet_for_layer5.acknum, received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        else //nak여서 acknum에 999를 보내는 경우 출력
        {
            printf("B_output : send_packet without ACK (seq = %d) : ", received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        B_ackstate = 0;   // ack를 보냈다고 가정하고 ACKstate 를 0 으로 변경해주어 ACK를 보내지 않는 상태를 의미하도록 사용함 이 함수는 재사용함으로 다음번 사용에 있어서 애크를 보내지 말아야하기에

        if (send_base_B == next_seqnum_B)
        {
            //stoptimer(1, (float)TIMEOUT);//안전한 켜기를 위한 타이머 껏다 키기
            starttimer(1, (float)TIMEOUT); //timer를 시작함

        }
            
        next_seqnum_B++;    //next sequence number를 1증가시킴
    }
    else {
        printf("B_output : Buffer is full.Drop the message.\n");
        //data 전송 거부 드랍
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(struct pkt packet) 
{
    //A의 receiver이며 B에게서 packet을 받을경우 호출되는 함수
    A_ackstate = 1;// 센더가 ack를 보낼 수 있는 상태로 만들어 줌

    // 받은 패킷의 checksum 결과를 비교함
    if (getchecksum(packet) == packet.checksum) 
    {
        //checksum에 이상이 없는 경우
        printf("A_input : recv packet (seq = %d) : ", packet.seqnum);
        for (int i = 0; i < 20; i++)
            printf("%c", packet.payload[i]);
        printf("\n");

        int acknum = packet.acknum; //packet의 acknum을 추출
        int seqnum = packet.seqnum; //packet의 seqnum을 추출
        /*
        //디버깅을 위해 추출값을 확인하는 출력문이다.
        //printf("seqnum : %d \n", seqnum);
        //printf("expected_seqnum_B : %d \n", expected_seqnum_B);
        //printf("next_seqnum_B : %d \n", next_seqnum_B);
        */
        int ack_check=0;


        //receiver로서 동작
        if (expected_seqnum_A == seqnum)
        {
            // Expected_seqnum == next_seqnum(즉 패킷의 seqnum)이유는 패킷을 만들 때 넥스트 시퀀스넘버를 넣어줌으로 기대하는 시퀀스는 패킷내부의 시퀀스 넘버이당.
            //(여기 부분 중요)
            tolayer5(0, packet.payload);    //layer5 에게 data 전달
            A_my_acknum = expected_seqnum_A; //acknum을 expected sequence number로 저장하며 갱신
            printf("A_input : make ACK and B사이드에 보낼 ACK (누적 ack 혹은 실시간 으로 갱신한 ack = %d)\n", A_my_acknum);
            expected_seqnum_A++;    //B의 expected seqeunce number를 1증가시킴 이유는 다음에도 같은 영역으로 전달하는 패킷을 받을 때 확인해야하고 이때 기대하는 시퀀스 넘버는 이 부분을 지난 다음
                                    //숫자 1이 증가한 상태에서 비교 해야함으로 (여기 부분 중요)
            ack_check++; //ack를 채크함으로써 999가 들어왔을 때 초기의 ack를 가지고 있지 않아서 999가 들어왔는지 nak의 역할을 하는 999인지 판단한다. 

        }
        else 
        {
            //out of order의 경우
            printf("A_input : not the expected seq. (expected_seq = %d)\n", expected_seqnum_A); //따로 네크는 보내주지 않는다.

        }
        //sender로서의 동작
        if (acknum != 999&& ack_check !=0 )  //acknum 이 999가 아닐때, 즉 NAK가 아닐때
        {
            send_base_A = acknum + 1;
            if (send_base_A == next_seqnum_A)
            {
                
                stoptimer(0, (float)TIMEOUT);//타이머 끄기
                printf("A_input : stop timer.\n");
            }
            else
            {
               
                stoptimer(0, (float)TIMEOUT);//안전한 켜기를 위한 타이머 껏다 키기
                starttimer(0, (float)TIMEOUT);//타이머 켜기 타이머 연장 느낌
                printf("A_input : start timer.\n");
            }
        }
        
        else if(acknum == 999 && ack_check==0)//acknum이 실행 초기의 ack를 가지고 있지 않아서 999일때를 제외하고 즉 NAK의 역할을 하는 999일때
        {
            //Got NAK
            printf("A_input : got NAK (ack = %d) drop\n", packet.acknum);// 네크는 처리 않고 그냥 버림
        }
        
    }
    else  //checksum에 이상이 있는 경우 받은 packet 버림
    {
        //Packet corrupted
        printf("A_input : Packet corrupted (seq = %d). Drop\n", packet.seqnum);//checksum에 이상이 있는 경우 받은 packet 버림
    }
}


B_input(struct pkt packet)
{
    //B의 영역의 receiver이며 A영역 에게서 packet을 받을경우 호출되는 함수
    B_ackstate = 1;//  B영역 센더가 ack를 보낼 수 있는 상태로 만들어 줌

    // 받은 패킷의 checksum 결과 비교
    if (getchecksum(packet) == packet.checksum) 
    {
        //checksum에 이상이 없는 경우
        printf("B_input : recv packet (seq = %d) : ", packet.seqnum);
        for (int i = 0; i < 20; i++)
            printf("%c", packet.payload[i]);
        printf("\n");
        int acknum = packet.acknum; //packet의 acknum 추출
        int seqnum = packet.seqnum; //packet의 seqnum 추출


        /*
        //디버깅을 위해 추출값을 확인하는 출력문이다.
        //printf("seqnum : %d \n", seqnum);
        //printf("expected_seqnum_A : %d \n", expected_seqnum_A);
        //printf("next_seqnum_A : %d \n", next_seqnum_A);
        */

        int ack_check = 0;//패킷이 데이터가 기대하는 시크넘을 만족하여 액크를 생산하는지 확인하는 변수

        //receiver로서의 동작
        if (expected_seqnum_B == seqnum) 
        {
            // Expected_seqnum == next_seqnum(즉 패킷의 seqnum)이유는 패킷을 만들 때 넥스트 시퀀스넘버를 넣어줌으로 기대하는 시퀀스는 패킷내부의 시퀀스 넘버이당.
            //(여기 부분 중요)
            tolayer5(1, packet.payload);    //layer5 에게 data 전달
            B_my_acknum = expected_seqnum_B; //acknum을 expected sequence number로 저장하며 갱신
            printf("B_input : make ACK and A 사이드에 보낼 ACK (누적 ack 혹은 실시간 으로 갱신한 ack = %d)\n", B_my_acknum);
            expected_seqnum_B++;     //B의 expected seqeunce number를 1증가시킴 이유는 다음에도 같은 영역으로 전달하는 패킷을 받을 때 확인해야하고 이때 기대하는 시퀀스 넘버는 이 부분을 지난 다음
                                    //숫자 1이 증가한 상태에서 비교 해야함으로 (여기 부분 중요)
            ack_check++; //ack를 채크함으로써 999가 들어왔을 때 초기의 ack를 가지고 있지 않아서 999가 들어왔는지 nak의 역할을 하는 999인지 판단한다. 
        }
        else 
        {
            //out of order의 경우 drop
            printf("B_input : not the expected seq. (expected_seq = %d)\n", expected_seqnum_B);

        }
        //sender로서의 동작
        if (acknum != 999&& ack_check !=0 )  //acknum 이 999가 아닐때, 즉 NAK가 아닐때
        {
            send_base_B = acknum + 1;
            if (send_base_B == next_seqnum_B)
            {
                //Stop timer
                stoptimer(1, (float)TIMEOUT);
                printf("B_input : stop timer.\n");
            }
            else
            {
                //Start timer
                stoptimer(1, (float)TIMEOUT);//안전한 켜기를 위한 타이머 껏다 키기
                starttimer(1, (float)TIMEOUT);//타이머 켜기 타이머 연장 느낌
                printf("B_input : start timer.\n");
            }
        }
        else if(acknum == 999&&ack_check == 0)//acknum이 실행 초기의 ack를 가지고 있지 않아서 999일때를 제외하고 즉 NAK의 역할을 하는 999일때
        {
            //Got NAK
            printf("B_input : got NAK (ack = %d). Drop\n", packet.acknum);// 네크는 처리 않고 그냥 버림
        }
        
    }
    else  //checksum에 이상이 있는 경우 받은 packet 버림
    {
        //Packet corrupted
        printf("B_input : Packet corrupted (seq = %d). Drop\n", packet.seqnum);//checksum에 이상이 있는 경우 받은 packet 버림
    }
}

void A_timerinterrupt(void)
{
    //timeout시 호출되는 함수
    //printf("A 호출했습당-------\n");
    for (int i = send_base_A-1; i < next_seqnum_A-1; i++) //현재 base 부터 nextsequence number-1까지의 packet 재전송 여기서 send_base_A-1가 현재 base임
    {
        printf("A_timerinterrupt : resend packet (seq = %d). data : ", pkt_A_window[i].seqnum);
        for (int j = 0; j < 20; j++)
            printf("%c", pkt_A_window[i].payload[j]);
        printf("\n");

        //재전송
        tolayer3(0, pkt_A_window[i]);
    }
    //stoptimer(0, (float)TIMEOUT);//안전한 켜기를 위한 타이머 껏다 키기
    starttimer(0, (float)TIMEOUT); //타이머 시작
}

//timeout시 호출되는 함수
void B_timerinterrupt(void)
{
   //printf("B 호출했습당-------\n");
    for (int i = send_base_B-1; i < next_seqnum_B-1; i++)    //현재 base 부터 nextsequence number-1까지의 packet 재전송 여기서 send_base_B-1가 현재 base임
    {
        printf("B_timerinterrupt : resend packet (seq = %d). data : ", pkt_B_window[i].seqnum);
        for (int j = 0; j < 20; j++)
            printf("%c", pkt_B_window[i].payload[j]);
        printf("\n");
        tolayer3(1, pkt_B_window[i]);
    }
    //stoptimer(1, (float)TIMEOUT);//안전한 켜기를 위한 타이머 껏다 키기
    starttimer(1, (float)TIMEOUT); //타이머 시작
}



/*****************************************************************
***************** NETWORK EMULATION CODE STARTS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
******************************************************************/

struct event {
    float evtime;           /* event time */
    int evtype;             /* event type code */
    int eventity;           /* entity where event occurs */
    struct pkt* pktptr;     /* ptr to packet (if any) assoc w/ this event */
    struct event* prev;
    struct event* next;
};
struct event* evlist = NULL;   /* the event list */

/* possible events: */
#define  TIMER_INTERRUPT 0  
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1



int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time = 0.000;
float lossprob;            /* probability that a packet is dropped  */
float corruptprob;         /* probability that one bit is packet is flipped */
float lambda;              /* arrival rate of messages from layer 5 */
int   ntolayer3;           /* number sent into layer 3 */
int   nlost;               /* number lost in media */
int ncorrupt;              /* number corrupted by media*/

main()
{
    struct event* eventptr;
    struct msg  msg2give;
    struct pkt  pkt2give;

    int i, j;
    char c;

    init();
    A_init();
    B_init();

    while (1) {
        eventptr = evlist;            /* get next event to simulate */
        if (eventptr == NULL)
            goto terminate;
        evlist = evlist->next;        /* remove this event from event list */
        if (evlist != NULL)
            evlist->prev = NULL;
        if (TRACE >= 2) {
            printf("\nEVENT time: %f,", eventptr->evtime);
            printf("  type: %d", eventptr->evtype);
            if (eventptr->evtype == 0)
                printf(", timerinterrupt  ");
            else if (eventptr->evtype == 1)
                printf(", fromlayer5 ");
            else
                printf(", fromlayer3 ");
            printf(" entity: %d\n", eventptr->eventity);
        }
        time = eventptr->evtime;        /* update time to next event time */
        if (nsim == nsimmax)
            break;                        /* all done with simulation */
        if (eventptr->evtype == FROM_LAYER5) {
            generate_next_arrival();   /* set up future arrival */
            /* fill in msg to give with string of same letter */
            j = nsim % 26;
            for (i = 0; i < 20; i++)
                msg2give.data[i] = 97 + j;
            if (TRACE > 2) {
                printf("          MAINLOOP: data given to student: ");
                for (i = 0; i < 20; i++)
                    printf("%c", msg2give.data[i]);
                printf("\n");
            }
            nsim++;
            if (eventptr->eventity == A)
                A_output(msg2give);
            else
                B_output(msg2give);
        }
        else if (eventptr->evtype == FROM_LAYER3) {
            pkt2give.seqnum = eventptr->pktptr->seqnum;
            pkt2give.acknum = eventptr->pktptr->acknum;
            pkt2give.checksum = eventptr->pktptr->checksum;
            for (i = 0; i < 20; i++)
                pkt2give.payload[i] = eventptr->pktptr->payload[i];
            if (eventptr->eventity == A)      /* deliver packet by calling */
                A_input(pkt2give);            /* appropriate entity */
            else
                B_input(pkt2give);
            free(eventptr->pktptr);          /* free the memory for packet */
        }
        else if (eventptr->evtype == TIMER_INTERRUPT) {
            if (eventptr->eventity == A)
                A_timerinterrupt();
            else
                B_timerinterrupt();
        }
        else {
            printf("INTERNAL PANIC: unknown event type \n");
        }
        free(eventptr);
    }

terminate:
    printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n", time, nsim);
}



init()                         /* initialize the simulator */
{
    int i;
    float sum, avg;
    float jimsrand();


    printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
    printf("Enter the number of messages to simulate: ");
    scanf("%d", &nsimmax);
    printf("Enter  packet loss probability [enter 0.0 for no loss]:");
    scanf("%f", &lossprob);
    printf("Enter packet corruption probability [0.0 for no corruption]:");
    scanf("%f", &corruptprob);
    printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
    scanf("%f", &lambda);
    printf("Enter TRACE:");
    scanf("%d", &TRACE);

    srand(9999);              /* init random number generator */
    sum = 0.0;                /* test random number generator for students */
    for (i = 0; i < 1000; i++)
        sum = sum + jimsrand();    /* jimsrand() should be uniform in [0,1] */
    avg = sum / 1000.0;
    if (avg < 0.25 || avg > 0.75) {
        printf("It is likely that random number generation on your machine\n");
        printf("is different from what this emulator expects.  Please take\n");
        printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
        exit(0);
    }

    ntolayer3 = 0;
    nlost = 0;
    ncorrupt = 0;

    time = 0.0;                    /* initialize time to 0.0 */
    generate_next_arrival();     /* initialize event list */
}

/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
    double mmm = RAND_MAX;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
    float x;                   /* individual students may need to change mmm */
    x = rand() / mmm;            /* x should be uniform in [0,1] */
    return(x);
}

/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

generate_next_arrival()
{
    double x, log(), ceil();
    struct event* evptr;
    char* malloc();
    float ttime;
    int tempint;

    if (TRACE > 2)
        printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

    x = lambda * jimsrand() * 2;  /* x is uniform on [0,2*lambda] */
                              /* having mean of lambda        */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = time + x;
    evptr->evtype = FROM_LAYER5;
    if (BIDIRECTIONAL && (jimsrand() > 0.5))
        evptr->eventity = B;
    else
        evptr->eventity = A;
    insertevent(evptr);
}


insertevent(p)
struct event* p;
{
    struct event* q, * qold;

    if (TRACE > 2) {
        printf("            INSERTEVENT: time is %lf\n", time);
        printf("            INSERTEVENT: future time will be %lf\n", p->evtime);
    }
    q = evlist;     /* q points to header of list in which p struct inserted */
    if (q == NULL) {   /* list is empty */
        evlist = p;
        p->next = NULL;
        p->prev = NULL;
    }
    else {
        for (qold = q; q != NULL && p->evtime > q->evtime; q = q->next)
            qold = q;
        if (q == NULL) {   /* end of list */
            qold->next = p;
            p->prev = qold;
            p->next = NULL;
        }
        else if (q == evlist) { /* front of list */
            p->next = evlist;
            p->prev = NULL;
            p->next->prev = p;
            evlist = p;
        }
        else {     /* middle of list */
            p->next = q;
            p->prev = q->prev;
            q->prev->next = p;
            q->prev = p;
        }
    }
}

printevlist()
{
    struct event* q;
    int i;
    printf("--------------\nEvent List Follows:\n");
    for (q = evlist; q != NULL; q = q->next) {
        printf("Event time: %f, type: %d entity: %d\n", q->evtime, q->evtype, q->eventity);
    }
    printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
stoptimer(AorB)
int AorB;  /* A or B is trying to stop timer */
{
    struct event* q, * qold;

    if (TRACE > 2)
        printf("          STOP TIMER: stopping timer at %f\n", time);
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            /* remove this event */
            if (q->next == NULL && q->prev == NULL)
                evlist = NULL;         /* remove first and only event on list */
            else if (q->next == NULL) /* end of list - there is one in front */
                q->prev->next = NULL;
            else if (q == evlist) { /* front of list - there must be event after */
                q->next->prev = NULL;
                evlist = q->next;
            }
            else {     /* middle of list */
                q->next->prev = q->prev;
                q->prev->next = q->next;
            }
            free(q);
            return;
        }
    printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


starttimer(AorB, increment)
int AorB;  /* A or B is trying to stop timer */
float increment;
{

    struct event* q;
    struct event* evptr;
    char* malloc();

    if (TRACE > 2)
        printf("          START TIMER: starting timer at %f\n", time);
    /* be nice: check to see if timer is already started, if so, then  warn */
   /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == TIMER_INTERRUPT && q->eventity == AorB)) {
            printf("Warning: attempt to start a timer that is already started\n");
            return;
        }

    /* create future event for when timer goes off */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtime = time + increment;
    evptr->evtype = TIMER_INTERRUPT;
    evptr->eventity = AorB;
    insertevent(evptr);
}


/************************** TOLAYER3 ***************/
tolayer3(AorB, packet)
int AorB;  /* A or B is trying to stop timer */
struct pkt packet;
{
    struct pkt* mypktptr;
    struct event* evptr, * q;
    char* malloc();
    float lastime, x, jimsrand();
    int i;


    ntolayer3++;

    /* simulate losses: */
    if (jimsrand() < lossprob) {
        nlost++;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being lost\n");
        return;
    }

    /* make a copy of the packet student just gave me since he/she may decide */
    /* to do something with the packet after we return back to him/her */
    mypktptr = (struct pkt*)malloc(sizeof(struct pkt));
    mypktptr->seqnum = packet.seqnum;
    mypktptr->acknum = packet.acknum;
    mypktptr->checksum = packet.checksum;
    for (i = 0; i < 20; i++)
        mypktptr->payload[i] = packet.payload[i];
    if (TRACE > 2) {
        printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
            mypktptr->acknum, mypktptr->checksum);
        for (i = 0; i < 20; i++)
            printf("%c", mypktptr->payload[i]);
        printf("\n");
    }

    /* create future event for arrival of packet at the other side */
    evptr = (struct event*)malloc(sizeof(struct event));
    evptr->evtype = FROM_LAYER3;   /* packet will pop out from layer3 */
    evptr->eventity = (AorB + 1) % 2; /* event occurs at other entity */
    evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
  /* finally, compute the arrival time of packet at the other end.
     medium can not reorder, so make sure packet arrives between 1 and 10
     time units after the latest arrival time of packets
     currently in the medium on their way to the destination */
    lastime = time;
    /* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
    for (q = evlist; q != NULL; q = q->next)
        if ((q->evtype == FROM_LAYER3 && q->eventity == evptr->eventity))
            lastime = q->evtime;
    evptr->evtime = lastime + 1 + 9 * jimsrand();



    /* simulate corruption: */
    if (jimsrand() < corruptprob) {
        ncorrupt++;
        if ((x = jimsrand()) < .75)
            mypktptr->payload[0] = 'Z';   /* corrupt payload */
        else if (x < .875)
            mypktptr->seqnum = 999999;
        else
            mypktptr->acknum = 999999;
        if (TRACE > 0)
            printf("          TOLAYER3: packet being corrupted\n");
    }

    if (TRACE > 2)
        printf("          TOLAYER3: scheduling arrival on other side\n");
    insertevent(evptr);
}

tolayer5(AorB, datasent)
int AorB;
char datasent[20];
{
    int i;
    if (TRACE > 2) {
        printf("          TOLAYER5: data received: ");
        for (i = 0; i < 20; i++)
            printf("%c", datasent[i]);
        printf("\n");
    }

}
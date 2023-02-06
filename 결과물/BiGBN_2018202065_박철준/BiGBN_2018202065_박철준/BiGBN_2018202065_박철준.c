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
/* ����� ��鿣 �������ݷ� ����*/


/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
// layer 5���� ���޵Ǵ� ������ ����
struct msg {
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
// layer 4���� ���޵Ǵ� ����
struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];    //msg�� data
};


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int send_base_A;// A send�� window base num
int send_base_B;// B send�� window base num

int next_seqnum_A; // A send���� ���� ���� packet��ġ
int next_seqnum_B; // B send���� ���� ���� packet��ġ

int expected_seqnum_A; // A receive���� ���ϴ� seqnum��ȣ
int expected_seqnum_B; // B receive���� ���ϴ� seqnum��ȣ

int A_ackstate = 0;   //ack�� ������ �������� �ƴ��� �����ϴ� ����
int B_ackstate = 0;   //ack�� ������ �������� �ƴ��� �����ϴ� ���� 

int A_my_acknum = 0;  //���� ack�� ���� ����, ack������ ���������� 999�� ����
int B_my_acknum = 0;  //���� ack�� ���� ����, ack������ ���������� 999�� ����

#define WINDOWSIZE 40   //window size N = 40���� ��
#define TIMEOUT 1000 //TIME OUT�� 10���� ��

struct pkt pkt_A_window[1024];  //A output���� ������ ��Ŷ�� �����ϴ� ����̸� ���Ŀ� �����찡 �� �� �������� ����� ������� �˳��� ����ָ� ��Ȯ�� ������ ������� WINDOWSIZE�� ����
struct pkt pkt_B_window[1024];  //B output���� ������ ��Ŷ�� �����ϴ� ����̸� ���Ŀ� �����찡 �� �� �������� ����� ������� �˳��� ����ָ� ��Ȯ�� ������ ������� WINDOWSIZE�� ����

//A side ������ �ʱ�ȭ
void A_init(void)
{
    memset(pkt_A_window, 0, 1024);
    send_base_A = 1;
    next_seqnum_A = 1;
    expected_seqnum_A = 1;
    A_ackstate = 0;
    A_my_acknum = 0;
}

//B side ������ �ʱ�ȭ
void B_init(void)
{
    memset(pkt_B_window, 0, 1024);
    send_base_B = 1;
    next_seqnum_B = 1;
    expected_seqnum_B = 1;
    B_ackstate = 0;
    B_my_acknum = 0;
}

//checksum�� ����ϴ� �Լ�
//seqnum, acknum, payload�� �� ������ �ƽ�Ű�ڵ��� ���� ��ȯ
//��� �� �������ν� �ϳ��� Ʋ���ٸ� �Ǻ�������

int getchecksum(struct pkt packet)
{
    // checksum ���ϴ� �Լ�
    //����Ʈ�� ũ���� ������δ� �������� ���Ͽ����ϴ�
    int checksum = 0;
    checksum += packet.seqnum;
    checksum += packet.acknum;
    for (int i = 0; i < sizeof(packet.payload) / sizeof(char); i++)
        checksum += packet.payload[i];    //��� ���Ͽ� checksum����
    return checksum;
}

struct pkt pkt_make(int seqnum, int acknum, char* data)
{
    //packet�� ����� �Լ�
    // 
    //���ο� packet�� �����ϰ� �ʱ�ȭ�ϴ� ����
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
    //layer5���� ������ message�� �ް� ��Ŷ�� ����� A���� B���� ��Ŷ�� ��ũ�� �Բ� ���������� ȣ��Ǵ� �Լ�

    if (next_seqnum_A < send_base_A + WINDOWSIZE)
    {
        if (A_ackstate == 0)
            A_my_acknum = 999;   //�ٸ� ���̵��� sender���Լ� ���� ack�� �������� �ʴ� ��� 
        struct pkt received_packet_for_layer5;    //���� packet
        received_packet_for_layer5 = pkt_make(next_seqnum_A, A_my_acknum, message.data); //message�� �����ϰ� packet�� ������
        pkt_A_window[next_seqnum_A-1] = received_packet_for_layer5;  //A ���̵��� window�� packet�� ����
        tolayer3(0, received_packet_for_layer5);   //packet�� B_input���� ���� ����
        if (A_my_acknum!=999)   //ack�� ���� ������ ��� ���
        {
            printf("A_output : send_packet with ACK (ACK = %d, seq = %d) : ", received_packet_for_layer5.acknum, received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        else //nak���� acknum�� 999�� ������ ��� ���
        {
            printf("A_output : send_packet without ACK (seq = %d) : ", received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        A_ackstate = 0;   // ack�� ���´ٰ� �����ϰ� ACKstate �� 0 ���� �������־� ACK�� ������ �ʴ� ���¸� �ǹ��ϵ��� �����

        if (send_base_A == next_seqnum_A)
        {
            //stoptimer(0, (float)TIMEOUT);//������ �ѱ⸦ ���� Ÿ�̸� ���� Ű��
            starttimer(0, (float)TIMEOUT); //timer�� ������
        }
            
        next_seqnum_A++;    //next sequence number�� 1������Ŵ
        
    }
    else
    {
        printf("A_output : Buffer is full.Drop the message.\n");

        //data ������ �ź��Ѵ�.
    }
}

B_output(struct msg message)  
{
    //layer5���� ������ message�� �ް� ��Ŷ�� ����� B���� A���� ��Ŷ�� ��ũ�� �Բ� ���������� ȣ��Ǵ� �Լ�
    if (next_seqnum_B < send_base_B + WINDOWSIZE)
    {
        if (B_ackstate == 0)
            B_my_acknum = 999;   //�ٸ� ���̵��� sender���Լ� ���� ack�� �������� �ʴ� ���  
        struct pkt received_packet_for_layer5;    //���� packet
        received_packet_for_layer5 = pkt_make(next_seqnum_B, B_my_acknum, message.data);  //message�� �����ϰ� packet�� ������
        pkt_B_window[next_seqnum_B-1] = received_packet_for_layer5;  //B ���̵��� window�� packet�� ����
        tolayer3(1, received_packet_for_layer5);   //packet�� A_input���� ���� ����
        if (B_my_acknum!=999)   //ack�� ���� ������ ��� ���
        {
            printf("B_output : send_packet with ACK (ACK = %d, seq = %d) : ", received_packet_for_layer5.acknum, received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        else //nak���� acknum�� 999�� ������ ��� ���
        {
            printf("B_output : send_packet without ACK (seq = %d) : ", received_packet_for_layer5.seqnum);
            for (int i = 0; i < 20; i++)
                printf("%c", received_packet_for_layer5.payload[i]);
            printf("\n");
        }
        B_ackstate = 0;   // ack�� ���´ٰ� �����ϰ� ACKstate �� 0 ���� �������־� ACK�� ������ �ʴ� ���¸� �ǹ��ϵ��� ����� �� �Լ��� ���������� ������ ��뿡 �־ ��ũ�� ������ ���ƾ��ϱ⿡

        if (send_base_B == next_seqnum_B)
        {
            //stoptimer(1, (float)TIMEOUT);//������ �ѱ⸦ ���� Ÿ�̸� ���� Ű��
            starttimer(1, (float)TIMEOUT); //timer�� ������

        }
            
        next_seqnum_B++;    //next sequence number�� 1������Ŵ
    }
    else {
        printf("B_output : Buffer is full.Drop the message.\n");
        //data ���� �ź� ���
    }
}

/* called from layer 3, when a packet arrives for layer 4 */
A_input(struct pkt packet) 
{
    //A�� receiver�̸� B���Լ� packet�� ������� ȣ��Ǵ� �Լ�
    A_ackstate = 1;// ������ ack�� ���� �� �ִ� ���·� ����� ��

    // ���� ��Ŷ�� checksum ����� ����
    if (getchecksum(packet) == packet.checksum) 
    {
        //checksum�� �̻��� ���� ���
        printf("A_input : recv packet (seq = %d) : ", packet.seqnum);
        for (int i = 0; i < 20; i++)
            printf("%c", packet.payload[i]);
        printf("\n");

        int acknum = packet.acknum; //packet�� acknum�� ����
        int seqnum = packet.seqnum; //packet�� seqnum�� ����
        /*
        //������� ���� ���Ⱚ�� Ȯ���ϴ� ��¹��̴�.
        //printf("seqnum : %d \n", seqnum);
        //printf("expected_seqnum_B : %d \n", expected_seqnum_B);
        //printf("next_seqnum_B : %d \n", next_seqnum_B);
        */
        int ack_check=0;


        //receiver�μ� ����
        if (expected_seqnum_A == seqnum)
        {
            // Expected_seqnum == next_seqnum(�� ��Ŷ�� seqnum)������ ��Ŷ�� ���� �� �ؽ�Ʈ �������ѹ��� �־������� ����ϴ� �������� ��Ŷ������ ������ �ѹ��̴�.
            //(���� �κ� �߿�)
            tolayer5(0, packet.payload);    //layer5 ���� data ����
            A_my_acknum = expected_seqnum_A; //acknum�� expected sequence number�� �����ϸ� ����
            printf("A_input :got ACK and make ACK �� B���̵忡 ���� ACK (���� ack Ȥ�� �ǽð� ���� ������ ack = %d)\n", A_my_acknum);
            expected_seqnum_A++;    //B�� expected seqeunce number�� 1������Ŵ ������ �������� ���� �������� �����ϴ� ��Ŷ�� ���� �� Ȯ���ؾ��ϰ� �̶� ����ϴ� ������ �ѹ��� �� �κ��� ���� ����
                                    //���� 1�� ������ ���¿��� �� �ؾ������� (���� �κ� �߿�)
            ack_check++; //ack�� äũ�����ν� 999�� ������ �� �ʱ��� ack�� ������ ���� �ʾƼ� 999�� ���Դ��� nak�� ������ �ϴ� 999���� �Ǵ��Ѵ�. 

        }
        else 
        {
            //out of order�� ���
            printf("A_input : not the expected seq. (expected_seq = %d)\n", expected_seqnum_A); //���� ��ũ�� �������� �ʴ´�.

        }
        //sender�μ��� ����
        if (acknum != 999&& ack_check !=0 )  //acknum �� 999�� �ƴҶ�, �� NAK�� �ƴҶ�
        {
            send_base_A = acknum + 1;
            if (send_base_A == next_seqnum_A)
            {
                
                stoptimer(0, (float)TIMEOUT);//Ÿ�̸� ����
                printf("A_input : stop timer.\n");
            }
            else
            {
               
                stoptimer(0, (float)TIMEOUT);//������ �ѱ⸦ ���� Ÿ�̸� ���� Ű��
                starttimer(0, (float)TIMEOUT);//Ÿ�̸� �ѱ� Ÿ�̸� ���� ����
                printf("A_input : start timer.\n");
            }
        }
        
        else if(acknum == 999 && ack_check==0)//acknum�� ���� �ʱ��� ack�� ������ ���� �ʾƼ� 999�϶��� �����ϰ� �� NAK�� ������ �ϴ� 999�϶�
        {
            //Got NAK
            printf("A_input : got NAK (ack = %d) drop\n", packet.acknum);// ��ũ�� ó�� �ʰ� �׳� ����
        }
        
    }
    else  //checksum�� �̻��� �ִ� ��� ���� packet ����
    {
        //Packet corrupted
        printf("A_input : Packet corrupted (seq = %d). Drop\n", packet.seqnum);//checksum�� �̻��� �ִ� ��� ���� packet ����
    }
}


B_input(struct pkt packet)
{
    //B�� ������ receiver�̸� A���� ���Լ� packet�� ������� ȣ��Ǵ� �Լ�
    B_ackstate = 1;//  B���� ������ ack�� ���� �� �ִ� ���·� ����� ��

    // ���� ��Ŷ�� checksum ��� ��
    if (getchecksum(packet) == packet.checksum) 
    {
        //checksum�� �̻��� ���� ���
        printf("B_input : recv packet (seq = %d) : ", packet.seqnum);
        for (int i = 0; i < 20; i++)
            printf("%c", packet.payload[i]);
        printf("\n");
        int acknum = packet.acknum; //packet�� acknum ����
        int seqnum = packet.seqnum; //packet�� seqnum ����


        /*
        //������� ���� ���Ⱚ�� Ȯ���ϴ� ��¹��̴�.
        //printf("seqnum : %d \n", seqnum);
        //printf("expected_seqnum_A : %d \n", expected_seqnum_A);
        //printf("next_seqnum_A : %d \n", next_seqnum_A);
        */

        int ack_check = 0;//��Ŷ�� �����Ͱ� ����ϴ� ��ũ���� �����Ͽ� ��ũ�� �����ϴ��� Ȯ���ϴ� ����

        //receiver�μ��� ����
        if (expected_seqnum_B == seqnum) 
        {
            // Expected_seqnum == next_seqnum(�� ��Ŷ�� seqnum)������ ��Ŷ�� ���� �� �ؽ�Ʈ �������ѹ��� �־������� ����ϴ� �������� ��Ŷ������ ������ �ѹ��̴�.
            //(���� �κ� �߿�)
            tolayer5(1, packet.payload);    //layer5 ���� data ����
            B_my_acknum = expected_seqnum_B; //acknum�� expected sequence number�� �����ϸ� ����
            printf("B_input :got ACK and make ACK �� A ���̵忡 ���� ACK (���� ack Ȥ�� �ǽð� ���� ������ ack = %d)\n", B_my_acknum);
            expected_seqnum_B++;     //B�� expected seqeunce number�� 1������Ŵ ������ �������� ���� �������� �����ϴ� ��Ŷ�� ���� �� Ȯ���ؾ��ϰ� �̶� ����ϴ� ������ �ѹ��� �� �κ��� ���� ����
                                    //���� 1�� ������ ���¿��� �� �ؾ������� (���� �κ� �߿�)
            ack_check++; //ack�� äũ�����ν� 999�� ������ �� �ʱ��� ack�� ������ ���� �ʾƼ� 999�� ���Դ��� nak�� ������ �ϴ� 999���� �Ǵ��Ѵ�. 
        }
        else 
        {
            //out of order�� ��� drop
            printf("B_input : not the expected seq. (expected_seq = %d)\n", expected_seqnum_B);

        }
        //sender�μ��� ����
        if (acknum != 999&& ack_check !=0 )  //acknum �� 999�� �ƴҶ�, �� NAK�� �ƴҶ�
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
                stoptimer(1, (float)TIMEOUT);//������ �ѱ⸦ ���� Ÿ�̸� ���� Ű��
                starttimer(1, (float)TIMEOUT);//Ÿ�̸� �ѱ� Ÿ�̸� ���� ����
                printf("B_input : start timer.\n");
            }
        }
        else if(acknum == 999&&ack_check == 0)//acknum�� ���� �ʱ��� ack�� ������ ���� �ʾƼ� 999�϶��� �����ϰ� �� NAK�� ������ �ϴ� 999�϶�
        {
            //Got NAK
            printf("B_input : got NAK (ack = %d). Drop\n", packet.acknum);// ��ũ�� ó�� �ʰ� �׳� ����
        }
        
    }
    else  //checksum�� �̻��� �ִ� ��� ���� packet ����
    {
        //Packet corrupted
        printf("B_input : Packet corrupted (seq = %d). Drop\n", packet.seqnum);//checksum�� �̻��� �ִ� ��� ���� packet ����
    }
}

void A_timerinterrupt(void)
{
    //timeout�� ȣ��Ǵ� �Լ�
    //printf("A ȣ���߽���-------\n");
    for (int i = send_base_A-1; i < next_seqnum_A-1; i++) //���� base ���� nextsequence number-1������ packet ������ ���⼭ send_base_A-1�� ���� base��
    {
        printf("A_timerinterrupt : resend packet (seq = %d). data : ", pkt_A_window[i].seqnum);
        for (int j = 0; j < 20; j++)
            printf("%c", pkt_A_window[i].payload[j]);
        printf("\n");

        //������
        tolayer3(0, pkt_A_window[i]);
    }
    //stoptimer(0, (float)TIMEOUT);//������ �ѱ⸦ ���� Ÿ�̸� ���� Ű��
    starttimer(0, (float)TIMEOUT); //Ÿ�̸� ����
}

//timeout�� ȣ��Ǵ� �Լ�
void B_timerinterrupt(void)
{
   //printf("B ȣ���߽���-------\n");
    for (int i = send_base_B-1; i < next_seqnum_B-1; i++)    //���� base ���� nextsequence number-1������ packet ������ ���⼭ send_base_B-1�� ���� base��
    {
        printf("B_timerinterrupt : resend packet (seq = %d). data : ", pkt_B_window[i].seqnum);
        for (int j = 0; j < 20; j++)
            printf("%c", pkt_B_window[i].payload[j]);
        printf("\n");
        tolayer3(1, pkt_B_window[i]);
    }
    //stoptimer(1, (float)TIMEOUT);//������ �ѱ⸦ ���� Ÿ�̸� ���� Ű��
    starttimer(1, (float)TIMEOUT); //Ÿ�̸� ����
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
backpressure 해결 전략으로 보통 queue를 사용함.  
프로세스 간 IPC 리소스에서 pipe, fifo, msg queue는 사이즈 제약이 있지만   
별도의 동기화 작업이 없이도 기본적인 큐잉을 제공함.  

## Pipe

단방향. fork 해서 related proc 간에만 사용 가능.  
PIPE_BUF보다 큰 쓰기 작업시 atomic 하지 않으므로 주의.  

pipe(int fd[2]) // fd[0] reader, fd[1] writer

## FIFO (named pipe)

파일 이름이 있어 unrelated proc 간에도 데이터 전달 가능   
open 시 reader, sender 모두 open될 때 까지 한 쪽은 blocking 상태 대기  

mkfifo()

## sys v Message Queue

가급적 POSIX mq를 이용하십시오.  

msg 별 type 을 담아 send할 수 있고, mq를 수신하는 프로세스에서는 특정 type만 가져올 수 있음   

msgget() 이 반환하는 sysv mq의 식별자는 `IPC Key 기반 식별자 (fd가 아님에 주의. 별도의 key임)`
    - 시스템 내에서 겹치면 안됨. key 값에 IPC\_PRIVATE 를 통해 겹치지 않는 key를 자동으로 반환 받을 수
    - key\_t = ftok(const char * pathname, int proj\_id)  // key값을 생성할 수 있지만 unique 하단 보장이 없음.

`ipcs`, `ipcrm` 등 sysv 계열 IPC 자원 다루는 cli로 제어가 가능함  

msgget(key, flag)  

msgsnd(msqid, * msqp, msgsz, flag)  
    - msgp 는 반드시 struct msgbuf { long mtype; char mtext[...]; } 형태여야 함
    - msgsz 는 struct msgbuf 사이즈가 아니라 mtype을 제외한 char mtext[...] 의 크기를 전달해야 함
    - IPC\_NOWAIT 를 flag에 줘야 비동기로 snd함

msgrcv(msqid, * msgp, msgsz, msgtype, flag)  
    - msgp는 struct msgbuf
    - msgsz에 mtext의 길이
    - msgtype
        - 0: 첫번째 메시지 수신
        - 양수 : msgtype에 매치되는 첫번째 메시지 수신
        - 음수 : 지정된 절대값보다 작거나 같은 msgtype에 매치되는 첫번째 메시지 수
    - flag
        - IPC\_NOWAIT   : non blocking IO
        - MSG_COPY      : msgtype이 index로 사용하여 index 번째 메시지를 복사해서 수. IPC\_NOWAIT와 같이 사용해야 함
        - MSG_EXCEPT    : msgtype과 매치되지 않는 메시지를 수신
        - MSG_NOERROR   : 메시지 사이즈가 msgsz보다 크면 truncate 시

msgctl(msqid, cmd, struct msqid\_ds)  
    - msgid  
    - cmd  
        - IPC\_STAT : kernel의 msgid_ds 정보 습득  
        - IPC\_SET  : kernel의 msgid_ds에 설정  
        - IPC\_RMID : message queue 제거  
    - msqid\_ds 구조체 필드가 많으니 알아서 찾아볼 것  


## POSIX Message Queue


partial read가 불가능함. sysv mq 보다 좀 더 엄격히 메시지 경계를 지님  
file i/o 기반의 동작. io multiplexing이 가능함  
message 우선순위 적용 가능  
notification 기능  
링킹 단계에서 반드시 -lrt를 추가해야만 사용 가능  
/dev/mqueue 에서 확인 가능  

mqd\_t mq\_open(name, oflag)  
mqd\_t mq\_open(name, oflag, mode, attr)  
    - mq의 이름은 반드시 / 로 시작해야함. file 기반이기 때문  
    - mq descriptor를 반환. 이건 sysv mq와 다르게 fd이다!

mq\_close(mqd\_t) : mq를 닫는 개념
mq\_unlink(name) : mq 파일을 지우는 개념

mq\_send(mqd\_t, msg\_ptr, msg\_len, msg\_priority)
    - priority : 0 ~ 32768

mq\_receive(mqd\_t, msg\_ptr, msg\_len, * msg\_priority)
    - msg\_len : 반드시 attr.mq\_msgsize 보다 크거나 같아야 함. partial read 자체가 생길 수 없는 이유 중 하나.
    - priority : 0 ~ 32768

mq\_setattr(mqd\_t, * mq\_attr)  
mq\_getattr( * mq\_attr, * mq\_attr)  

```c
    struct mq_attr {
       long mq_flags;       Flags: 0 or O\_NONBLOCK 
       long mq_maxmsg;      Max. # of messages on queue 
       long mq_msgsize;     Max. message size (bytes) 
       long mq_curmsgs;     # of messages currently in queue 
    };
```

mq\_notify(mqd\_t, * sigevent )
    - notification 설정

```c
  struct sigevent {
      int             sigev_notify; /* Notification type */
      int             sigev_signo;  /* Signal number */
      union sigval    sigev_value;  /* Signal value */
      void          (*sigev_notify_function)(union sigval);
                                    /* Notification function */
      pthread_attr_t *sigev_notify_attributes;
                                    /* Notification attributes */
  };

  union sigval {
      int     sigval_int; /* Integer value */
      void   *sigval_ptr; /* Pointer value */
  };

```


## `ipcs` view  

show System V IPC facilities  

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x61130001 0          server01   644        0            0           

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status      

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     



## socket (datagram), UDS


UDP socket으로 전달하는 것은, msg boundary가 명확하므로 데이터를 전달하는 IPC 용으로도 사용 가능하다.
다만 UDP는 신뢰성이 없다는 점을 고려해야 한다.  
Internet Domain Socket이지만 Local Address를 사용한 socket이어야 함.  
내 생각엔 굳이 이런 패턴으로 하지 않는게 좋은 것 같다. 이런 용도로 쓰라고 UDS를 두었다.  

Unix Domain Socket(AF\_UNIX, UDS)을 사용하자. 
- UDS는 동일한 시스템 내에서 프로세스 간 통신을 위해 사용됨  
- fd 전송 가능, 신뢰성이 보장됨, host only socket, filepath로 address
- 파일 시스템을 통해 통신하며, 데이터 전송은 커널 내부에서 처리됨. 때문에 UDS는 네트워크 스택을 거치지 않기 때문에 UDP에 비해 훨씬 빠름












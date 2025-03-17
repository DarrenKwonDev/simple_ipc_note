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


## UDS


일반 socket으로 프로세스간 정보를 주고 받는 건 가능은 하다.
Internet Domain Socket이지만 Local Address를 사용한 socket이어야 함.  
내 생각엔 굳이 이런 패턴으로 하지 않는게 좋은 것 같다. 이런 용도로 쓰라고 UDS를 두었다.  

Unix Domain Socket(AF\_UNIX, UDS)을 사용하자. 
- UDS는 동일한 시스템 내에서 프로세스 간 통신을 위해 사용됨  
- fd 전송 가능, 신뢰성이 보장됨, host only socket, filepath로 address
- 파일 시스템을 통해 통신하며, 데이터 전송은 커널 내부에서 처리됨. 때문에 UDS는 네트워크 스택을 거치지 않기 때문에 UDP에 비해 훨씬 빠름
- UDS 라고 하더라도 통신 방식을 tcp, udp 중 하나로 결정하여야 함
    - tcp의 경우 partial read, write를 보완할 것
    - udp의 경우 msg boundary가 명확하여 partial read, write 걱정이 없지만 유실 가능성이 있음




## mmap

void * mmap(void addr[.length], size\_t length, int prot, int flags, int fd, off\_t offset);  
    - addr는 커널에게 원하는 주소의 '힌트' (반드시 그 주소로 주지는 않음) 대부분 NULL로 넘겨서 자동으로 할당 받음.  
    - offset은 반드시 page size의 배수  
    - prot(protection) : PROT\_EXEC | PORT\_READ | PROT\_WRITE | PROT\_NONE  
    - flag : MAP\_SHARED(shared memory로 쓰려면 필수 ), MAP\_PRIVATE, MAP\_FIXED, MAP\_ANONYMOUS  
    
int munmap(void addr[.length], size_t length);
    - mmap 으로 받은 주소를 풀겠다.  


- 일반 파일의 mmap
    - unrelated processes 간에도 사용 가능. 파일을 기준으로 mmap을 하니.
    - mmap 후에 fd를 close해도 됩니다. mmap은 파일을 메모리에 매핑한 후에는 파일 디스크립터가 더 이상 필요하지 않습니다
    - munmap을 호출해도 실제 파일은 삭제되지 않습니다. 그냥 남아 있음


- anonymous mapping (MAP\_SHARED | MAP\_ANONYMOUS)
    - 특정 file을 지정하지 않고 익명의 파일에 메모리 매핑 (/dev/zero 에 mmap한 것과 동일)
    - 그냥 malloc 한거랑 뭔 차인가?
        - malloc은 fork() 후에 Copy-on-Write 되면 완전히 다른 영역을 가리킴. 프로세스간 공유 안됨
        - anon mapping은 fork() 후에도 각 프로세스들이 접근 가능함 
    - related proc 끼리만 사용 가능


## system V shm


shmget -> shmat -> 사용 -> shmdt    


## POSIX shm



일반적인 posix shm 프로그래밍 시퀀스.
사실상 file 기반 mmap을 사용하는 것과 동일한 시퀀스인데 open 대신 shm_open을 사용한 것 정도의 차이임 .  


* create의 경우
fd = shm_open(); 
ftruncate(fd, sizse); // resize shm
ptr = mmap(fd);
munmap(ptr);
shm_unlink(fd);

* attach의 경우
fd = shm_open(); 
fstat(fd, &stat);	// attach할 때 create한 size만큼의 mmap에 attach 하기 위해서 사이즈를 얻어와야
ptr = mmap(fd, stat.st_size);
munmap(ptr);
shm_unlink(fd);

## semaphore  

- System V 세마포어: 항상 프로세스 간 동기화용  
    - 한 번에 여러 개의 세마포어를 세트(set)로 만들고 다룰 수 있는 유연성을 제공
    - 커널 내에 영속적으로 객체를 생성하고 관리하기 때문에 무겁고 인터페이스가 복잡하다는 단점이 있다
- POSIX 세마포어: 스레드 간 또는 프로세스 간 동기화 모두 가능 (IPC 자원의 동시성 문제를 해결하기 위함)  
    - 세마포어 집합 개념이 없어 필요에 따라 개별 세마포어를 여러 개 만들면 되므로 이해와 사용이 더 쉽다  
    - unnamed semaphore
    - named semaphore
- pthread 뮤텍스: 주로 스레드 간 동기화용  

counting semaphore  
binary semaphore (= mutex)  


## system V Semaphore  

1. semget(key_t semid, int nsems, int semflg)  
- 기본적으로 sysv sem은 세마포어 1세트를 만들고 그 안에 nsems 개의 semaphore가 존재함  

2. semop(semid, struct sembuf *sops, nsops)  
struct sembuf {  
   unsigned short sem_num;  // semaphore number
   short          sem_op;   // semaphore operation (양수 해제, 0 이면 sem 값이 0될 때 까지 대기, 음수면 sem 획득)
   short          sem_flg;  // operation flags (IPC_NOWAIT, SEM_UNDO)
}  

3. semctl(int semid, int semnum, int cmd, ...)


## POSIX Semaphore  

- unnamed semaphore 
    - shared memory 내의 메모리 영역을 이용  
    - 즉, shared memory 영역 일부를 semaphore로 사용  
    - sem_init, sem_destroy

- named semaphore  
    - ID를 가지는 별도의 객체 생성  
    - shared memory 이외의 자원에 대한 동기화가 필요할 때 사용하기에 좋음  
    - 프로세스 간의 동기화
    - sem_open, sem_close, sem_unlink

- 공통:  sem_wait (획득) sem_post (해제)

### unnamed semaphore  

sem_init(sem_t *sem, int pshared, unsigned int value);
- sem : 초기화 할 semaphore 위치  
- pshared : 0 : process 내 thread간 공유, 1 : process 간 공유  
- value : semaphore 초기값   

sem_destroy(sem_t *sem)
- sem 반납. 다른 proc, thread가 blocking 중인 semaphore를 destroy 하면 안 됨  

### named semaphore  

- 열기   
sem_t sem_open(const char *name, int oflag); 

- 생성   
sem_t sem_open(const char *name, int oflag, mode_t mode, unsigned int value);


### 공통  

- 해제(너 나가라)  
int sem_post(sem_t *sem);  

- 획득 요청

sem_wait : 리소스 획득 요청  
sem_trywait : 리소스 획득 시도  
sem_timedwait : 타임아웃을 시도하고 리소스 획득 요청  

## how to monitoring?

show System V IPC facilities  

------ Message Queues --------
key        msqid      owner      perms      used-bytes   messages    
0x61130001 0          server01   644        0            0           

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status      

------ Semaphore Arrays --------
key        semid      owner      perms      nsems     


POSIX IPC는 다른 방식으로 확인해야 합니다:

POSIX 공유메모리는 /dev/shm 디렉토리에서 확인
POSIX 세마포어는 /dev/shm 또는 /dev/posix/sem에서 확인
POSIX 메시지큐는 /dev/mqueue에서 확인

mmap으로 매핑된 파일은 /proc/<pid>/maps  
mkfifo 는 실제 파일이 생성되는 named pipe니까 파일을 직접 확인해야하고  
pipe는 프로세스 종료시 수거되므로 아직 프로세스가 살아 있을 때 /proc/<pid>/fd 에서 직접 확인해야  



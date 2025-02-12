


backpressure 해결 전략으로 보통 queue를 사용함.
프로세스 간 IPC 리소스에서 pipe, fifo, msg queue는 사이즈 제약이 있지만 
별도의 동기화 작업이 없이도 기본적인 큐잉을 제공함.

## Pipe

단방향. fork 해서 related proc 간에만 사용 가능.
PIPE_BUF보다 큰 쓰기 작업시 atomic 하지 않으므로 주의.

## FIFO (named pipe)

파일 이름이 있어 unrelated proc 간에도 데이터 전달 가능.
open 시 reader, sender 모두 open될 때 까지 한 쪽은 blocking 상태 대기


## Messaeg Queue

posix mq를 사용하자.
system v mq는 너무 verbose하다... 장점이 없는듯. 




# Multithreaded-TCP-Server

## 사용방법
### 클라이언트
- (실행)SERVERHOST=localhost ./echocli (포트번호)   
-> 일반 모드    
-> 클라이언트를 실행한 뒤에 한 정수값을 입력해주면 서버에 그 정수값 사이즈를 가진 랜덤 정수 배열이 전달된다.
- SERVERHOST=localhost ./echocli (포트번호) -d     
-> 디버깅 모드     
-> 클라이언트를 실행한 뒤에 정수 값을 입력해주면 정수 값 사이즈를 가지는 2부터 순차적으로 커지는 랜덤 정수 배열이 전달된다.    
### 서버    
- (실행)./multisrv -n (총 스레드 개수)     
-> 반드시 n 옵션을 주고 서버를 실행해야 한다.     
-> (총 스레드 개수)에 0이 들어가면 sequential version으로 실행된다.   
-> (총 스레드 개수)에 1이 들어가면 pthread 한개가 생성되어 해당 스레드에서 sequential version으로 실행된다.     
-> (총 스레드 개수)에 2 이상의 값이 들어가면 multithreaded server로서 해당 값만큼 스레드가 thread pool에 생성되어 실행된다. 
      
-> 서버를 실행하면 10초 뒤에 throughput을 측정하기 위해 alarm이 동작하기 시작하고 이후 10초 뒤에 알람이 종료된다.     
-> 매번 클라이언트로부터 request를 받을 때마다 서버는 latency를 측정하여 print한다.    

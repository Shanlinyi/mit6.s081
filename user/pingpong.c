//user/pingpong.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define RE 0
#define WR 1
int main(int argc,char**argv){
	int pp2c[2],pc2p[2];
	pipe(pp2c);
	pipe(pc2p);

	if(fork()!=0){
		close(pp2c[RE]);
		close(pc2p[WR]);
		write(pp2c[WR],"p",1);
		
		char buf;
		read(pc2p[RE],&buf,1);
		printf("%d recieved pong",getpid());
		wait(0);
		close(pp2c[WR]);
		close(pc2p[RE]);
	}else{
		close(pp2c[WR]);
		close(pc2p[RE]);
		char buf;
		read(pp2c[RE],&buf,1);
		printf("%d recieved ping",getpid());

		write(pc2p[WR],&buf,1);
		close(pp2c[RE]);
		close(pc2p[WR]);
		exit(0);
	}
	exit(0);
}

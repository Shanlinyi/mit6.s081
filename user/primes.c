#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define RE 0
#define WR 1
void sieve(int pleft[2]){
	int p;
	read(pleft[RE],&p,sizeof(p));
	if(p==-1){
		exit(0);
	}

	printf("prime %d\n",p);

	int pright[2];
	pipe(pright);

	if(fork()==0){
		close(pleft[RE]);
		close(pright[WR]);
		sieve(pright);
	}else{
		close(pright[RE]);
		int buf;
		while(read(pleft[RE],&buf,sizeof(buf))&&buf!=-1){
			if(buf%p!=0){
				write(pright[WR],&buf,sizeof(buf));
			}
		}
		buf=-1;
		write(pright[WR],&buf,sizeof(buf));
		close(pleft[RE]);
		close(pright[WR]);
		wait(0);
		exit(0);
	}
}

int main(int argc,char**argv){
	int input_pipe[2];
	pipe(input_pipe);

	if(fork()==0){
		close(input_pipe[WR]);
		sieve(input_pipe);
	}else{
		close(input_pipe[RE]);
		int i;
		for(i=2;i<=35;i++){
			write(input_pipe[WR],&i,sizeof(i));
		}
		i=-1;
		write(input_pipe[WR],&i,sizeof(i));
		close(input_pipe[WR]);
	}
	wait(0);
	exit(0);

}

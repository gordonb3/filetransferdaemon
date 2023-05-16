#include "Socket.h"
#include <iostream>

using namespace std;

int main(int argc, char** argv){
		int recfd;
		char buf[]="Im done";
		try{
			EUtils::UnixClientSocket sock(SOCK_STREAM, "/tmp/mysockpath");

			recfd=sock.ReceiveFd();

			size_t readsize;
			char rbuf[111];
			while((readsize=read(recfd,rbuf,sizeof(rbuf)))>0){
				cout << "Read:"<<readsize<<" bytes"<<endl;
			}

			sock.Send(buf,sizeof(buf));
		}catch(runtime_error* e){
			cout << "Caught:"<<e->what()<<endl;
		}
		return 0;
}

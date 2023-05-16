#include "AsyncWorker.h"

int main(int argc,char** argv){

    AsyncWorker aw;
    struct buffer* buf;

    for (int i=0;i<1000;i++) {
        buf=(struct buffer*)malloc(sizeof(struct buffer));
        buf->data=(unsigned char *)malloc(i);
        buf->length=i;
        aw.AddLast(buf);
        usleep(30);

    }

    aw.Cancel();

    sleep(1);

    return 0;
}

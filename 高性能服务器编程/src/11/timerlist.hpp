#ifndef TIMERLIST_H
#define TIMERLIST_H

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE (64)
class util_timer;


struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer* timer;
}

class util_timer
{
public:
    util_timer():prev(NULL),next(NULL){}

public:
    time_t expire; //@ 任务超时时间，这里使用的是绝对时间
    void (*cb_func)(client_data*);  //@ 任务回调函数

    //@ 回调函数处理的客户数据，由定时器的执行者传递给回调函数
    client_data* user_data;
    util_timer* prev;  //@ 指向前一个定时器
    util_timer* next;  //@ 指向下一个定时器
};

class sort_timer_lst
{
public:
    sort_timer_lst():head(NULL),tail(NULL)
    {
    }

    ~sort_timer_lst()
    {
        util_timer* temp = head;
        while (temp)
        {
            head = temp->next;
            delete temp;
            temp = head;
        }
    }

    void add_timer(util_timer* timer)
    {
        if(!timer)
            return;
        if(!head)
        {
            head = tail = timer;
            return;
        }

        if(timer->expire < head->expire)
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        add_timer(timer,head);
    }

    void adjust_timer(util_timer* timer)
    {
        if(!timer)
            return;
        util_timer* tmp = timer->next;
        if(!tmp || (timer->expire < tmp->expire))
            return;
        if(timer == head)
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL; //@ 切断之前的连接
            add_timer(timer,head);
        }
        else
        {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer(timer,timer->next);
        }
    }

    void del_timer(util_timer* timer)
    {
        if(!timer)
            return;

        if((timer == head) && (timer == tail))
        {
            delete timer;
            head = NULL;
            tail = NELL;
            return;
        }

        if(timer == head)
        {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }

        if(timer == tail)
        {
            tail = timer->prev;
            tail->next = NULL;
            delete tail;
            return;
        }

        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }

    void tick()
    {
        if(!head)
            return;
        printf("timer tick\n");
        timer_t cur = time(NULL);
        util_timer* tmp = head;
        while(tmp)
        {
            if(cur < tmp->expire)
                break;
            //@ 任务执行完成后，将其从链表中删除
            tmp->cb_func(timer->user_data);
            head = tmp->next;
            if(head)
                head->prev = NULL;
            delete tmp;
            tmp = head;
        }
    }

private:
    void add_timer(util_timer* timer,util_timer* lst_head)
    {
        util_timer* prev = lst_head;
        util_timer* tmp = prev->next;

        while(tmp)
        {
            if(timer->expire < tmp->expire)
            {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
            }
            prev = tmp;
            tmp = tmp->next;
        }

        if(!tmp)
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
    }
};

#endif //@ TIMERLIST_H
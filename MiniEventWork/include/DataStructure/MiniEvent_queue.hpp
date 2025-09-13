#ifndef _MINIEVENT_QUEUE_HPP_
#define _MINIEVENT_QUEUE_HPP_

/*
 * Tail queue (双向链表) 的宏定义
 */

// 1. 定义链表头结构体
// name: 你为这个头结构体起的名字
// type: 链表节点的类型
#define TAILQ_HEAD(name, type) \
struct name { \
    struct type *tqh_first; \
    struct type **tqh_last; /* 指向最后一个元素的next指针的地址 */\
}

// 2. 定义链表节点入口结构体
#define TAILQ_ENTRY(type) \
struct { \
    struct type *tqe_next; /* 指向下一个元素 */ \
    struct type **tqe_prev; /* 指向前一个元素的next指针的地址 */ \
}

// 3. 初始化链表头
#define TAILQ_INIT(head) do { \
    (head)->tqh_first = NULL; \
    (head)->tqh_last = &(head)->tqh_first;// head.tqh_last 指向 tqh_first，所以 *head.tqh_last 就是 tqh_first\
} while(0)

//4. 尾插
#define TAILQ_INSERT_TAIL(head, elm, field) do { \
    /* 步骤1: 新元素的 next 指针总是 NULL，因为它是最后一个 */ \
    (elm)->field.tqe_next = NULL;\
    /* 步骤2: 新元素的 prev 指针，应该指向原来链表尾部的 next 指针地址 */ \
    (elm)->field.tqe_prev = (head)->tqh_last;\
    /* 步骤3: 让原来链表尾部的 next 指针指向新元素 */    \
    *(head)->tqh_last = (elm);/*等价于 tqh_first = elm */\
    /* 步骤4: 更新链表头的 tqh_last，让它指向新元素的 next 指针地址 */ \
    (head)->tqh_last = &(elm)->field.tqe_next;\
}while(0)

#endif /* _MINIEVENT_QUEUE_HPP_ */
#ifndef _MINI_EVENT_MIN_HEAP_H_
#define _MINI_EVENT_MIN_HEAP_H_

#include <stdlib.h> // for malloc, free, realloc
#include <time.h>   // for time_t, although we'll use a custom time struct later

// 前向声明 event 结构体，因为我们的堆是用来存储 event 指针的。
// 真正的 event 结构体定义会在后面的章节中创建。
struct event {
    // 核心：超时时间。这是最小堆进行比较的依据。
    // 在 libevent 中，这是一个 timeval 结构体。为简化，我们先假设它是个 long。
    long ev_timeout;

    // 核心：记录该 event 在最小堆数组中的索引。
    // 这个成员是解答我们之前“苏格ྲ底式提问”的关键。
    // 它使得任意元素的删除操作可以达到 O(log n) 的效率。
    unsigned int min_heap_idx;
};

// 定义最小堆的结构体
typedef struct min_heap
{
    struct event** p; // p 是一个指针，指向一个存储 event* 的动态数组
    unsigned int n;   // n (number) 是堆中当前存储的元素数量
    unsigned int a;   // a (allocated) 是数组 p 的总容量
} min_heap_t;

// 函数声明区域

// 构造与析构
static inline void min_heap_ctor(min_heap_t* s);
static inline void min_heap_dtor(min_heap_t* s);

// 元素管理
static inline void min_heap_elem_init(struct event* e);
static inline struct event* min_heap_top(const min_heap_t* s);
static inline int min_heap_empty(const min_heap_t* s);

// 核心操作：压入、弹出和删除
static inline int min_heap_push(min_heap_t* s, struct event* e);
static inline struct event* min_heap_pop(min_heap_t* s);
static inline int min_heap_erase(min_heap_t* s, struct event* e);

// --- 内部辅助函数 ---
// 比较函数，决定了这是最小堆还是最大堆
static inline int min_heap_elem_greater(struct event *a, struct event *b)
{
    return a->ev_timeout > b->ev_timeout;
}

// 交换堆中两个元素的位置，并更新它们内部记录的索引
static inline void min_heap_swap(min_heap_t* s, unsigned int i, unsigned int j)
{
    struct event* tmp = s->p[i];
    s->p[i] = s->p[j];
    s->p[j] = tmp;
    // 关键一步：交换元素后，必须更新元素自身存储的索引！
    s->p[i]->min_heap_idx = i;
    s->p[j]->min_heap_idx = j;
}

// 核心算法：上浮
static inline void min_heap_shift_up(min_heap_t* s, unsigned int hole_index, struct event* e)
{
    unsigned int parent = (hole_index - 1) / 2; // 计算父节点索引
    // 循环上浮：当洞(hole)还没到顶(index>0)并且父节点比新元素大时
    while (hole_index && min_heap_elem_greater(s->p[parent], e)) {
        // 将父节点向下移动来填补“洞”
        s->p[hole_index] = s->p[parent];
        s->p[hole_index]->min_heap_idx = hole_index;
        // “洞”的位置移动到父节点处
        hole_index = parent;
        // 计算新的父节点
        parent = (hole_index - 1) / 2;
    }
    // 将新元素放入最终的“洞”中
    s->p[hole_index] = e;
    e->min_heap_idx = hole_index;
}

// 核心算法：下沉
static inline void min_heap_shift_down(min_heap_t* s, unsigned int hole_index, struct event* e)
{
    unsigned int min_child = 2 * hole_index + 1; // 计算左子节点索引
    while (min_child < s->n) { // 确保子节点存在
        // 找出左右子节点中较小的那个
        // min_child + 1 是右子节点，必须确保它存在
        if (min_child + 1 < s->n &&
            min_heap_elem_greater(s->p[min_child], s->p[min_child + 1])) {
            min_child++; // 右子节点更小
        }

        // 如果要放入的元素 e 已经比最小的子节点还小，则找到了正确位置，下沉结束
        if (!min_heap_elem_greater(e, s->p[min_child])) {
            break;
        }

        // 否则，将最小的子节点向上移动，填补“洞”
        s->p[hole_index] = s->p[min_child];
        s->p[hole_index]->min_heap_idx = hole_index;
        // “洞”下沉到最小子节点的位置
        hole_index = min_child;
        // 计算新的左子节点
        min_child = 2 * hole_index + 1;
    }
    // 将元素 e 放入最终的“洞”中
    s->p[hole_index] = e;
    e->min_heap_idx = hole_index;
}

// 确保堆有足够的容量
static inline int min_heap_reserve(min_heap_t* s, unsigned int n)
{
    if (s->a < n) {
        struct event** p;
        // 容量按2倍增长，是一种常见的策略，避免频繁的内存分配
        unsigned int a = s->a ? s->a * 2 : 8;
        if (a < n)
            a = n;
        // 重新分配内存
        p = (struct event**)realloc(s->p, a * sizeof(*p));
        if (!p)
            return -1;
        s->p = p;
        s->a = a;
    }
    return 0;
}

// --- 函数实现区域 ---

// 构造函数：初始化一个空的最小堆
static inline void min_heap_ctor(min_heap_t* s)
{
    s->p = NULL;
    s->n = 0;
    s->a = 0;
}

// 析构函数：释放堆所占用的内存
static inline void min_heap_dtor(min_heap_t* s)
{
    if (s->p)
        free(s->p);
}

// 当一个 event 被创建时，用这个函数初始化它的堆相关成员
static inline void min_heap_elem_init(struct event* e)
{
    e->min_heap_idx = -1; // -1 是一个哨兵值，表示它不在任何堆中
}

// 查看堆顶元素（最小的元素），但不移除它
static inline struct event* min_heap_top(const min_heap_t* s)
{
    return s->n ? *s->p : NULL;
}

// 判断堆是否为空
static inline int min_heap_empty(const min_heap_t* s)
{
    return 0 == s->n;
}

// 将一个元素压入堆
static inline int min_heap_push(min_heap_t* s, struct event* e)
{
    // 检查容量，不够则扩容
    if (min_heap_reserve(s, s->n + 1))
        return -1;
    // 调用“上浮”算法，将新元素放到正确的位置
    min_heap_shift_up(s, s->n++, e);
    return 0;
}

// 弹出堆顶（最小）的元素
static inline struct event* min_heap_pop(min_heap_t* s)
{
    if (s->n) {
        // 取出堆顶元素作为返回值
        struct event* e = *s->p;
        // 将最后一个元素挪到堆顶，然后执行“下沉”操作来维持堆的性质
        min_heap_shift_down(s, 0, s->p[--s->n]);
        e->min_heap_idx = -1; // 标记弹出的元素已不在堆中
        return e;
    }
    return NULL;
}

// 从堆中移除任意一个指定的元素
static inline int min_heap_erase(min_heap_t* s, struct event* e)
{
    // 如果元素的索引无效，或者堆中对应索引的元素不是它自己，说明有错误
    if (((unsigned int)-1) == e->min_heap_idx || e->min_heap_idx >= s->n || s->p[e->min_heap_idx] != e)
        return -1;

    // 获取要删除元素的当前索引
    unsigned int hole_index = e->min_heap_idx;
    // 取出堆中最后一个元素
    struct event* last = s->p[--s->n];

    // 如果要删除的恰好就是最后一个元素，那就什么都不用做了
    if (hole_index != s->n) {
        // 否则，用最后一个元素 last 来填补被删除元素留下的“洞”
        // 然后需要判断 last 应该是“上浮”还是“下沉”
        // 如果 last 比它在洞口位置的父节点小，则需要上浮
        int parent = (hole_index - 1) / 2;
        if (hole_index > 0 && min_heap_elem_greater(s->p[parent], last))
            min_heap_shift_up(s, hole_index, last);
        else // 否则，就下沉
            min_heap_shift_down(s, hole_index, last);
    }

    e->min_heap_idx = -1; // 标记被删除的元素已不在堆中
    return 0;
}

#endif /* _MINI_EVENT_MIN_HEAP_H_ */

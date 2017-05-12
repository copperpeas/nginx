
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;
    void                 *data;
    ngx_pool_cleanup_t   *next;
};

//phoenix add  Nginx里内存的使用大都十分有特色:申请了永久保存: 抑或伴随着请求的结束而全部释放,还有写满了缓冲再从头接着写.
// 这么做的原因也主要取决于Web Server的特殊的场景内存的分配和请求相关：一条请求处理完毕,即可释放其相关的内存池,降低了开发中对内存资源管理的复杂度,
//也减少了内存碎片的存在。所以，Nginx使用内存池时总是只申请,不释放,使用完毕后直接destroy整个内存池.我们来看下内存池相关的实现。
//ngx_poll_large_s,ngx_pool_data_t, ngx_pool_s，这三个结构构成了基本的内存池的主体，并通过ngx_create_pool可以创建一个内存池，通过ngx_palloc可以从内存池中分配指定大小的内存。
//-----------------nginx 内存分配相关数据结构 start-----------------------------------//
typedef struct ngx_pool_large_s  ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;
    void                 *alloc;
};

//phoenix add 
typedef struct {
    u_char               *last;
    u_char               *end;
    ngx_pool_t           *next;
    ngx_uint_t            failed;
} ngx_pool_data_t;

/*phoenix add
* ngx_pool_s是ngx的内存池，每个工作线程都会持有一个，我们来看它的结构如下struct ngx_pool_s{...}
*
*/
struct ngx_pool_s {
    ngx_pool_data_t       d;    //*last ,*end, *next, failed.  //phoenix add 数据块
    size_t                max;     //phoenix add 小块内存的最大值
    ngx_pool_t           *current; //指向当前内存池
    ngx_chain_t          *chain;
    ngx_pool_large_t     *large;  //*next, *alloc  //phoenix add 指向大块内存用，即超过max的内存请求。
    ngx_pool_cleanup_t   *cleanup; //phoenix add 挂载一些内存池释放的时候，同时释放的资源。
    ngx_log_t            *log;
};
//-----------------nginx 内存分配相关数据结构 end-----------------------------------//

typedef struct {
    ngx_fd_t              fd;
    u_char               *name;
    ngx_log_t            *log;
} ngx_pool_cleanup_file_t;


void *ngx_alloc(size_t size, ngx_log_t *log);
void *ngx_calloc(size_t size, ngx_log_t *log);

//phoenix add # ngx_create_pool用于
ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */

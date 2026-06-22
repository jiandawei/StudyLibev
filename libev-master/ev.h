/*
 * libev native API header
 *
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2015 Marc Alexander Lehmann <libev@schmorp.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modifica-
 * tion, are permitted provided that the following conditions are met:
 *
 *   1.  Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *   2.  Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPE-
 * CIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTH-
 * ERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License ("GPL") version 2 or any later version,
 * in which case the provisions of the GPL are applicable instead of
 * the above. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL. If you do not delete the
 * provisions above, a recipient may use your version of this file under
 * either the BSD or the GPL.
 */

#ifndef EV_H_
#define EV_H_

#ifdef __cplusplus
# define EV_CPP(x) x
# if __cplusplus >= 201103L
#  define EV_THROW noexcept
# else
#  define EV_THROW throw ()
# endif
#else
# define EV_CPP(x)
# define EV_THROW
#endif

EV_CPP(extern "C" {)

/*****************************************************************************/

/* pre-4.0 compatibility */
#ifndef EV_COMPAT3
# define EV_COMPAT3 1
#endif

/* 库的特性开关 */
#ifndef EV_FEATURES
# if defined __OPTIMIZE_SIZE__
#  define EV_FEATURES 0x7c /*关掉了EV_FEATURE_CODE和EV_FEATURE_DATA，其余全开*/
# else
#  define EV_FEATURES 0x7f /*feature全开*/
# endif
#endif

/**
 * 库的特性开关位
 * 特性宏	               十进制	 功能说明	     影响范围
 * EV_FEATURE_CODE	  	1	     代码相关特性	  代码生成、内联优化
 * EV_FEATURE_DATA	  	2	     数据相关特性	  数据结构、内存布局
 * EV_FEATURE_CONFIG		4	     配置相关特性	  多循环、优先级系统
 * EV_FEATURE_API	    	8	     API 相关特性	 高级 API、统计信息
 * EV_FEATURE_WATCHERS  16	   Watcher 特性  各类事件监听器
 * EV_FEATURE_BACKENDS	32	   后端特性	     多种 IO 多路复用后端
 * EV_FEATURE_OS	      64	   操作系统特性	  OS 系统调用、平台特性
*/
#define EV_FEATURE_CODE     ((EV_FEATURES) &  1)
#define EV_FEATURE_DATA     ((EV_FEATURES) &  2)
#define EV_FEATURE_CONFIG   ((EV_FEATURES) &  4)
#define EV_FEATURE_API      ((EV_FEATURES) &  8)
#define EV_FEATURE_WATCHERS ((EV_FEATURES) & 16)
#define EV_FEATURE_BACKENDS ((EV_FEATURES) & 32)
#define EV_FEATURE_OS       ((EV_FEATURES) & 64)

/* these priorities are inclusive, higher priorities will be invoked earlier */
/* 优先级：优先级高的优先被调度 */
#ifndef EV_MINPRI
# define EV_MINPRI (EV_FEATURE_CONFIG ? -2 : 0)
#endif
#ifndef EV_MAXPRI
# define EV_MAXPRI (EV_FEATURE_CONFIG ? +2 : 0)
#endif

/* 多事件循环 */
#ifndef EV_MULTIPLICITY
# define EV_MULTIPLICITY EV_FEATURE_CONFIG
#endif

/* 下面的都是watcher相关的使能宏 */
#ifndef EV_PERIODIC_ENABLE
# define EV_PERIODIC_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_STAT_ENABLE
# define EV_STAT_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_PREPARE_ENABLE
# define EV_PREPARE_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_CHECK_ENABLE
# define EV_CHECK_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_IDLE_ENABLE
# define EV_IDLE_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_FORK_ENABLE
# define EV_FORK_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_CLEANUP_ENABLE
# define EV_CLEANUP_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_SIGNAL_ENABLE
# define EV_SIGNAL_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_CHILD_ENABLE
# ifdef _WIN32
#  define EV_CHILD_ENABLE 0
# else
#  define EV_CHILD_ENABLE EV_FEATURE_WATCHERS
#endif
#endif

#ifndef EV_ASYNC_ENABLE
# define EV_ASYNC_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_EMBED_ENABLE
# define EV_EMBED_ENABLE EV_FEATURE_WATCHERS
#endif

#ifndef EV_WALK_ENABLE
# define EV_WALK_ENABLE 0 /* not yet */
#endif

/*****************************************************************************/

#if EV_CHILD_ENABLE && !EV_SIGNAL_ENABLE
# undef EV_SIGNAL_ENABLE
# define EV_SIGNAL_ENABLE 1
#endif

/*****************************************************************************/

// 时间戳类型定义
typedef double ev_tstamp;

#include <string.h> /* for memmove */

#ifndef EV_ATOMIC_T
# include <signal.h>
# define EV_ATOMIC_T sig_atomic_t volatile
#endif

#if EV_STAT_ENABLE
# ifdef _WIN32
#  include <time.h>
#  include <sys/types.h>
# endif
# include <sys/stat.h>
#endif

/* support multiple event loops? */
/**
 * 支持多个event loop，通常用于函数调用时的第一个参数
 * 有下划线后缀代表加上逗号
 * 
 * 宏命名	含义	用途
 * EV_P	Parameter	函数声明中的参数
 * EV_A	Argument	函数调用中的参数
 * _ 后缀	逗号分隔	用于多参数场景
*/
#if EV_MULTIPLICITY
struct ev_loop;
# define EV_P  struct ev_loop *loop               /* a loop as sole parameter in a declaration */
# define EV_P_ EV_P,                              /* a loop as first of multiple parameters */
# define EV_A  loop                               /* a loop as sole argument to a function call */
# define EV_A_ EV_A,                              /* a loop as first of multiple arguments */
# define EV_DEFAULT_UC  ev_default_loop_uc_ ()    /* the default loop, if initialised, as sole arg */
# define EV_DEFAULT_UC_ EV_DEFAULT_UC,            /* the default loop as first of multiple arguments */
# define EV_DEFAULT  ev_default_loop (0)          /* the default loop as sole arg */
# define EV_DEFAULT_ EV_DEFAULT,                  /* the default loop as first of multiple arguments */
#else
# define EV_P void
# define EV_P_
# define EV_A
# define EV_A_
# define EV_DEFAULT
# define EV_DEFAULT_
# define EV_DEFAULT_UC
# define EV_DEFAULT_UC_
# undef EV_EMBED_ENABLE
#endif

/**
 * 函数内联宏
*/
/* EV_INLINE is used for functions in header files */
#if __STDC_VERSION__ >= 199901L || __GNUC__ >= 3
# define EV_INLINE static inline
#else
# define EV_INLINE static
#endif

/**
 * 函数声明前的宏
*/
#ifdef EV_API_STATIC
# define EV_API_DECL static
#else
# define EV_API_DECL extern
#endif

/* EV_PROTOTYPES can be used to switch of prototype declarations */
/**
 * 用于控制函数原型声明的宏，它允许用户选择是否需要完整的函数原型声明
 * 1：头文件有函数原型声明，要用时直接包含头文件使用
 * 0：头文件没有函数原型声明，要用时要自己声明函数
 * 特性	      EV_PROTOTYPES = 1	       EV_PROTOTYPES = 0
 * 开发便利性 	✅ 便于使用，无需手动声明	❌ 需要手动声明函数
 * 编译速度  	 ⚠️ 需要处理更多声明	      ✅ 编译更快
 * 控制粒度	   ❌ 全部或全无	            ✅ 精确控制需要的声明
 * 重复声明警告	⚠️ 可能产生警告	          ✅ 避免警告
 * 文档完整性	 ✅ 函数接口完整可见	       ❌ 需要查看其他文件
 * 内存占用	   ⚠️ 符号表稍大	           ✅ 符号表更小
*/
#ifndef EV_PROTOTYPES
# define EV_PROTOTYPES 1
#endif

/*****************************************************************************/
/* 库版本号 */
#define EV_VERSION_MAJOR 4
#define EV_VERSION_MINOR 22

/* event类别，用于eventmask, revents, events...，通常用于按位或来添加event */
/* eventmask, revents, events... */
enum {
  EV_UNDEF    = (int)0xFFFFFFFF, /* 无效值 */ /* guaranteed to be invalid */
  EV_NONE     =            0x00, /* 无事件 */ /* no events */
  EV_READ     =            0x01, /* IO 读就绪 */ /* ev_io detected read will not block */
  EV_WRITE    =            0x02, /* IO 写就绪 */ /* ev_io detected write will not block */
  EV__IOFDSET =            0x80, /* 内部标记 */ /* internal use only */
  EV_IO       =         EV_READ, /* IO 类型检测别名 */ /* alias for type-detection */
  EV_TIMER    =      0x00000100, /* 定时器超时 */ /* timer timed out */
#if EV_COMPAT3
  EV_TIMEOUT  =        EV_TIMER, /* pre 4.0 API compatibility */
#endif
  EV_PERIODIC =      0x00000200, /* 周期性定时器 */ /* periodic timer timed out */
  EV_SIGNAL   =      0x00000400, /* 信号到达 */ /* signal was received */
  EV_CHILD    =      0x00000800, /* 子进程状态变化 */ /* child/pid had status change */
  EV_STAT     =      0x00001000, /* 文件状态变化 */ /* stat data changed */
  EV_IDLE     =      0x00002000, /* 事件循环空闲 */ /* event loop is idling */
  EV_PREPARE  =      0x00004000, /* 即将进入 IO 等待 */ /* event loop about to poll */
  EV_CHECK    =      0x00008000, /* 刚完成 IO 等待 */ /* event loop finished poll */
  EV_EMBED    =      0x00010000, /* 嵌入循环需要扫描 */ /* embedded event loop needs sweep */
  EV_FORK     =      0x00020000, /* fork 后子进程恢复 */ /* event loop resumed in child */
  EV_CLEANUP  =      0x00040000, /* 事件循环销毁 */ /* event loop resumed in child */
  EV_ASYNC    =      0x00080000, /* 跨线程异步信号 */ /* async intra-loop signal */
  EV_CUSTOM   =      0x01000000, /* 用户自定义事件 */ /* for use by user code */
  EV_ERROR    = (int)0x80000000  /* 错误发生 */ /* sent when an error occurs */
};

/* can be used to add custom fields to all watchers, while losing binary compatibility */
/* 自定义数据 void* */
#ifndef EV_COMMON
# define EV_COMMON void *data;
#endif

/**
 * watcher callback类型定义
*/
#ifndef EV_CB_DECLARE
# define EV_CB_DECLARE(type) void (*cb)(EV_P_ struct type *w, int revents);
#endif

/**
 * 调用watcher callback
*/
#ifndef EV_CB_INVOKE
# define EV_CB_INVOKE(watcher,revents) (watcher)->cb (EV_A_ (watcher), (revents))
#endif

/*
 * struct member types:
 * private: you may look at them, but not change them,
 *          and they might not mean anything to you.
 * ro: can be read anytime, but only changed when the watcher isn't active.
 * rw: can be read and modified anytime, even when the watcher is active.
 *
 * some internal details that might be helpful for debugging:
 *
 * active is either 0, which means the watcher is not active,
 *           or the array index of the watcher (periodics, timers)
 *           or the array index + 1 (most other watchers)
 *           or simply 1 for watchers that aren't in some array.
 * pending is either 0, in which case the watcher isn't,
 *           or the array index + 1 in the pendings array.
 */

#if EV_MINPRI == EV_MAXPRI
# define EV_DECL_PRIORITY
#elif !defined (EV_DECL_PRIORITY)
# define EV_DECL_PRIORITY int priority;
#endif

/* shared by all watchers */
/* 所有watcher都拥有的成员 */
#define EV_WATCHER(type)			\
  int active; /* private */ /* 是否正在监听 */ \
  int pending; /* private */ /* 是否在pending队列等待调度 */ \
  EV_DECL_PRIORITY /* private */ /* watcher的优先级 */ \
  EV_COMMON /* rw */ /* 存放自定义数据 */ \
  EV_CB_DECLARE (type) /* event 发生时的 callback */ /* private */ 

/* 所有watcher链表节点都拥有的成员 */
#define EV_WATCHER_LIST(type)			\
  EV_WATCHER (type)				\
  struct ev_watcher_list *next; /* private */

/* 带时间戳的watcher */
#define EV_WATCHER_TIME(type)			\
  EV_WATCHER (type)				\
  ev_tstamp at;     /* private */

/* base class, nothing to see here unless you subclass */
typedef struct ev_watcher
{
  EV_WATCHER (ev_watcher)
} ev_watcher;

/* base class, nothing to see here unless you subclass */
typedef struct ev_watcher_list
{
  EV_WATCHER_LIST (ev_watcher_list)
} ev_watcher_list;

/* base class, nothing to see here unless you subclass */
typedef struct ev_watcher_time
{
  EV_WATCHER_TIME (ev_watcher_time)
} ev_watcher_time;

/**
 * 以下为库用户常用的数据结构
*/
/* invoked when fd is either EV_READable or EV_WRITEable */
/* revent EV_READ, EV_WRITE */
/* I/O事件watcher */
typedef struct ev_io
{
  EV_WATCHER_LIST (ev_io)

  int fd;     /* ro */ // 监听的文件描述符
  int events; /* ro */ // 监听的事件类型（EV_READ, EV_WRITE）
} ev_io;

/* invoked after a specific time, repeatable (based on monotonic clock) */
/* revent EV_TIMEOUT */
/* 相对定时器（一段时间后超时） */
typedef struct ev_timer
{
  EV_WATCHER_TIME (ev_timer)

  ev_tstamp repeat; /* rw */
} ev_timer;

/* invoked at some specific time, possibly repeating at regular intervals (based on UTC) */
/* revent EV_PERIODIC */
/* 绝对定时器（某个时间点超时） */
/*
模式	           设置方式	                        行为
绝对时间	        ev_periodic_set(&w, at, 0, 0)	 只在 at 时间点触发一次
offset/interval	ev_periodic_set(&w, 0, 3600, 0)	在 offset 之后，每 interval 秒触发一次（如每小时整点）
reschedule_cb	  ev_periodic_set(&w, 0, 0, cb)	  每次触发后调用 cb，由回调决定下次时间
*/
typedef struct ev_periodic
{
  EV_WATCHER_TIME (ev_periodic)

  ev_tstamp offset; /* rw */
  ev_tstamp interval; /* rw */
  ev_tstamp (*reschedule_cb)(struct ev_periodic *w, ev_tstamp now) EV_THROW; /* rw */ // reschedule_cb 是 ev_periodic 三种调度模式之一，用于自定义下次触发时间
} ev_periodic;

/* invoked when the given signal has been received */
/* revent EV_SIGNAL */
/* signal watcher */
typedef struct ev_signal
{
  EV_WATCHER_LIST (ev_signal)

  int signum; /* ro */
} ev_signal;

/* invoked when sigchld is received and waitpid indicates the given pid */
/* revent EV_CHILD */
/* does not support priorities */
/* 子进程状态变化 */
typedef struct ev_child
{
  EV_WATCHER_LIST (ev_child)

  int flags;   /* private */
  int pid;     /* ro */
  int rpid;    /* rw, holds the received pid */
  int rstatus; /* rw, holds the exit status, use the macros from sys/wait.h */
} ev_child;

#if EV_STAT_ENABLE
/* st_nlink = 0 means missing file or other error */
# ifdef _WIN32
typedef struct _stati64 ev_statdata;
# else
typedef struct stat ev_statdata;
# endif

/* invoked each time the stat data changes for a given path */
/* revent EV_STAT */
/* 文件状态变化 */
typedef struct ev_stat
{
  EV_WATCHER_LIST (ev_stat)

  ev_timer timer;     /* private */ /* 定时器，用于定时检测文件stat */
  ev_tstamp interval; /* ro */      /* 定时器间隔，每隔interval就检查 */
  const char *path;   /* ro */      /* 文件路径 */
  ev_statdata prev;   /* ro */      /* 文件属性 old */
  ev_statdata attr;   /* ro */      /* 文件属性 new */

  int wd; /* wd for inotify, fd for kqueue */
} ev_stat;
#endif

#if EV_IDLE_ENABLE
/* invoked when the nothing else needs to be done, keeps the process from blocking */
/* revent EV_IDLE */
/* event loop空闲触发事件 */
typedef struct ev_idle
{
  EV_WATCHER (ev_idle)
} ev_idle;
#endif

/* invoked for each run of the mainloop, just before the blocking call */
/* you can still change events in any way you like */
/* revent EV_PREPARE */
/* 每次loop之前需要执行的 */
typedef struct ev_prepare
{
  EV_WATCHER (ev_prepare)
} ev_prepare;

/* invoked for each run of the mainloop, just after the blocking call */
/* revent EV_CHECK */
/* event loop之后需要执行的watcher？？？ */
typedef struct ev_check
{
  EV_WATCHER (ev_check)
} ev_check;

#if EV_FORK_ENABLE
/* the callback gets invoked before check in the child process when a fork was detected */
/* revent EV_FORK */
/* fork事件 */
typedef struct ev_fork
{
  EV_WATCHER (ev_fork)
} ev_fork;
#endif

#if EV_CLEANUP_ENABLE
/* is invoked just before the loop gets destroyed */
/* revent EV_CLEANUP */
/* event loop退出触发事件 */
typedef struct ev_cleanup
{
  EV_WATCHER (ev_cleanup)
} ev_cleanup;
#endif

#if EV_EMBED_ENABLE
/* used to embed an event loop inside another */
/* the callback gets invoked when the event loop has handled events, and can be 0 */
typedef struct ev_embed
{
  EV_WATCHER (ev_embed)

  struct ev_loop *other; /* ro */
  ev_io io;              /* private */
  ev_prepare prepare;    /* private */
  ev_check check;        /* unused */
  ev_timer timer;        /* unused */
  ev_periodic periodic;  /* unused */
  ev_idle idle;          /* unused */
  ev_fork fork;          /* private */
#if EV_CLEANUP_ENABLE
  ev_cleanup cleanup;    /* unused */
#endif
} ev_embed;
#endif

#if EV_ASYNC_ENABLE
/* invoked when somebody calls ev_async_send on the watcher */
/* revent EV_ASYNC */
/* 线程间异步事件 */
typedef struct ev_async
{
  EV_WATCHER (ev_async)

  EV_ATOMIC_T sent; /* private */
} ev_async;

# define ev_async_pending(w) (+(w)->sent)
#endif

/* the presence of this union forces similar struct layout */
/* 所有watcher的union */
union ev_any_watcher
{
  struct ev_watcher w;
  struct ev_watcher_list wl;

  struct ev_io io;
  struct ev_timer timer;
  struct ev_periodic periodic;
  struct ev_signal signal;
  struct ev_child child;
#if EV_STAT_ENABLE
  struct ev_stat stat;
#endif
#if EV_IDLE_ENABLE
  struct ev_idle idle;
#endif
  struct ev_prepare prepare;
  struct ev_check check;
#if EV_FORK_ENABLE
  struct ev_fork fork;
#endif
#if EV_CLEANUP_ENABLE
  struct ev_cleanup cleanup;
#endif
#if EV_EMBED_ENABLE
  struct ev_embed embed;
#endif
#if EV_ASYNC_ENABLE
  struct ev_async async;
#endif
};

/* flag bits for ev_default_loop and ev_loop_new */
enum {
  /* the default */
  EVFLAG_AUTO      = 0x00000000U, /* not quite a mask */
  /* flag bits */
  EVFLAG_NOENV     = 0x01000000U, /* do NOT consult environment */
  EVFLAG_FORKCHECK = 0x02000000U, /* check for a fork in each iteration */
  /* debugging/feature disable */
  EVFLAG_NOINOTIFY = 0x00100000U, /* do not attempt to use inotify */
#if EV_COMPAT3
  EVFLAG_NOSIGFD   = 0, /* compatibility to pre-3.9 */
#endif
  EVFLAG_SIGNALFD  = 0x00200000U, /* attempt to use signalfd */
  EVFLAG_NOSIGMASK = 0x00400000U  /* avoid modifying the signal mask */
};

/* method bits to be ored together */
enum {
  EVBACKEND_SELECT  = 0x00000001U, /* about anywhere */
  EVBACKEND_POLL    = 0x00000002U, /* !win */
  EVBACKEND_EPOLL   = 0x00000004U, /* linux */
  EVBACKEND_KQUEUE  = 0x00000008U, /* bsd */
  EVBACKEND_DEVPOLL = 0x00000010U, /* solaris 8 */ /* NYI */
  EVBACKEND_PORT    = 0x00000020U, /* solaris 10 */
  EVBACKEND_ALL     = 0x0000003FU, /* all known backends */
  EVBACKEND_MASK    = 0x0000FFFFU  /* all future backends */
};

#if EV_PROTOTYPES
/* 库版本号 */
EV_API_DECL int ev_version_major (void) EV_THROW;
EV_API_DECL int ev_version_minor (void) EV_THROW;

/* 后端 */
EV_API_DECL unsigned int ev_supported_backends (void) EV_THROW;
EV_API_DECL unsigned int ev_recommended_backends (void) EV_THROW;
EV_API_DECL unsigned int ev_embeddable_backends (void) EV_THROW;

/* 时间戳 */
EV_API_DECL ev_tstamp ev_time (void) EV_THROW;
/* sleep */
EV_API_DECL void ev_sleep (ev_tstamp delay) EV_THROW; /* sleep for a while */

/* Sets the allocation function to use, works like realloc.
 * It is used to allocate and free memory.
 * If it returns zero when memory needs to be allocated, the library might abort
 * or take some potentially destructive action.
 * The default is your system realloc function.
 */
EV_API_DECL void ev_set_allocator (void *(*cb)(void *ptr, long size) EV_THROW) EV_THROW;

/* set the callback function to call on a
 * retryable syscall error
 * (such as failed select, poll, epoll_wait)
 */
EV_API_DECL void ev_set_syserr_cb (void (*cb)(const char *msg) EV_THROW) EV_THROW;

#if EV_MULTIPLICITY
/**
 * 多event loop模式下支持的函数
*/

/* the default loop is the only one that handles signals and child watchers */
/* you can call this as often as you like */
/* 返回default loop的指针（如果未初始化，就会初始化后再返回） */
EV_API_DECL struct ev_loop *ev_default_loop (unsigned int flags EV_CPP (= 0)) EV_THROW;

#ifdef EV_API_STATIC
EV_API_DECL struct ev_loop *ev_default_loop_ptr;
#endif

/* 返回default loop的指针（uc： Uninitialized Check） */
EV_INLINE struct ev_loop *
ev_default_loop_uc_ (void) EV_THROW
{
  extern struct ev_loop *ev_default_loop_ptr;

  return ev_default_loop_ptr;
}

/* loop是否为default loop */
EV_INLINE int
ev_is_default_loop (EV_P) EV_THROW
{
  return EV_A == EV_DEFAULT_UC;
}

/* create and destroy alternative loops that don't handle signals */
/* 创建loop */
EV_API_DECL struct ev_loop *ev_loop_new (unsigned int flags EV_CPP (= 0)) EV_THROW;

/* loop相关的时间戳 */
EV_API_DECL ev_tstamp ev_now (EV_P) EV_THROW; /* time w.r.t. timers and the eventloop, updated after each poll */

#else
/**
 * 单event loop模式下支持的函数
*/

EV_API_DECL int ev_default_loop (unsigned int flags EV_CPP (= 0)) EV_THROW; /* returns true when successful */

EV_API_DECL ev_tstamp ev_rt_now;

EV_INLINE ev_tstamp
ev_now (void) EV_THROW
{
  return ev_rt_now;
}

/* looks weird, but ev_is_default_loop (EV_A) still works if this exists */
EV_INLINE int
ev_is_default_loop (void) EV_THROW
{
  return 1;
}

#endif /* multiplicity */

/* destroy event loops, also works for the default loop */
/* 销毁event loop */
EV_API_DECL void ev_loop_destroy (EV_P);

/* this needs to be called after fork, to duplicate the loop */
/* when you want to re-use it in the child */
/* you can call it in either the parent or the child */
/* you can actually call it at any time, anywhere :) */
/* 在fork后，如果子进程也要使用loop前，需要调用该函数 */
EV_API_DECL void ev_loop_fork (EV_P) EV_THROW;

/* 返回loop的backend */
EV_API_DECL unsigned int ev_backend (EV_P) EV_THROW; /* backend in use by loop */

/* 更新loop的now时间（mn_now） */
EV_API_DECL void ev_now_update (EV_P) EV_THROW; /* update event loop time */

#if EV_WALK_ENABLE
/* walk (almost) all watchers in the loop of a given type, invoking the */
/* callback on every such watcher. The callback might stop the watcher, */
/* but do nothing else with the loop */
EV_API_DECL void ev_walk (EV_P_ int types, void (*cb)(EV_P_ int type, void *w)) EV_THROW;
#endif

#endif /* prototypes */

/* ev_run flags values */
enum {
  EVRUN_NOWAIT = 1, /* do not block/wait */
  EVRUN_ONCE   = 2  /* block *once* only */
};

/* ev_break how values */
enum {
  EVBREAK_CANCEL = 0, /* undo unloop 撤销之前的退出请求 */
  EVBREAK_ONE    = 1, /* unloop once 退出最近的（最内层）事件循环 */
  EVBREAK_ALL    = 2  /* unloop all loops 完全停止事件处理，退出所有层级的循环 */
};

#if EV_PROTOTYPES
EV_API_DECL int  ev_run (EV_P_ int flags EV_CPP (= 0));
EV_API_DECL void ev_break (EV_P_ int how EV_CPP (= EVBREAK_ONE)) EV_THROW; /* break out of the loop */

/*
 * ref/unref can be used to add or remove a refcount on the mainloop. every watcher
 * keeps one reference. if you have a long-running watcher you never unregister that
 * should not keep ev_loop from running, unref() after starting, and ref() before stopping.
 */
EV_API_DECL void ev_ref   (EV_P) EV_THROW;
EV_API_DECL void ev_unref (EV_P) EV_THROW;

/*
 * convenience function, wait for a single event, without registering an event watcher
 * if timeout is < 0, do wait indefinitely
 */
EV_API_DECL void ev_once (EV_P_ int fd, int events, ev_tstamp timeout, void (*cb)(int revents, void *arg), void *arg) EV_THROW;

# if EV_FEATURE_API
EV_API_DECL unsigned int ev_iteration (EV_P) EV_THROW; /* number of loop iterations */
EV_API_DECL unsigned int ev_depth     (EV_P) EV_THROW; /* #ev_loop enters - #ev_loop leaves */
EV_API_DECL void         ev_verify    (EV_P) EV_THROW; /* abort if loop data corrupted */

EV_API_DECL void ev_set_io_collect_interval (EV_P_ ev_tstamp interval) EV_THROW; /* sleep at least this time, default 0 */
EV_API_DECL void ev_set_timeout_collect_interval (EV_P_ ev_tstamp interval) EV_THROW; /* sleep at least this time, default 0 */

/* advanced stuff for threading etc. support, see docs */
EV_API_DECL void ev_set_userdata (EV_P_ void *data) EV_THROW;
EV_API_DECL void *ev_userdata (EV_P) EV_THROW;
typedef void (*ev_loop_callback)(EV_P);
EV_API_DECL void ev_set_invoke_pending_cb (EV_P_ ev_loop_callback invoke_pending_cb) EV_THROW;
/* C++ doesn't allow the use of the ev_loop_callback typedef here, so we need to spell it out */
EV_API_DECL void ev_set_loop_release_cb (EV_P_ void (*release)(EV_P) EV_THROW, void (*acquire)(EV_P) EV_THROW) EV_THROW;

EV_API_DECL unsigned int ev_pending_count (EV_P) EV_THROW; /* number of pending events, if any */
EV_API_DECL void ev_invoke_pending (EV_P); /* invoke all pending watchers */

/*
 * stop/start the timer handling.
 */
EV_API_DECL void ev_suspend (EV_P) EV_THROW;
EV_API_DECL void ev_resume  (EV_P) EV_THROW;
#endif

#endif

/* these may evaluate ev multiple times, and the other arguments at most once */
/* either use ev_init + ev_TYPE_set, or the ev_TYPE_init macro, below, to first initialise a watcher */
/**
 * 设置active、pending、priority和callback
*/
#define ev_init(ev,cb_) do {			\
  ((ev_watcher *)(void *)(ev))->active  =	\
  ((ev_watcher *)(void *)(ev))->pending = 0;	\
  ev_set_priority ((ev), 0);			\
  ev_set_cb ((ev), cb_);			\
} while (0)

/**
 * 以下是各种watcher的set函数
 */
#define ev_io_set(ev,fd_,events_)            do { (ev)->fd = (fd_); (ev)->events = (events_) | EV__IOFDSET; } while (0)
#define ev_timer_set(ev,after_,repeat_)      do { ((ev_watcher_time *)(ev))->at = (after_); (ev)->repeat = (repeat_); } while (0)
#define ev_periodic_set(ev,ofs_,ival_,rcb_)  do { (ev)->offset = (ofs_); (ev)->interval = (ival_); (ev)->reschedule_cb = (rcb_); } while (0)
#define ev_signal_set(ev,signum_)            do { (ev)->signum = (signum_); } while (0)
#define ev_child_set(ev,pid_,trace_)         do { (ev)->pid = (pid_); (ev)->flags = !!(trace_); } while (0)
#define ev_stat_set(ev,path_,interval_)      do { (ev)->path = (path_); (ev)->interval = (interval_); (ev)->wd = -2; } while (0)
#define ev_idle_set(ev)                      /* nop, yes, this is a serious in-joke */
#define ev_prepare_set(ev)                   /* nop, yes, this is a serious in-joke */
#define ev_check_set(ev)                     /* nop, yes, this is a serious in-joke */
#define ev_embed_set(ev,other_)              do { (ev)->other = (other_); } while (0)
#define ev_fork_set(ev)                      /* nop, yes, this is a serious in-joke */
#define ev_cleanup_set(ev)                   /* nop, yes, this is a serious in-joke */
#define ev_async_set(ev)                     /* nop, yes, this is a serious in-joke */

/**
 * 以下是各种watcher的init函数（调用ev_init和对应的set函数）
 */
#define ev_io_init(ev,cb,fd,events)          do { ev_init ((ev), (cb)); ev_io_set ((ev),(fd),(events)); } while (0)
#define ev_timer_init(ev,cb,after,repeat)    do { ev_init ((ev), (cb)); ev_timer_set ((ev),(after),(repeat)); } while (0)
#define ev_periodic_init(ev,cb,ofs,ival,rcb) do { ev_init ((ev), (cb)); ev_periodic_set ((ev),(ofs),(ival),(rcb)); } while (0)
#define ev_signal_init(ev,cb,signum)         do { ev_init ((ev), (cb)); ev_signal_set ((ev), (signum)); } while (0)
#define ev_child_init(ev,cb,pid,trace)       do { ev_init ((ev), (cb)); ev_child_set ((ev),(pid),(trace)); } while (0)
#define ev_stat_init(ev,cb,path,interval)    do { ev_init ((ev), (cb)); ev_stat_set ((ev),(path),(interval)); } while (0)
#define ev_idle_init(ev,cb)                  do { ev_init ((ev), (cb)); ev_idle_set ((ev)); } while (0)
#define ev_prepare_init(ev,cb)               do { ev_init ((ev), (cb)); ev_prepare_set ((ev)); } while (0)
#define ev_check_init(ev,cb)                 do { ev_init ((ev), (cb)); ev_check_set ((ev)); } while (0)
#define ev_embed_init(ev,cb,other)           do { ev_init ((ev), (cb)); ev_embed_set ((ev),(other)); } while (0)
#define ev_fork_init(ev,cb)                  do { ev_init ((ev), (cb)); ev_fork_set ((ev)); } while (0)
#define ev_cleanup_init(ev,cb)               do { ev_init ((ev), (cb)); ev_cleanup_set ((ev)); } while (0)
#define ev_async_init(ev,cb)                 do { ev_init ((ev), (cb)); ev_async_set ((ev)); } while (0)

#define ev_is_pending(ev)                    (0 + ((ev_watcher *)(void *)(ev))->pending) /* ro, true when watcher is waiting for callback invocation */
#define ev_is_active(ev)                     (0 + ((ev_watcher *)(void *)(ev))->active) /* ro, true when the watcher has been started */

/**
 * 获取watcher的callback
 *  ev_cb_：直接获取
 *  ev_cb：先memmove，再返回callback指针
 * */ 
#define ev_cb_(ev)                           (ev)->cb /* rw */
#define ev_cb(ev)                            (memmove (&ev_cb_ (ev), &((ev_watcher *)(ev))->cb, sizeof (ev_cb_ (ev))), (ev)->cb)

#if EV_MINPRI == EV_MAXPRI
# define ev_priority(ev)                     ((ev), EV_MINPRI)
# define ev_set_priority(ev,pri)             ((ev), (pri))
#else
# define ev_priority(ev)                     (+(((ev_watcher *)(void *)(ev))->priority))
# define ev_set_priority(ev,pri)             (   (ev_watcher *)(void *)(ev))->priority = (pri)
#endif

#define ev_periodic_at(ev)                   (+((ev_watcher_time *)(ev))->at)

// 设置watcher的callback
#ifndef ev_set_cb
# define ev_set_cb(ev,cb_)                   (ev_cb_ (ev) = (cb_), memmove (&((ev_watcher *)(ev))->cb, &ev_cb_ (ev), sizeof (ev_cb_ (ev))))
#endif

/* stopping (enabling, adding) a watcher does nothing if it is already running */
/* stopping (disabling, deleting) a watcher does nothing unless it's already running */
#if EV_PROTOTYPES

/* feeds an event into a watcher as if the event actually occurred */
/* accepts any ev_watcher type */
/* 将watcher加入到pending队列或更新对应的event */
EV_API_DECL void ev_feed_event     (EV_P_ void *w, int revents) EV_THROW;
/* 将fd对应的watcher加入到pending队列或更新对应的event */
EV_API_DECL void ev_feed_fd_event  (EV_P_ int fd, int revents) EV_THROW;
#if EV_SIGNAL_ENABLE
EV_API_DECL void ev_feed_signal    (int signum) EV_THROW;
EV_API_DECL void ev_feed_signal_event (EV_P_ int signum) EV_THROW;
#endif
/* 调用watcher的callback */
EV_API_DECL void ev_invoke         (EV_P_ void *w, int revents);
/* 清除watcher的pending标志，并返回其watch的event */
EV_API_DECL int  ev_clear_pending  (EV_P_ void *w) EV_THROW;

/* 以下是各种watcher的start和stop函数 */
/* 将I/O watcher加入监听 */
EV_API_DECL void ev_io_start       (EV_P_ ev_io *w) EV_THROW;
/* 将I/O watcher移出监听 */
EV_API_DECL void ev_io_stop        (EV_P_ ev_io *w) EV_THROW;

EV_API_DECL void ev_timer_start    (EV_P_ ev_timer *w) EV_THROW;
EV_API_DECL void ev_timer_stop     (EV_P_ ev_timer *w) EV_THROW;
/* stops if active and no repeat, restarts if active and repeating, starts if inactive and repeating */
EV_API_DECL void ev_timer_again    (EV_P_ ev_timer *w) EV_THROW;
/* return remaining time */
EV_API_DECL ev_tstamp ev_timer_remaining (EV_P_ ev_timer *w) EV_THROW;

#if EV_PERIODIC_ENABLE
EV_API_DECL void ev_periodic_start (EV_P_ ev_periodic *w) EV_THROW;
EV_API_DECL void ev_periodic_stop  (EV_P_ ev_periodic *w) EV_THROW;
EV_API_DECL void ev_periodic_again (EV_P_ ev_periodic *w) EV_THROW;
#endif

/* only supported in the default loop */
#if EV_SIGNAL_ENABLE
EV_API_DECL void ev_signal_start   (EV_P_ ev_signal *w) EV_THROW;
EV_API_DECL void ev_signal_stop    (EV_P_ ev_signal *w) EV_THROW;
#endif

/* only supported in the default loop */
# if EV_CHILD_ENABLE
EV_API_DECL void ev_child_start    (EV_P_ ev_child *w) EV_THROW;
EV_API_DECL void ev_child_stop     (EV_P_ ev_child *w) EV_THROW;
# endif

# if EV_STAT_ENABLE
EV_API_DECL void ev_stat_start     (EV_P_ ev_stat *w) EV_THROW;
EV_API_DECL void ev_stat_stop      (EV_P_ ev_stat *w) EV_THROW;
EV_API_DECL void ev_stat_stat      (EV_P_ ev_stat *w) EV_THROW;
# endif

# if EV_IDLE_ENABLE
EV_API_DECL void ev_idle_start     (EV_P_ ev_idle *w) EV_THROW;
EV_API_DECL void ev_idle_stop      (EV_P_ ev_idle *w) EV_THROW;
# endif

#if EV_PREPARE_ENABLE
EV_API_DECL void ev_prepare_start  (EV_P_ ev_prepare *w) EV_THROW;
EV_API_DECL void ev_prepare_stop   (EV_P_ ev_prepare *w) EV_THROW;
#endif

#if EV_CHECK_ENABLE
EV_API_DECL void ev_check_start    (EV_P_ ev_check *w) EV_THROW;
EV_API_DECL void ev_check_stop     (EV_P_ ev_check *w) EV_THROW;
#endif

# if EV_FORK_ENABLE
EV_API_DECL void ev_fork_start     (EV_P_ ev_fork *w) EV_THROW;
EV_API_DECL void ev_fork_stop      (EV_P_ ev_fork *w) EV_THROW;
# endif

# if EV_CLEANUP_ENABLE
EV_API_DECL void ev_cleanup_start  (EV_P_ ev_cleanup *w) EV_THROW;
EV_API_DECL void ev_cleanup_stop   (EV_P_ ev_cleanup *w) EV_THROW;
# endif

# if EV_EMBED_ENABLE
/* only supported when loop to be embedded is in fact embeddable */
EV_API_DECL void ev_embed_start    (EV_P_ ev_embed *w) EV_THROW;
EV_API_DECL void ev_embed_stop     (EV_P_ ev_embed *w) EV_THROW;
EV_API_DECL void ev_embed_sweep    (EV_P_ ev_embed *w) EV_THROW;
# endif

# if EV_ASYNC_ENABLE
EV_API_DECL void ev_async_start    (EV_P_ ev_async *w) EV_THROW;
EV_API_DECL void ev_async_stop     (EV_P_ ev_async *w) EV_THROW;
EV_API_DECL void ev_async_send     (EV_P_ ev_async *w) EV_THROW;
# endif

#if EV_COMPAT3
  #define EVLOOP_NONBLOCK EVRUN_NOWAIT
  #define EVLOOP_ONESHOT  EVRUN_ONCE
  #define EVUNLOOP_CANCEL EVBREAK_CANCEL
  #define EVUNLOOP_ONE    EVBREAK_ONE
  #define EVUNLOOP_ALL    EVBREAK_ALL
  #if EV_PROTOTYPES
    EV_INLINE void ev_loop   (EV_P_ int flags) { ev_run   (EV_A_ flags); }
    EV_INLINE void ev_unloop (EV_P_ int how  ) { ev_break (EV_A_ how  ); }
    EV_INLINE void ev_default_destroy (void) { ev_loop_destroy (EV_DEFAULT); }
    EV_INLINE void ev_default_fork    (void) { ev_loop_fork    (EV_DEFAULT); }
    #if EV_FEATURE_API
      EV_INLINE unsigned int ev_loop_count  (EV_P) { return ev_iteration  (EV_A); }
      EV_INLINE unsigned int ev_loop_depth  (EV_P) { return ev_depth      (EV_A); }
      EV_INLINE void         ev_loop_verify (EV_P) {        ev_verify     (EV_A); }
    #endif
  #endif
#else
  typedef struct ev_loop ev_loop;
#endif

#endif

EV_CPP(})

#endif


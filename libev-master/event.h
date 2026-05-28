/*
 * libevent compatibility header, only core events supported
 * libevent 兼容头文件 - 仅支持核心事件（IO、定时器、信号）
 *
 * Copyright (c) 2007,2008,2010,2012 Marc Alexander Lehmann <libev@schmorp.de>
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
 * the BSD license. If you wish to allow the use of your version of this file
 * only under the terms of the GPL and not to allow others to use your
 * version of this file under the BSD license, indicate your decision
 * by deleting the provisions above and replace them with the notice and
 * other provisions required by the GPL. If you do not delete the
 * provisions above and above, a recipient may use your version of this file
 * under either the BSD or the GPL.
 */

#ifndef EVENT_H_
#define EVENT_H_

/* 包含 ev.h 头文件 */
#ifdef EV_H
# include EV_H
#else
# include "ev.h"
#endif

/* libevent API 兼容性宏定义 */
#ifndef EVLOOP_NONBLOCK
# define EVLOOP_NONBLOCK EVRUN_NOWAIT  /* 非阻塞模式 */
#endif
#ifndef EVLOOP_ONESHOT
# define EVLOOP_ONESHOT EVRUN_ONCE      /* 单次模式 */
#endif
#ifndef EV_TIMEOUT
# define EV_TIMEOUT EV_TIMER            /* 定时器事件 */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 需要 sys/time.h 来获取 struct timeval 定义 */
#if !defined (WIN32) || defined (__MINGW32__)
# include <time.h> /* mingw 出于某种原因需要这个 */
# include <sys/time.h>
#endif

/* libevent event_base 前向声明（实际未使用，为兼容性保留）*/
struct event_base;

/* libevent 事件列表标志 */
#define EVLIST_TIMEOUT  0x01  /* 定时器事件 */
#define EVLIST_INSERTED 0x02  /* 已插入事件列表 */
#define EVLIST_SIGNAL   0x04  /* 信号事件 */
#define EVLIST_ACTIVE   0x08  /* 活跃事件 */
#define EVLIST_INTERNAL 0x10  /* 内部使用 */
#define EVLIST_INIT     0x80  /* 已初始化 */

/* libevent 回调函数类型 */
typedef void (*event_callback_fn)(int, short, void *);

/* libevent event 结构体 - 将 libevent 的 event 映射到 libev 的 Watcher */
struct event
{
  /* 映射到 libev 的 Watcher */
  union {
    struct ev_io io;       /* IO Watcher */
    struct ev_signal sig;  /* 信号 Watcher */
  } iosig;                 /* IO 或信号 Watcher（二选一）*/
  struct ev_timer to;      /* 定时器 Watcher */

  /* 兼容性成员（保持 libevent API 兼容性）*/
  struct event_base *ev_base;  /* 事件基础（未实际使用）*/
  event_callback_fn ev_callback;  /* 回调函数 */
  void *ev_arg;               /* 回调参数 */
  int ev_fd;                  /* 文件描述符或信号编号 */
  int ev_pri;                 /* 优先级 */
  int ev_res;                 /* 结果事件类型 */
  int ev_flags;               /* 事件标志 */
  short ev_events;            /* 事件类型掩码 */
};

event_callback_fn event_get_callback (const struct event *ev);

/* libevent 事件类型定义（与 libev EV_* 兼容）*/
#define EV_READ                    EV_READ      /* 可读事件 */
#define EV_WRITE                   EV_WRITE     /* 可写事件 */
#define EV_PERSIST                 0x10         /* 持久事件（未完全支持）*/
#define EV_ET                      0x20         /* 边缘触发（未支持，空操作）*/

/* 获取事件的文件描述符或信号编号 */
#define EVENT_SIGNAL(ev)           ((int) (ev)->ev_fd)  /* 获取信号编号 */
#define EVENT_FD(ev)               ((int) (ev)->ev_fd)  /* 获取文件描述符 */

/* 检查事件是否已初始化 */
#define event_initialized(ev)      ((ev)->ev_flags & EVLIST_INIT)

/* 定时器相关宏（evtimer_*）*/
#define evtimer_add(ev,tv)         event_add (ev, tv)                           /* 添加定时器事件 */
#define evtimer_set(ev,cb,data)    event_set (ev, -1, 0, cb, data)             /* 设置定时器事件 */
#define evtimer_del(ev)            event_del (ev)                               /* 删除定时器事件 */
#define evtimer_pending(ev,tv)     event_pending (ev, EV_TIMEOUT, tv)          /* 检查定时器是否挂起 */
#define evtimer_initialized(ev)    event_initialized (ev)                       /* 检查定时器是否已初始化 */

/* timeout_* 宏（与 evtimer_* 相同，为别名）*/
#define timeout_add(ev,tv)         evtimer_add (ev, tv)                          /* 添加超时事件 */
#define timeout_set(ev,cb,data)    evtimer_set (ev, cb, data)                   /* 设置超时事件 */
#define timeout_del(ev)            evtimer_del (ev)                              /* 删除超时事件 */
#define timeout_pending(ev,tv)     evtimer_pending (ev, tv)                     /* 检查超时是否挂起 */
#define timeout_initialized(ev)    timeout_initialized (ev)                     /* 检查超时是否已初始化 */

/* 信号处理相关宏（signal_*）*/
#define signal_add(ev,tv)          event_add (ev, tv)                           /* 添加信号事件 */
#define signal_set(ev,sig,cb,data) event_set (ev, sig, EV_SIGNAL | EV_PERSIST, cb, data)  /* 设置信号事件 */
#define signal_del(ev)             event_del (ev)                               /* 删除信号事件 */
#define signal_pending(ev,tv)      event_pending (ev, EV_SIGNAL, tv)          /* 检查信号是否挂起 */
#define signal_initialized(ev)     event_initialized (ev)                       /* 检查信号是否已初始化 */

/* libevent API 函数声明 */
const char *event_get_version (void);        /* 获取 libevent 版本字符串*/
const char *event_get_method (void);         /* 获取使用的后端方法名称 */

void *event_init (void);                     /* 初始化默认事件循环（已废弃）*/
void event_base_free (struct event_base *base);  /* 释放 event_base*/

#define EVLOOP_ONCE      EVLOOP_ONESHOT       /* 单次循环标志 */
int event_loop (int);                        /* 运行事件循环（已废弃）*/
int event_loopexit (struct timeval *tv);     /* 退出事件循环（已废弃）*/
int event_dispatch (void);                   /* 分发事件（已废弃）*/

/* 日志级别定义 */
#define _EVENT_LOG_DEBUG 0  /* 调试级别 */
#define _EVENT_LOG_MSG   1  /* 消息级别 */
#define _EVENT_LOG_WARN  2  /* 警告级别 */
#define _EVENT_LOG_ERR   3  /* 错误级别 */
typedef void (*event_log_cb)(int severity, const char *msg);  /* 日志回调函数类型 */
void event_set_log_callback(event_log_cb cb);  /* 设置日志回调函数（未实现）*/

/* libevent 核心 API 函数 */
void event_set (struct event *ev, int fd, short events, void (*cb)(int, short, void *), void *arg);  /* 设置事件 */
int event_once (int fd, short events, void (*cb)(int, short, void *), void *arg, struct timeval *tv);  /* 等待单个事件 */

int event_add (struct event *ev, struct timeval *tv);  /* 添加事件到循环 */
int event_del (struct event *ev);                     /* 从循环中删除事件 */
void event_active (struct event *ev, int res, short ncalls); /* 激活事件（ncalls 参数被忽略）*/

int event_pending (struct event *ev, short, struct timeval *tv);  /* 检查事件是否挂起 */

int event_priority_init (int npri);        /* 初始化优先级（已废弃）*/
int event_priority_set (struct event *ev, int pri);  /* 设置事件优先级 */

/* libevent event_base API（部分实现）*/
struct event_base *event_base_new (void);  /* 创建新的 event_base（不做任何事）*/
const char *event_base_get_method (const struct event_base *);  /* 获取 event_base 的方法（总是返回 epoll）*/
int event_base_set (struct event_base *base, struct event *ev);  /* 设置事件所属的 event_base（不做任何事）*/
int event_base_loop (struct event_base *base, int);               /* 运行 event_base 的循环（总是运行默认循环）*/
int event_base_loopexit (struct event_base *base, struct timeval *tv);  /* 退出 event_base 循环（退出默认循环）*/
int event_base_dispatch (struct event_base *base);                /* 分发 event_base 的事件（调用默认循环）*/
int event_base_once (struct event_base *base, int fd, short events, void (*cb)(int, short, void *), void *arg, struct timeval *tv);  /* 等待单个 event_base 事件（使用默认循环）*/
int event_base_priority_init (struct event_base *base, int fd);  /* 初始化 event_base 优先级（不做任何事）*/

/* next line is different in the libevent+libev version */
/*libevent-include*/

#ifdef __cplusplus
}
#endif

#endif


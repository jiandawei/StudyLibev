/*
 * loop member variable declarations
 * 事件循环成员变量声明 - 定义 ev_loop 结构体的所有字段
 *
 * Copyright (c) 2007,2008,2009,2010,2011,2012,2013 Marc Alexander Lehmann <libev@schmorp.de>
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

#define VARx(type,name) VAR(name, type name)  /* 变量定义宏：声明变量及其类型和名称 */

/* ========================================
   时间管理相关字段
   ======================================== */

VARx(ev_tstamp, now_floor)    /* 上次刷新实时时间的缓存值 */
VARx(ev_tstamp, mn_now)       /* 单调时钟的当前时间（单调递增，不受系统时间调整影响）*/
VARx(ev_tstamp, rtmn_diff)    /* 实时时间与单调时间的差值（用于时间转换）*/

/* ========================================
   事件反转（reverse feeding）相关字段
   用于在事件处理过程中反向触发事件
   ======================================== */

VARx(W *, rfeeds)             /* 反向事件投递队列：存储需要反向触发的 watcher 指针数组 */
VARx(int, rfeedmax)           /* 反向事件队列的最大容量 */
VARx(int, rfeedcnt)           /* 反向事件队列的当前元素数量 */

/* ========================================
   待处理事件队列（pending queue）相关字段
   存储等待触发回调的 watcher
   ======================================== */

VAR (pendings, ANPENDING *pendings [NUMPRI])              /* 每个优先级的待处理事件数组（5 个优先级：-2 到 +2），二维数组：pendings[NUMPRI][pendingmax]*/
VAR (pendingmax, int pendingmax [NUMPRI])                 /* 每个优先级队列的最大容量 */
VAR (pendingcnt, int pendingcnt [NUMPRI])                 /* 每个优先级队列的当前待处理事件数量 */
VARx(int, pendingpri)                                      /* 当前最高的待处理优先级 */
VARx(ev_prepare, pending_w)                               /* 临时的 pending watcher（用于 pending 机制）*/

/* ========================================
   阻塞时间相关字段
   控制事件循环的阻塞行为
   ======================================== */

VARx(ev_tstamp, io_blocktime)                          /* IO 操作的阻塞时间（最小休眠时间）*/
VARx(ev_tstamp, timeout_blocktime)                     /* 超时操作的阻塞时间（最小休眠时间）*/

/* ========================================
   事件循环后端（backend）相关字段
   管理底层 IO 多路复用机制
   ======================================== */

VARx(int, backend)                                      /* 当前使用的后端类型（select/poll/epoll/kqueue/port 等）*/
VARx(int, activecnt)                                   /* 活跃事件总数（引用计数）*/
VARx(EV_ATOMIC_T, loop_done)                           /* 循环退出标志（由 ev_break 设置）*/

VARx(int, backend_fd)                                  /* 后端文件描述符（如 epoll_create 的返回值）*/
VARx(ev_tstamp, backend_mintime)                       /* 后端的典型定时器分辨率（防止过短的阻塞）*/
VAR (backend_modify, void (*backend_modify)(EV_P_ int fd, int oev, int nev))  /* 修改后端监控事件的函数指针。oev：old events，nev：new events */
VAR (backend_poll  , void (*backend_poll)(EV_P_ ev_tstamp timeout))         /* 后端轮询等待事件的函数指针 */

/* ========================================
   文件描述符（fd）管理相关字段
   管理所有监控的文件描述符
   ======================================== */

VARx(ANFD *, anfds)                                     /* 文件描述符信息数组：每个 fd 对应的 watcher 链表 */
VARx(int, anfdmax)                                      /* 文件描述符数组的最大容量（已分配的 fd 数量）*/

/* ========================================
   管道（pipe）相关字段
   用于跨线程或异步通知
   ======================================== */

VAR (evpipe, int evpipe [2])                           /* 管道数组：[0] 读端，[1] 写端，用于跨线程通信 */
VARx(ev_io, pipe_w)                                    /* 监控管道读端的 IO watcher */
VARx(EV_ATOMIC_T, pipe_write_wanted)                   /* 标志：是否希望写入管道（原子操作）*/
VARx(EV_ATOMIC_T, pipe_write_skipped)                   /* 标志：是否跳过了管道写入（原子操作）*/

/* ========================================
   进程 ID 相关字段
   用于检测 fork
   ======================================== */

#if !defined(_WIN32) || EV_GENWRAP
VARx(pid_t, curpid)                                    /* 当前进程 ID（用于检测 fork）*/
#endif

/* ========================================
   Fork 处理相关字段
   ======================================== */

VARx(char, postfork)                                   /* 标志：是否在 fork 后需要重新创建内核状态 */

#if EV_USE_SELECT || EV_GENWRAP
/* ========================================
   Select 后端特定字段
   ======================================== */
/**
 * 读和写位向量的副本是为了避免每次都有重新设置 
 * i：input，o：output
 * */
VARx(void *, vec_ri)                                    /* select 读位向量（read fd_set）*/
VARx(void *, vec_ro)                                    /* select 读位向量副本（read fd_set 副本）*/
VARx(void *, vec_wi)                                    /* select 写位向量（write fd_set）*/
VARx(void *, vec_wo)                                    /* select 写位向量副本（write fd_set 副本）*/
#if defined(_WIN32) || EV_GENWRAP
VARx(void *, vec_eo)                                    /* select 异常位向量（exception fd_set，Windows）*/
#endif
VARx(int, vec_max)                                      /* select 向量的最大 fd 数量 */
#endif

#if EV_USE_POLL || EV_GENWRAP
/* ========================================
   Poll 后端特定字段
   ======================================== */
VARx(struct pollfd *, polls)                            /* poll 函数使用的 pollfd 数组 */
VARx(int, pollmax)                                      /* polls 数组的最大容量 */
VARx(int, pollcnt)                                      /* polls 数组的当前 fd 数量 */
VARx(int *, pollidxs)                                   /* fd 到 polls 数组索引的映射表 */
VARx(int, pollidxmax)                                   /* pollidxs 数组的最大容量 */
#endif

#if EV_USE_EPOLL || EV_GENWRAP
/* ========================================
   Epoll 后端特定字段（Linux 高性能 IO）
   ======================================== */
VARx(struct epoll_event *, epoll_events)                 /* epoll_wait 返回的事件数组 */
VARx(int, epoll_eventmax)                               /* epoll_events 数组最大容量 */
VARx(int *, epoll_eperms)                               /* EPERM 状态的 fd 数组（epoll 返回 EPERM 的 fd）*/
VARx(int, epoll_epermcnt)                               /* epoll_eperms 数组的当前 fd 数量 */
VARx(int, epoll_epermmax)                               /* epoll_eperms 数组最大容量 */
#endif

#if EV_USE_KQUEUE || EV_GENWRAP
/* ========================================
   Kqueue 后端特定字段（BSD/macOS 高性能 IO）
   ======================================== */
VARx(pid_t, kqueue_fd_pid)                              /* 创建 kqueue 的进程 ID */
VARx(struct kevent *, kqueue_changes)                   /* kqueue 变更事件数组（需要监控/修改的事件）*/
VARx(int, kqueue_changemax)                             /* kqueue_changes 数组最大容量 */
VARx(int, kqueue_changecnt)                             /* kqueue_changes 数组当前事件数量 */
VARx(struct kevent *, kqueue_events)                    /* kqueue 返回的事件数组 */
VARx(int, kqueue_eventmax)                              /* kqueue_events 数组最大容量 */
#endif

#if EV_USE_PORT || EV_GENWRAP
/* ========================================
   Port 后端特定字段（Solaris 10+ event ports）
   ======================================== */
VARx(struct port_event *, port_events)                  /* port_getn 返回的事件数组 */
VARx(int, port_eventmax)                               /* port_events 数组最大容量 */
#endif

#if EV_USE_IOCP || EV_GENWRAP
/* ========================================
   IOCP 后端特定字段（Windows IO 完成端口）
   ======================================== */
VARx(HANDLE, iocp)                                      /* IOCP 端口句柄 */
#endif

/* ========================================
   文件描述符变更管理相关字段
   跟踪需要更新后端状态的 fd
   ======================================== */

VARx(int *, fdchanges)                                  /* 需要变更的 fd 数组 */
VARx(int, fdchangemax)                                  /* fdchanges 数组最大容量 */
VARx(int, fdchangecnt)                                  /* fdchanges 数组当前数量 */

/* ========================================
   定时器（timer watcher）相关字段
   使用最小堆管理
   ======================================== */

VARx(ANHE *, timers)                                    /* 定时器堆数组（按触发时间排序的最小堆）*/
VARx(int, timermax)                                     /* timers 数组最大容量 */
VARx(int, timercnt)                                     /* timers 数组当前数量 */

/* ========================================
   周期性定时器（periodic watcher）相关字段
   使用最小堆管理
   ======================================== */

#if EV_PERIODIC_ENABLE || EV_GENWRAP
VARx(ANHE *, periodics)                                 /* 周期性定时器堆数组 */
VARx(int, periodicmax)                                  /* periodics 数组最大容量 */
VARx(int, periodiccnt)                                  /* periodics 数组当前数量 */
#endif

/* ========================================
   Idle watcher 相关字段
   事件循环空闲时触发
   ======================================== */

#if EV_IDLE_ENABLE || EV_GENWRAP
VAR (idles, ev_idle **idles [NUMPRI])                   /* 每个优先级的 idle watcher 指针数组 */
VAR (idlemax, int idlemax [NUMPRI])                     /* 每个优先级 idle 数组的最大容量 */
VAR (idlecnt, int idlecnt [NUMPRI])                     /* 每个优先级 idle 数组的当前数量 */
#endif
VARx(int, idleall)                                      /* 所有 idle watcher 总数 */

/* ========================================
   Prepare watcher 相关字段
   每次循环迭代开始前触发
   ======================================== */

VARx(struct ev_prepare **, prepares)                     /* prepare watcher 指针数组 */
VARx(int, preparemax)                                   /* prepares 数组最大容量 */
VARx(int, preparecnt)                                   /* prepares 数组当前数量 */

/* ========================================
   Check watcher 相关字段
   每次循环迭代结束后触发
   ======================================== */

VARx(struct ev_check **, checks)                        /* check watcher 指针数组 */
VARx(int, checkmax)                                     /* checks 数组最大容量 */
VARx(int, checkcnt)                                     /* checks 数组当前数量 */

/* ========================================
   Fork watcher 相关字段
   子进程中触发
   ======================================== */

#if EV_FORK_ENABLE || EV_GENWRAP
VARx(struct ev_fork **, forks)                          /* fork watcher 指针数组 */
VARx(int, forkmax)                                      /* forks 数组最大容量 */
VARx(int, forkcnt)                                      /* forks 数组当前数量 */
#endif

/* ========================================
   Cleanup watcher 相关字段
   循环销毁前触发
   ======================================== */

#if EV_CLEANUP_ENABLE || EV_GENWRAP
VARx(struct ev_cleanup **, cleanups)                    /* cleanup watcher 指针数组 */
VARx(int, cleanupmax)                                   /* cleanups 数组最大容量 */
VARx(int, cleanupcnt)                                   /* cleanups 数组当前数量 */
#endif

/* ========================================
   Async watcher 相关字段
   跨线程异步通知
   ======================================== */

#if EV_ASYNC_ENABLE || EV_GENWRAP
VARx(EV_ATOMIC_T, async_pending)                        /* async watcher 待处理标志（原子操作）*/
VARx(struct ev_async **, asyncs)                        /* async watcher 指针数组 */
VARx(int, asyncmax)                                     /* asyncs 数组最大容量 */
VARx(int, asynccnt)                                     /* asyncs 数组当前数量 */
#endif

/* ========================================
   文件系统监听（inotify）相关字段
   Linux 特有的文件监听
   ======================================== */

#if EV_USE_INOTIFY || EV_GENWRAP
VARx(int, fs_fd)                                        /* inotify 文件描述符 */
VARx(ev_io, fs_w)                                       /* 监控 inotify fd 的 IO watcher */
VARx(char, fs_2625)                                    /* 标志：是否运行在 Linux 2.6.25 或更新版本 */
VAR (fs_hash, ANFS fs_hash [EV_INOTIFY_HASHSIZE])       /* 文件系统监听的哈希表 */
#endif

/* ========================================
   信号处理相关字段
   ======================================== */

VARx(EV_ATOMIC_T, sig_pending)                          /* 信号待处理标志（原子操作）*/
#if EV_USE_SIGNALFD || EV_GENWRAP
VARx(int, sigfd)                                        /* signalfd 文件描述符 */
VARx(ev_io, sigfd_w)                                    /* 监控 signalfd 的 IO watcher */
VARx(sigset_t, sigfd_set)                              /* signalfd 使用的信号集 */
#endif

/* ========================================
   循环配置相关字段
   ======================================== */

VARx(unsigned int, origflags)                           /* 原始循环标志（创建时传入的参数）*/

/* ========================================
   高级 API（EV_FEATURE_API）相关字段
   用于线程安全和高级控制
   ======================================== */

#if EV_FEATURE_API || EV_GENWRAP
VARx(unsigned int, loop_count)                          /* 循环迭代计数：总迭代次数或次数统计 */
VARx(unsigned int, loop_depth)                          /* 循环嵌套深度：ev_run 进入次数 - ev_run 退出次数 */

VARx(void *, userdata)                                  /* 用户自定义数据指针（可绑定任意数据到循环）*/
/* C++ 不支持这里的 ev_loop_callback typedef（C++ 语法限制）*/
VAR (release_cb, void (*release_cb)(EV_P) EV_THROW)     /* 循环释放回调：在释放循环资源前调用 */
VAR (acquire_cb, void (*acquire_cb)(EV_P) EV_THROW)     /* 循环获取回调：在获取循环后调用 */
VAR (invoke_cb , ev_loop_callback invoke_cb)            /* 待处理事件调用回调：替代默认的待处理事件调用逻辑 */
#endif

#undef VARx  /* 取消 VARx 宏定义，避免污染后续代码 */


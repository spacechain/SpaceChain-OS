/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: lib_strerror.c
**
** 创   建   人: Han.Hui (韩辉)
**
** 文件创建日期: 2006 年 12 月 25 日
**
** 描        述: 库
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
/*********************************************************************************************************
** 函数名称: lib_strerror
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PCHAR  lib_strerror (INT  iNum)
{
    switch (iNum) {
    
    /*
     *  Posix error code
     */
    case 0:             return  ("No error");
    case EPERM:         return  ("Not owner");
    case ENOENT:        return  ("No such file or directory");
    case ESRCH:         return  ("No such process");
    case EINTR:         return  ("Interrupted system call");
    case EIO:           return  ("I/O error");
    case ENXIO:         return  ("No such device or address");
    case E2BIG:         return  ("Arg list too long or overflow");
    case ENOEXEC:       return  ("Exec format error");
    case EBADF:         return  ("Bad file number");
    case ECHILD:        return  ("No children");
    case EAGAIN:        return  ("No more processes or operation would block");
    case ENOMEM:        return  ("No enough memory");
    case EACCES:        return  ("Permission denied or can not access");
    case EFAULT:        return  ("Bad address");
    case ENOTEMPTY:     return  ("Directory not empty");
    case EBUSY:         return  ("Mount device busy");
    case EEXIST:        return  ("File exists");
    case EXDEV:         return  ("Cross-device link");
    case ENODEV:        return  ("No such device");
    case ENOTDIR:       return  ("Not a directory");
    case EISDIR:        return  ("Is a directory");
    case EINVAL:        return  ("Invalid argument or format");
    case ENFILE:        return  ("File table overflow");
    case EMFILE:        return  ("Too many open files");
    case ENOTTY:        return  ("Not a typewriter");
    case ENAMETOOLONG:  return  ("File name too long");
    case EFBIG:         return  ("File too large");
    case ENOSPC:        return  ("No space left on device");
    case ESPIPE:        return  ("Illegal seek");
    case EROFS:         return  ("Read-only file system");
    case EMLINK:        return  ("Too many links");
    case EPIPE:         return  ("Broken pipe");
    case EDEADLK:       return  ("Resource deadlock avoided");
    case ENOLCK:        return  ("No locks available");
    case ENOTSUP:       return  ("Unsupported value");
    case EMSGSIZE:      return  ("Message size");
    
    /*
     * ANSI math software
     */
    case EDOM:          return  ("Argument too large");
    case ERANGE:        return  ("Result too large");

    /*
     *  ipc/network software (argument errors)
     */
    case EDESTADDRREQ:      return  ("Destination address required");
    case EPROTOTYPE:        return  ("Protocol wrong type for socket");
    case ENOPROTOOPT:       return  ("Protocol not available");
    case EPROTONOSUPPORT:   return  ("Protocol not supported");
    case ESOCKTNOSUPPORT:   return  ("Socket type not supported");
    case EOPNOTSUPP:        return  ("Operation not supported on socket");
    case EPFNOSUPPORT:      return  ("Protocol family not supported");
    case EAFNOSUPPORT:      return  ("Addr family not supported");
    case EADDRINUSE:        return  ("Address already in use");
    case EADDRNOTAVAIL:     return  ("Can't assign requested address");
    case ENOTSOCK:          return  ("Socket operation on non-socket");

    /*
     *  ipc/network software (operational errors)
     */
    case ENETUNREACH:   return  ("Network unreachable");
    case ENETRESET:     return  ("Network dropped connection on reset");
    case ECONNABORTED:  return  ("Software caused connection abort");
    case ECONNRESET:    return  ("Connection reset by peer");
    case ENOBUFS:       return  ("No buffer space available");
    case EISCONN:       return  ("Socket is already connected");
    case ENOTCONN:      return  ("Socket is not connected");
    case ESHUTDOWN:     return  ("Can't send after socket shutdown");
    case ETOOMANYREFS:  return  ("Too many references: can't splice");
    case ETIMEDOUT:     return  ("Connection timed out");
    case ECONNREFUSED:  return  ("Connection refused");
    case ENETDOWN:      return  ("Network is down");
    case ETXTBSY:       return  ("Text file busy");
    case ELOOP:         return  ("Too many levels of symbolic links");
    case EHOSTUNREACH:  return  ("Host unreachable");
    case ENOTBLK:       return  ("Block device required");
    case EHOSTDOWN:     return  ("Host is down");

    /*
     *  ipc/network software (non-blocking and interrupt i/o)
     */
    case EINPROGRESS:   return  ("Operation now in progress");
    case EALREADY:      return  ("Operation already in progress");
    case ENOSYS:        return  ("Function not implemented");

    /*
     *  aio errors (should be under posix)
     */
    case ECANCELED:      return  ("Operation canceled");

    /*
     *  specific STREAMS errno values
     */
    case ENOSR:         return  ("Insufficient memory");
    case ENOSTR:        return  ("STREAMS device required");
    case EPROTO:        return  ("Generic STREAMS error");
    case EBADMSG:       return  ("Invalid STREAMS message");
    case ENODATA:       return  ("Missing expected message data");
    case ETIME:         return  ("STREAMS timeout occurred");
    case ENOMSG:        return  ("Unexpected message type");

    /*
     *  io errors (should be under posix)
     */
    case EWRPROTECT:    return  ("Write protect");
    case EFORMAT:       return  ("Invalid format");
    
    /*
     *  others
     */
    case EIDRM:         return  ("Identifier removed");
    case ECHRNG:        return  ("Channel number out of range");
    case EL2NSYNC:      return  ("Level 2 not synchronized");
    case EL3HLT:        return  ("Level 3 halted");
    case EL3RST:        return  ("Level 3 reset");
    case ELNRNG:        return  ("Link number out of range");
    case EUNATCH:       return  ("Protocol driver not attached");
    case ENOCSI:        return  ("No CSI structure available");
    case EL2HLT:        return  ("Level 2 halted");
    case EBADE:         return  ("Invalid exchange");
    case EBADR:         return  ("Invalid request descriptor");
    case EXFULL:        return  ("Exchange full");
    case ENOANO:        return  ("No anode");
    case EBADRQC:       return  ("Invalid request code");
    case EBADSLT:       return  ("Invalid slot");
    case EBFONT:        return  ("Bad font file format");
    case ENONET:        return  ("Machine is not on the network");
    case ENOPKG:        return  ("Package not installed");
    case EREMOTE:       return  ("Object is remote");
    case ENOLINK:       return  ("Link has been severed");
    case EADV:          return  ("Advertise error");
    case ESRMNT:        return  ("Srmount error");
    case ECOMM:         return  ("Communication error on send");
    case EMULTIHOP:     return  ("Multihop attempted");
    case EDOTDOT:       return  ("RFS specific error");
    case EUCLEAN:       return  ("Structure needs cleaning");
    case ENOTUNIQ:      return  ("Name not unique on network");
    case EBADFD:        return  ("File descriptor in bad state");
    case EREMCHG:       return  ("Remote address changed");
    case ELIBACC:       return  ("Can not access a needed shared library");
    case ELIBBAD:       return  ("Accessing a corrupted shared library");
    case ELIBSCN:       return  (".lib section in a.out corrupted");
    case ELIBMAX:       return  ("Attempting to link in too many shared libraries");
    case ELIBEXEC:      return  ("Cannot exec a shared library directly");
    case ERESTART:      return  ("Interrupted system call should be restarted");
    case ESTRPIPE:      return  ("Streams pipe error");
    case EUSERS:        return  ("Too many users");
    case ESTALE:        return  ("Stale NFS file handle");
    case ENOTNAM:       return  ("Not a XENIX named type file");
    case ENAVAIL:       return  ("No XENIX semaphores available");
    case EISNAM:        return  ("Is a named type file");
    case EREMOTEIO:     return  ("Remote I/O error");
    case EDQUOT:        return  ("Quota exceeded");
    case ENOMEDIUM:     return  ("No medium found");
    case EMEDIUMTYPE:   return  ("Wrong medium type");
    case EILSEQ:        return  ("Illegal byte sequence");
    
    /*
     *  For robust mutexes
     */
    case EOWNERDEAD:        return  ("Owner died");
    case ENOTRECOVERABLE:   return  ("State not recoverable");
#if defined(LW_CFG_CPU_ARCH_C6X)
    }

    switch (iNum) {
#endif
    /*
     *  SylixOS kernel error
     */
    case ERROR_KERNEL_PNAME_NULL:       return  ("Invalid name");
    case ERROR_KERNEL_PNAME_TOO_LONG:   return  ("Name too long");
    case ERROR_KERNEL_HANDLE_NULL:      return  ("Invalid handle");
    case ERROR_KERNEL_IN_ISR:           return  ("Kernel in interrupt service mode");
    case ERROR_KERNEL_RUNNING:          return  ("Kernel is running");
    case ERROR_KERNEL_NOT_RUNNING:      return  ("Kernel is not running");
    case ERROR_KERNEL_OBJECT_NULL:      return  ("Invalid object");
    case ERROR_KERNEL_LOW_MEMORY:       return  ("Kernel not enough memory");
    case ERROR_KERNEL_BUFFER_NULL:      return  ("Invalid buffer");
    case ERROR_KERNEL_OPTION:           return  ("Unsupported option");
    case ERROR_KERNEL_VECTOR_NULL:      return  ("Invalid vector");
    case ERROR_KERNEL_HOOK_NULL:        return  ("Invalid hook");
    case ERROR_KERNEL_OPT_NULL:         return  ("Invalid option");
    case ERROR_KERNEL_MEMORY:           return  ("Invalid address");
    case ERROR_KERNEL_LOCK:             return  ("Kernel locked");
    case ERROR_KERNEL_CPU_NULL:         return  ("Invalid cpu");
    case ERROR_KERNEL_HOOK_FULL:        return  ("Hook table full");
    case ERROR_KERNEL_KEY_CONFLICT:     return  ("Key conflict");

    /*
     *  thread 
     */
    case ERROR_THREAD_STACKSIZE_LACK:   return  ("Not enough stack");
    case ERROR_THREAD_STACK_NULL:       return  ("Invalid stack");
    case ERROR_THREAD_FP_STACK_NULL:    return  ("Invalid FP stack");
    case ERROR_THREAD_ATTR_NULL:        return  ("Invalid attribute");
    case ERROR_THREAD_PRIORITY_WRONG:   return  ("Invalid priority");
    case ERROR_THREAD_WAIT_TIMEOUT:     return  ("Wait timed out");
    case ERROR_THREAD_NULL:             return  ("Invalid thread");
    case ERROR_THREAD_FULL:             return  ("Thread full");
    case ERROR_THREAD_NOT_INIT:         return  ("Thread not initialized");
    case ERROR_THREAD_NOT_SUSPEND:      return  ("Thread not suspend");
    case ERROR_THREAD_VAR_FULL:         return  ("Thread var full");
    case ERROR_THERAD_VAR_NULL:         return  ("Invalid thread var");
    case ERROR_THREAD_VAR_NOT_EXIST:    return  ("Thread var not exist");
    case ERROR_THREAD_NOT_READY:        return  ("Thread not ready");
    case ERROR_THREAD_IN_SAFE:          return  ("Thread in safe mode");
    case ERROR_THREAD_OTHER_DELETE:     return  ("Thread has been delete by other");
    case ERROR_THREAD_JOIN_SELF:        return  ("Thread join self");
    case ERROR_THREAD_DETACHED:         return  ("Thread detached");
    case ERROR_THREAD_JOIN:             return  ("Thread join");
    case ERROR_THREAD_NOT_SLEEP:        return  ("Thread not sleep");
    case ERROR_THREAD_NOTEPAD_INDEX:    return  ("Invalid notepad index");
    case ERROR_THREAD_OPTION:           return  ("Invalid option");
    case ERROR_THREAD_RESTART_SELF:     return  ("Thread restart self");
    case ERROR_THREAD_DELETE_SELF:      return  ("Thread delete self");
    case ERROR_THREAD_NEED_SIGNAL_SPT:  return  ("Thread need signal support");
    case ERROR_THREAD_DISCANCEL:        return  ("Thread discancel");

    /*
     *  time
     */
    case ERROR_TIME_NULL:               return  ("Invalid time");

    /*
     *  event
     */
    case ERROR_EVENT_MAX_COUNTER_NULL:  return  ("Invalid event max counter");
    case ERROR_EVENT_INIT_COUNTER:      return  ("Invalid event counter");
    case ERROR_EVENT_NULL:              return  ("Invalid event");
    case ERROR_EVENT_FULL:              return  ("Event full");
    case ERROR_EVENT_TYPE:              return  ("Event type");
    case ERROR_EVENT_WAS_DELETED:       return  ("Event was delete");
    case ERROR_EVENT_NOT_OWN:           return  ("Event not own");
    
    /*
     *  interrupt
     */
    case ERROR_INTER_LEVEL_NULL:        return  ("Invalid interrupt level");

    /*
     *  event set
     */
    case ERROR_EVENTSET_NULL:           return  ("Invalid eventset");
    case ERROR_EVENTSET_FULL:           return  ("Eventset full");
    case ERROR_EVENTSET_TYPE:           return  ("Eventset type");
    case ERROR_EVENTSET_WAIT_TYPE:      return  ("Eventset wait type");
    case ERROR_EVENTSET_WAS_DELETED:    return  ("Eventset was delete");
    case ERROR_EVENTSET_OPTION:         return  ("Eventset option");

    /*
     *  message queue
     */
    case ERROR_MSGQUEUE_MAX_COUNTER_NULL:   return  ("Invalid MQ max counter");
    case ERROR_MSGQUEUE_MAX_LEN_NULL:       return  ("Invalid MQ max length");
    case ERROR_MSGQUEUE_FULL:               return  ("MQ full");
    case ERROR_MSGQUEUE_NULL:               return  ("Invalid MQ");
    case ERROR_MSGQUEUE_TYPE:               return  ("MQ type");
    case ERROR_MSGQUEUE_WAS_DELETED:        return  ("MQ was delete");
    case ERROR_MSGQUEUE_MSG_NULL:           return  ("Invalid MQ message");
    case ERROR_MSGQUEUE_MSG_LEN:            return  ("Invalid MQ message length");
    case ERROR_MSGQUEUE_OPTION:             return  ("MQ option");
    
    /*
     *  timer
     */
    case ERROR_TIMER_FULL:              return  ("Timer full");
    case ERROR_TIMER_NULL:              return  ("Invalid timer");
    case ERROR_TIMER_CALLBACK_NULL:     return  ("Invalid timer callback");
    case ERROR_TIMER_ISR:               return  ("In timer interrupt service");
    case ERROR_TIMER_TIME:              return  ("Invalid time");
    case ERROR_TIMER_OPTION:            return  ("Timer option");

    /*
     *  partition
     */
    case ERROR_PARTITION_FULL:          return  ("Partition full");
    case ERROR_PARTITION_NULL:          return  ("Invalid partition");
    case ERROR_PARTITION_BLOCK_COUNTER: return  ("Invalid partition block counter");
    case ERROR_PARTITION_BLOCK_SIZE:    return  ("Invalid partition block size");
    case ERROR_PARTITION_BLOCK_USED:    return  ("Partition used");
    
    /*
     *  region
     */
    case ERROR_REGION_FULL:             return  ("Region full");
    case ERROR_REGION_NULL:             return  ("Invalid region");
    case ERROR_REGION_SIZE:             return  ("Invalid region size");
    case ERROR_REGION_USED:             return  ("Pegion used");
    case ERROR_REGION_ALIGN:            return  ("Miss align");
    case ERROR_REGION_NOMEM:            return  ("No enough memory");

    /*
     *  rms
     */
    case ERROR_RMS_FULL:                return  ("RMS full");
    case ERROR_RMS_NULL:                return  ("Invalid RMS");
    case ERROR_RMS_TICK:                return  ("RMS tick");
    case ERROR_RMS_WAS_CHANGED:         return  ("RMS was changed");
    case ERROR_RMS_STATUS:              return  ("RMS status");
    
    /*
     *  rtc
     */
    case ERROR_RTC_NULL:                return  ("Invalid RTC");
    case ERROR_RTC_TIMEZONE:            return  ("Invalid timezone");

    /*
     *  vmm
     */
    case ERROR_VMM_LOW_PHYSICAL_PAGE:   return  ("Not enough physical page");
    case ERROR_VMM_LOW_LEVEL:           return  ("Low level error");
    case ERROR_VMM_PHYSICAL_PAGE:       return  ("Physical page error");
    case ERROR_VMM_VIRTUAL_PAGE:        return  ("Virtual page error");
    case ERROR_VMM_PHYSICAL_ADDR:       return  ("Invalid physical address");
    case ERROR_VMM_VIRTUAL_ADDR:        return  ("Invalid virtual address");
    case ERROR_VMM_ALIGN:               return  ("Miss page align");
    case ERROR_VMM_PAGE_INVAL:          return  ("Invalid page");
    case ERROR_VMM_LOW_PAGE:            return  ("Low page");

    /*
     *  system
     */
    case ERROR_SYSTEM_LOW_MEMORY:         return  ("System not enough memory");

    /*
     *  I/O system 
     */
    case ERROR_IOS_DRIVER_GLUT:             return  ("Driver full");
    case ERROR_IOS_FILE_OPERATIONS_NULL:    return  ("Invalid file operations");
    case ERROR_IOS_FILE_READ_PROTECTED:     return  ("Read protected");
    case ERROR_IOS_FILE_SYMLINK:            return  ("symbol link");
    case ERROR_IO_NO_DRIVER:                return  ("No driver");
    case ERROR_IO_BUFFER_ERROR:             return  ("Buffer incorrect");
    case ERROR_IO_VOLUME_ERROR:             return  ("Volume incorrect");

    /*
     *  select
     */
    case ERROR_IO_SELECT_UNSUPPORT_IN_DRIVER:   return  ("Driver unsupport select");
    case ERROR_IO_SELECT_CONTEXT:               return  ("Invalid select context");
    case ERROR_IO_SELECT_WIDTH:                 return  ("Invalid width");
    case ERROR_IO_SELECT_FDSET_NULL:            return  ("Invalid fd set");

    /*
     *  thread pool
     */
    case ERROR_THREADPOOL_NULL:         return  ("Invalid threadpool");
    case ERROR_THREADPOOL_FULL:         return  ("Threadpool full");
    case ERROR_THREADPOOL_MAX_COUNTER:  return  ("Invalid threadpool Max counter");

    /*
     *  system hool lisk
     */
    case ERROR_SYSTEM_HOOK_NULL:        return  ("Invalid hook");

    /*
     *  except & log
     */
    case ERROR_EXCE_LOST:               return  ("Exception message lost");
    case ERROR_LOG_LOST:                return  ("Log message lost");
    case ERROR_LOG_FMT:                 return  ("Invalid log format");
    case ERROR_LOG_FDSET_NULL:          return  ("Invalid fd set");

    /*
     *  DMA
     */
    case ERROR_DMA_CHANNEL_INVALID:     return  ("Invalid DMA channel");
    case ERROR_DMA_TRANSMSG_INVALID:    return  ("Invalid DMA Transmessage");
    case ERROR_DMA_DATA_TOO_LARGE:      return  ("Data too large");
    case ERROR_DMA_NO_FREE_NODE:        return  ("No free DMA node");
    case ERROR_DMA_MAX_NODE:            return  ("Max DMA node in queue");

    /*
     *  power management
     */
    case ERROR_POWERM_NODE:         return  ("Invalid PowerM node");
    case ERROR_POWERM_TIME:         return  ("Invalid PowerM time");
    case ERROR_POWERM_FUNCTION:     return  ("Invalid PowerM function");
    case ERROR_POWERM_NULL:         return  ("Invalid PowerM");
    case ERROR_POWERM_FULL:         return  ("PowerM full");
    case ERROR_POWERM_STATUS:       return  ("PowerM status");

    /*
     *  signal
     */
    case ERROR_SIGNAL_SIGQUEUE_NODES_NULL:  return  ("Not enough sigqueue node");

    /*
     *  hotplug
     */
    case ERROR_HOTPLUG_POLL_NODE_NULL:      return  ("No hotplug node");
    case ERROR_HOTPLUG_MESSAGE_NULL:        return  ("No hotplug message");
    
    /*
     *  loader
     */
#if LW_CFG_MODULELOADER_EN > 0
    case ERROR_LOADER_FORMAT:           return  ("Invalid format");
    case ERROR_LOADER_ARCH:             return  ("Invalid architectural");
    case ERROR_LOADER_RELOCATE:         return  ("Reloacate error");
    case ERROR_LOADER_EXPORT_SYM:       return  ("Can not export symbol(s)");
    case ERROR_LOADER_NO_MODULE:        return  ("Can not find module");
    case ERROR_LOADER_CREATE:           return  ("Can not create module");
    case ERROR_LOADER_NO_INIT:          return  ("Can not find initial routien");
    case ERROR_LOADER_NO_ENTRY:         return  ("Can not find entry routien");
    case ERROR_LOADER_PARAM_NULL:       return  ("Invalid parameter(s)");
    case ERROR_LOADER_UNEXPECTED:       return  ("Unexpected error");
    case ERROR_LOADER_NO_SYMBOL:        return  ("Can not find symbol");
    case ERROR_LOADER_VERSION:          return  ("Module version not fix to current OS");
    case ERROR_LOADER_NOT_KO:           return  ("Not a kernel module");
#endif                                                                  /*  LW_CFG_MODULELOADER_EN      */

    /*
     *  mpi
     */
    case ERROR_DPMA_NULL:           return  ("Invalid DPMA");
    case ERROR_DPMA_FULL:           return  ("DPMA full");
    case ERROR_DPMA_OVERFLOW:       return  ("DPMA overflow");

    /*
     *  shell
     */
    case ERROR_TSHELL_EPARAM:           return  ("Invalid shell parameter(s)");
    case ERROR_TSHELL_OVERLAP:          return  ("Keyword overlap");
    case ERROR_TSHELL_EKEYWORD:         return  ("Invalid keyword");
    case ERROR_TSHELL_EVAR:             return  ("Invalid variable");
    case ERROR_TSHELL_CANNT_OVERWRITE:  return  ("Can not over write");
    case ERROR_TSHELL_ENOUSER:          return  ("Invalid user name");
    case ERROR_TSHELL_EUSER:            return  ("No user");
    case ERROR_TSHELL_ELEVEL:           return  ("Insufficient permissions");
    case ERROR_TSHELL_CMDNOTFUND:       return  ("Can not find command");

    default:    return  ("Unkown error");
    }
}
/*********************************************************************************************************
** 函数名称: lib_strerror_r
** 功能描述: 
** 输　入  : 
** 输　出  : 
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
INT  lib_strerror_r (INT  iNum, PCHAR  pcBuffer, size_t stLen)
{
    lib_strlcpy(pcBuffer, lib_strerror(iNum), stLen);
    
    return  (0);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
